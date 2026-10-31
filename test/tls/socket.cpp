/**
 * Copyright (c) 2011-2026 libbitcoin developers
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../test.hpp"
#include "client.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <numeric>
#include <string>
#include <thread>

BOOST_FIXTURE_TEST_SUITE(tls_socket_tests, test::directory_setup_fixture)

using namespace std::chrono_literals;

const auto server_secret = system::base16_array("c9afa9d845ba75166b5c215767b1d6934e50c3db36e89b127b8a622b120f6721");
const auto client_secret = system::base16_array("0000000000000000000000000000000000000000000000000000000000000002");

// aes-128-cbc pbes2 encryption of a P-256 key with password "libbitcoin".
static const std::string encrypted_key
{
    "-----BEGIN ENCRYPTED PRIVATE KEY-----\n"
    "MIH0MF8GCSqGSIb3DQEFDTBSMDEGCSqGSIb3DQEFDDAkBBCeWYC8K8tt3k1p2W2C\n"
    "BSHPAgIIADAMBggqhkiG9w0CCQUAMB0GCWCGSAFlAwQBAgQQEEqL3F4LDJRnsq8k\n"
    "KuQWggSBkItfC8QMn/9QWyl/BuEi9FnmiA+Xg3EsCBEP7XSIFukis6DOCVsv/wd0\n"
    "SzMahuSsbEWC5ymzqN8bB0tQSKPA5MusA5KO27+UsZ1RsoJMgYyCLOKiEQYSBxwd\n"
    "ZgyQwKN6I9azQBYO8xaRnBc17nIiXcn6pMNriEdIoV+3CoyCKvo5h4KKnY6i/9GW\n"
    "pLS7iXVnWA==\n"
    "-----END ENCRYPTED PRIVATE KEY-----\n"
};

static void write_text(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream file(path, std::ios::binary);
    file.write(text.data(), system::possible_narrow_sign_cast<std::streamsize>(text.size()));
}

static system::x509::certificate make_identity(
    const system::x509::secret& key, const std::string& name,
    const std::filesystem::path& certificate,
    const std::filesystem::path& private_key)
{
    system::x509::subject subject{};
    subject.common_name = name;
    subject.dns_names = { "localhost" };
    subject.not_before = 1735689600;
    subject.not_after = 2524608000;

    system::data_chunk der{};
    system::x509::build_self_signed(der, key, subject);
    write_text(certificate, system::x509::encode_certificate(der));
    write_text(private_key, system::x509::encode_private_key(key));

    system::x509::certificate out{};
    system::x509::parse(out, der);
    return out;
}

// A network::socket accepting on loopback with the tls_server context.
struct server_fixture
{
    server_fixture(bool authenticate,
        const steady_clock::duration& connect_timeout={})
      : pool(2), tls("test"),
        strand(pool.service().get_executor()),
        acceptor(strand)
    {
        const std::filesystem::path directory{ TEST_DIRECTORY };
        tls.cert_path = directory / "server.pem";
        tls.key_path = directory / "server.key";
        tls.safes = { config::authority{ "127.0.0.1:1" } };
        certificate = make_identity(server_secret, "server", tls.cert_path, tls.key_path);

        const auto clients = directory / "clients";
        std::filesystem::create_directories(clients);
        client = make_identity(client_secret,
            "client", clients / "client.pem", directory / "client.key");
        tls.cert_auth = authenticate ? clients : std::filesystem::path{};

        BOOST_REQUIRE_EQUAL(tls.initialize_context(), error::success);

        boost_code ec{};
        const asio::endpoint bind(asio::ipv4::loopback(), 0);
        acceptor.open(bind.protocol(), ec);
        acceptor.bind(bind, ec);
        acceptor.listen(1, ec);
        port = acceptor.local_endpoint().port();

        socket::parameters params{};
        params.connect_timeout = connect_timeout;
        params.maximum_request = 1024;
        params.maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer;
        params.context = tls.secure_context();
        server = std::make_shared<network::socket>(log, pool.service(), params);

        const auto promise = std::make_shared<std::promise<code>>();
        accepted = promise->get_future();
        server->accept(acceptor, [=](const code& ec) NOEXCEPT
        {
            promise->set_value(ec);
        });
    }

    ~server_fixture()
    {
        server->stop();
        pool.stop();
        BOOST_REQUIRE(pool.join());
    }

    code accept_result()
    {
        BOOST_REQUIRE(accepted.wait_for(5s) == std::future_status::ready);
        return accepted.get();
    }

    const logger log{};
    threadpool pool;
    settings::tls_server tls;
    asio::strand strand;
    asio::acceptor acceptor;
    uint16_t port{};
    system::x509::certificate certificate{};
    system::x509::certificate client{};
    socket::ptr server{};
    std::future<code> accepted{};
};

// The harness client over a blocking loopback socket.
struct tls_peer
{
    tls_peer(const test::tls_client::options& options, uint16_t port)
      : socket(io), client(options)
    {
        socket.connect({ boost::asio::ip::make_address("127.0.0.1"), port });
        client.start();
    }

    void flush()
    {
        boost::asio::write(socket, boost::asio::buffer(client.output()));
        client.output().clear();
    }

    // Exchange until established or failed (or the server closes).
    void handshake()
    {
        std::array<uint8_t, 4096> buffer{};
        while (!client.is_established() && !client.is_failed())
        {
            flush();
            boost::system::error_code ec{};
            const auto size = socket.read_some(boost::asio::buffer(buffer), ec);
            if (ec)
                return;

            client.receive({ buffer.data(), size });
        }

        flush();
    }

    system::data_chunk read(size_t size)
    {
        system::data_chunk out{};
        std::array<uint8_t, 4096> buffer{};
        while (out.size() < size)
        {
            const auto count = socket.read_some(boost::asio::buffer(buffer));
            client.receive({ buffer.data(), count });
            const auto data = client.read();
            out.insert(out.end(), data.begin(), data.end());
        }

        return out;
    }

    // Feed received records to the client until end of file or timeout, or
    // until the client is closed (if closing). True if end of file.
    bool receive(const std::chrono::milliseconds& timeout, bool closing=false)
    {
        std::array<uint8_t, 4096> buffer{};
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        socket.non_blocking(true);
        while ((std::chrono::steady_clock::now() < deadline) &&
            !(closing && client.is_closed()))
        {
            boost::system::error_code ec{};
            const auto size = socket.read_some(boost::asio::buffer(buffer), ec);
            if (ec == boost::asio::error::would_block ||
                ec == boost::asio::error::try_again)
                std::this_thread::sleep_for(10ms);
            else if (ec)
                return true;
            else
                client.receive({ buffer.data(), size });
        }

        return false;
    }

    boost::asio::io_context io{};
    boost::asio::ip::tcp::socket socket;
    test::tls_client client;
};

static test::tls_client::options client_options(const server_fixture& server,
    bool present)
{
    test::tls_client::options value{};
    value.anchors = { server.certificate };
    value.time = 1767225600;
    value.chain = present ?
        std::vector<system::data_chunk>{ server.client.encoding } :
        std::vector<system::data_chunk>{};
    value.key = client_secret;
    return value;
}

static code tcp_write(const socket::ptr& server, const system::data_chunk& data)
{
    const auto promise = std::make_shared<std::promise<code>>();
    auto future = promise->get_future();
    server->tcp_write({ data.data(), data.size() },
        [=](const code& ec, size_t) NOEXCEPT
        {
            promise->set_value(ec);
        });

    BOOST_REQUIRE(future.wait_for(5s) == std::future_status::ready);
    return future.get();
}

static system::data_chunk tcp_read(const socket::ptr& server, size_t size)
{
    const auto buffer = std::make_shared<system::data_chunk>(size);
    const auto promise = std::make_shared<std::promise<code>>();
    auto future = promise->get_future();
    server->tcp_read({ buffer->data(), buffer->size() },
        [=](const code& ec, size_t) NOEXCEPT
        {
            promise->set_value(ec);
        });

    BOOST_REQUIRE(future.wait_for(5s) == std::future_status::ready);
    BOOST_REQUIRE_EQUAL(future.get(), error::success);
    return *buffer;
}

static code tcp_read_result(const socket::ptr& server)
{
    const auto buffer = std::make_shared<system::data_chunk>(8);
    const auto promise = std::make_shared<std::promise<code>>();
    auto future = promise->get_future();
    server->tcp_read({ buffer->data(), buffer->size() }, [=](const code& ec, size_t) NOEXCEPT
    {
        promise->set_value(ec);
    });

    BOOST_REQUIRE(future.wait_for(5s) == std::future_status::ready);
    return future.get();
}

// tls_server settings

BOOST_AUTO_TEST_CASE(tls_socket__initialize_context__identity_files__success)
{
    const std::filesystem::path directory{ TEST_DIRECTORY };
    settings::tls_server instance{ "test" };
    instance.safes = { config::authority{ "127.0.0.1:1" } };
    instance.cert_path = directory / "server.pem";
    instance.key_path = directory / "server.key";
    make_identity(server_secret, "server", instance.cert_path, instance.key_path);
    BOOST_REQUIRE_EQUAL(instance.initialize_context(), error::success);
    BOOST_REQUIRE(instance.context->is_ready());
    BOOST_REQUIRE(!instance.context->request());
}

BOOST_AUTO_TEST_CASE(tls_socket__initialize_context__certificate_is_key__tls_use_certificate)
{
    const std::filesystem::path directory{ TEST_DIRECTORY };
    settings::tls_server instance{ "test" };
    instance.safes = { config::authority{ "127.0.0.1:1" } };
    instance.cert_path = directory / "server.key";
    instance.key_path = directory / "server.key";
    make_identity(server_secret, "server", directory / "server.pem", instance.key_path);
    BOOST_REQUIRE_EQUAL(instance.initialize_context(), error::tls_use_certificate);
}

BOOST_AUTO_TEST_CASE(tls_socket__initialize_context__missing_certificate__tls_use_certificate)
{
    const std::filesystem::path directory{ TEST_DIRECTORY };
    settings::tls_server instance{ "test" };
    instance.safes = { config::authority{ "127.0.0.1:1" } };
    instance.cert_path = directory / "missing.pem";
    instance.key_path = directory / "missing.key";
    BOOST_REQUIRE_EQUAL(instance.initialize_context(), error::tls_use_certificate);
}

BOOST_AUTO_TEST_CASE(tls_socket__initialize_context__mismatched_key__tls_use_private_key)
{
    const std::filesystem::path directory{ TEST_DIRECTORY };
    settings::tls_server instance{ "test" };
    instance.safes = { config::authority{ "127.0.0.1:1" } };
    instance.cert_path = directory / "server.pem";
    instance.key_path = directory / "client.key";
    make_identity(server_secret, "server", instance.cert_path, directory / "server.key");
    make_identity(client_secret, "client", directory / "client.pem", instance.key_path);
    BOOST_REQUIRE_EQUAL(instance.initialize_context(), error::tls_use_private_key);
}

BOOST_AUTO_TEST_CASE(tls_socket__initialize_context__encrypted_key__password_required)
{
    const std::filesystem::path directory{ TEST_DIRECTORY };
    system::x509::secret secret{};
    BOOST_REQUIRE(system::x509::decode_private_key(secret, encrypted_key, "libbitcoin"));
    make_identity(secret, "server", directory / "server.pem", directory / "server.key");
    write_text(directory / "encrypted.key", encrypted_key);

    settings::tls_server instance{ "test" };
    instance.safes = { config::authority{ "127.0.0.1:1" } };
    instance.cert_path = directory / "server.pem";
    instance.key_path = directory / "encrypted.key";
    instance.key_pass = "libbitcoin";
    BOOST_REQUIRE_EQUAL(instance.initialize_context(), error::success);

    settings::tls_server wrong{ "test" };
    wrong.safes = instance.safes;
    wrong.cert_path = instance.cert_path;
    wrong.key_path = instance.key_path;
    wrong.key_pass = "bitcoin";
    BOOST_REQUIRE_EQUAL(wrong.initialize_context(), error::tls_use_private_key);

    settings::tls_server none{ "test" };
    none.safes = instance.safes;
    none.cert_path = instance.cert_path;
    none.key_path = instance.key_path;
    BOOST_REQUIRE_EQUAL(none.initialize_context(), error::tls_use_private_key);
}

BOOST_AUTO_TEST_CASE(tls_socket__initialize_context__authority_directory__anchors_and_verify)
{
    const std::filesystem::path directory{ TEST_DIRECTORY };
    const auto clients = directory / "clients";
    std::filesystem::create_directories(clients);
    make_identity(client_secret, "client", clients / "client.pem", directory / "client.key");
    write_text(clients / "readme.txt", "not a certificate");

    settings::tls_server instance{ "test" };
    instance.safes = { config::authority{ "127.0.0.1:1" } };
    instance.cert_path = directory / "server.pem";
    instance.key_path = directory / "server.key";
    instance.cert_auth = clients;
    make_identity(server_secret, "server", instance.cert_path, instance.key_path);
    BOOST_REQUIRE_EQUAL(instance.initialize_context(), error::success);
    BOOST_REQUIRE_EQUAL(instance.context->anchors().size(), 1u);
    BOOST_REQUIRE(instance.context->request());
    BOOST_REQUIRE(instance.context->require());
}

BOOST_AUTO_TEST_CASE(tls_socket__initialize_context__missing_authority_directory__tls_set_add_verify)
{
    const std::filesystem::path directory{ TEST_DIRECTORY };
    settings::tls_server instance{ "test" };
    instance.safes = { config::authority{ "127.0.0.1:1" } };
    instance.cert_path = directory / "server.pem";
    instance.key_path = directory / "server.key";
    instance.cert_auth = directory / "missing";
    make_identity(server_secret, "server", instance.cert_path, instance.key_path);
    BOOST_REQUIRE_EQUAL(instance.initialize_context(), error::tls_set_add_verify);
}

// socket

BOOST_AUTO_TEST_CASE(tls_socket__accept__handshake_and_exchange__success)
{
    server_fixture server{ false };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BOOST_REQUIRE(peer.client.is_established());
    BOOST_REQUIRE_EQUAL(server.accept_result(), error::success);

    const auto response = system::to_chunk("response from the server");
    BOOST_REQUIRE_EQUAL(tcp_write(server.server, response), error::success);
    BOOST_REQUIRE_EQUAL(peer.read(response.size()), response);

    const auto request = system::to_chunk("request from the client");
    BOOST_REQUIRE(peer.client.write(request));
    peer.flush();
    BOOST_REQUIRE_EQUAL(tcp_read(server.server, request.size()), request);
}

BOOST_AUTO_TEST_CASE(tls_socket__accept__response_of_many_records__exchanged)
{
    server_fixture server{ false };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BOOST_REQUIRE_EQUAL(server.accept_result(), error::success);

    system::data_chunk response(100000);
    std::iota(response.begin(), response.end(), uint8_t{});
    const auto promise = std::make_shared<std::promise<code>>();
    auto written = promise->get_future();
    server.server->tcp_write({ response.data(), response.size() }, [=](const code& ec, size_t) NOEXCEPT
    {
        promise->set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(peer.read(response.size()), response);
    BOOST_REQUIRE(written.wait_for(5s) == std::future_status::ready);
    BOOST_REQUIRE_EQUAL(written.get(), error::success);
}

BOOST_AUTO_TEST_CASE(tls_socket__accept__trusted_client_certificate__success)
{
    server_fixture server{ true };
    tls_peer peer{ client_options(server, true), server.port };
    peer.handshake();
    BOOST_REQUIRE(peer.client.is_requested());
    BOOST_REQUIRE(peer.client.is_established());
    BOOST_REQUIRE_EQUAL(server.accept_result(), error::success);
}

BOOST_AUTO_TEST_CASE(tls_socket__accept__absent_client_certificate__failure)
{
    server_fixture server{ true };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BOOST_REQUIRE(peer.client.is_requested());
    BOOST_REQUIRE_EQUAL(server.accept_result(), error::tls_alert_certificate_required);
}

BOOST_AUTO_TEST_CASE(tls_socket__lazy_stop__zero_connect_timeout__closed_without_close_notify)
{
    server_fixture server{ false };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BOOST_REQUIRE(peer.client.is_established());
    BOOST_REQUIRE_EQUAL(server.accept_result(), error::success);

    server.server->lazy_stop();
    BOOST_REQUIRE(server.server->stopped());

    const auto closed = peer.receive(5s);
    peer.socket.close();
    BOOST_REQUIRE(closed);
    BOOST_REQUIRE(!peer.client.is_closed());
}

// A peer is not required to answer close_notify (rfc8446 6.1).
BOOST_AUTO_TEST_CASE(tls_socket__lazy_stop__unanswered_connect_timeout__closed)
{
    server_fixture server{ false, 100ms };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BOOST_REQUIRE(peer.client.is_established());
    BOOST_REQUIRE_EQUAL(server.accept_result(), error::success);

    server.server->lazy_stop();
    BOOST_REQUIRE(server.server->stopped());
    BOOST_REQUIRE(!peer.receive(5s, true));
    BOOST_REQUIRE(peer.client.is_closed());

    const auto closed = peer.receive(5s);
    peer.socket.close();
    BOOST_REQUIRE(closed);
}

BOOST_AUTO_TEST_CASE(tls_socket__read__peer_close_notify__peer_disconnect)
{
    server_fixture server{ false };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BOOST_REQUIRE_EQUAL(server.accept_result(), error::success);

    peer.client.close();
    peer.flush();
    BOOST_REQUIRE_EQUAL(tcp_read_result(server.server), error::peer_disconnect);
}

BOOST_AUTO_TEST_CASE(tls_socket__read__transport_closed__tls_stream_truncated)
{
    server_fixture server{ false };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BOOST_REQUIRE_EQUAL(server.accept_result(), error::success);

    peer.socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
    BOOST_REQUIRE_EQUAL(tcp_read_result(server.server), error::tls_stream_truncated);
}

BOOST_AUTO_TEST_CASE(tls_socket__read__tampered_record__tls_alert_bad_record_mac)
{
    server_fixture server{ false };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BOOST_REQUIRE_EQUAL(server.accept_result(), error::success);

    BOOST_REQUIRE(peer.client.write(system::to_chunk("data")));
    peer.client.output().back() ^= 0x01;
    peer.flush();
    BOOST_REQUIRE_EQUAL(tcp_read_result(server.server), error::tls_alert_bad_record_mac);
}

BOOST_AUTO_TEST_CASE(tls_socket__read__requested_key_update__exchanged)
{
    server_fixture server{ false };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BOOST_REQUIRE_EQUAL(server.accept_result(), error::success);

    const auto request = system::to_chunk("request after update");
    peer.client.update(true);
    BOOST_REQUIRE(peer.client.write(request));
    peer.flush();
    BOOST_REQUIRE_EQUAL(tcp_read(server.server, request.size()), request);

    const auto response = system::to_chunk("response after update");
    BOOST_REQUIRE_EQUAL(tcp_write(server.server, response), error::success);
    BOOST_REQUIRE_EQUAL(peer.read(response.size()), response);
}

BOOST_AUTO_TEST_SUITE_END()
