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

BC_FIXTURE_TEST_SUITE(tls_socket_tests, test::directory_setup_fixture)

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

        BC_REQUIRE_EQUAL(tls.initialize_context(), error::success);

        code ec{};
        const auto bind = test::loopback();
        acceptor.open(io::tcp::of(bind), ec);
        acceptor.bind(bind, ec);
        acceptor.listen(ec);
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
        BC_REQUIRE(pool.join());
    }

    code accept_result()
    {
        BC_REQUIRE(accepted.wait_for(5s) == std::future_status::ready);
        return accepted.get();
    }

    const logger log{};
    threadpool pool;
    settings::tls_server tls;
    io::strand strand;
    io::acceptor acceptor;
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
        REQUIRE_SUCCESS(socket, connect, test::loopback(port));
        client.start();
    }

    void flush()
    {
        test::write(socket, io::buffer(client.output()));
        client.output().clear();
    }

    // Exchange until established or failed (or the server closes).
    void handshake()
    {
        std::array<uint8_t, 4096> buffer{};
        while (!client.is_established() && !client.is_failed())
        {
            if (!try_flush())
                return;

            code ec{};
            const auto size = socket.read_some(io::buffer(buffer), ec);
            if (ec)
                return;

            client.receive({ buffer.data(), size });
        }

        try_flush();
    }

    bool try_flush()
    {
        code ec{};
        io::write(socket, io::buffer(client.output()), ec);
        client.output().clear();
        return !ec;
    }

    system::data_chunk read(size_t size)
    {
        system::data_chunk out{};
        std::array<uint8_t, 4096> buffer{};
        while (out.size() < size)
        {
            code ec{};
            const auto count = socket.read_some(io::buffer(buffer), ec);
            BC_REQUIRE(!ec);
            raw.insert(raw.cend(), buffer.cbegin(), std::next(buffer.cbegin(), count));
            client.receive({ buffer.data(), count });
            const auto data = client.read();
            out.insert(out.end(), data.begin(), data.end());
        }

        return out;
    }

    // Feed received records to the client until end of file or timeout.
    // True if end of file.
    bool receive(const std::chrono::milliseconds& timeout)
    {
        std::array<uint8_t, 4096> buffer{};
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        REQUIRE_SUCCESS(socket, non_blocking, true);
        while (std::chrono::steady_clock::now() < deadline)
        {
            code ec{};
            const auto size = socket.read_some(io::buffer(buffer), ec);
            if (ec == error::would_block)
                std::this_thread::sleep_for(10ms);
            else if (ec)
                return true;
            else
                client.receive({ buffer.data(), size });
        }

        return false;
    }

    io::context io{};
    io::socket socket;
    test::tls_client client;
    system::data_chunk raw{};
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

    BC_REQUIRE(future.wait_for(5s) == std::future_status::ready);
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

    BC_REQUIRE(future.wait_for(5s) == std::future_status::ready);
    BC_REQUIRE_EQUAL(future.get(), error::success);
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

    BC_REQUIRE(future.wait_for(5s) == std::future_status::ready);
    return future.get();
}

// tls_server settings

BC_AUTO_TEST_CASE(tls_socket__initialize_context__identity_files__success)
{
    const std::filesystem::path directory{ TEST_DIRECTORY };
    settings::tls_server instance{ "test" };
    instance.safes = { config::authority{ "127.0.0.1:1" } };
    instance.cert_path = directory / "server.pem";
    instance.key_path = directory / "server.key";
    make_identity(server_secret, "server", instance.cert_path, instance.key_path);
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::success);
    BC_REQUIRE(instance.context->is_ready());
    BC_REQUIRE(!instance.context->request());
}

BC_AUTO_TEST_CASE(tls_socket__initialize_context__certificate_is_key__tls_use_certificate)
{
    const std::filesystem::path directory{ TEST_DIRECTORY };
    settings::tls_server instance{ "test" };
    instance.safes = { config::authority{ "127.0.0.1:1" } };
    instance.cert_path = directory / "server.key";
    instance.key_path = directory / "server.key";
    make_identity(server_secret, "server", directory / "server.pem", instance.key_path);
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::tls_use_certificate);
}

BC_AUTO_TEST_CASE(tls_socket__initialize_context__missing_certificate__tls_use_certificate)
{
    const std::filesystem::path directory{ TEST_DIRECTORY };
    settings::tls_server instance{ "test" };
    instance.safes = { config::authority{ "127.0.0.1:1" } };
    instance.cert_path = directory / "missing.pem";
    instance.key_path = directory / "missing.key";
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::tls_use_certificate);
}

BC_AUTO_TEST_CASE(tls_socket__initialize_context__mismatched_key__tls_use_private_key)
{
    const std::filesystem::path directory{ TEST_DIRECTORY };
    settings::tls_server instance{ "test" };
    instance.safes = { config::authority{ "127.0.0.1:1" } };
    instance.cert_path = directory / "server.pem";
    instance.key_path = directory / "client.key";
    make_identity(server_secret, "server", instance.cert_path, directory / "server.key");
    make_identity(client_secret, "client", directory / "client.pem", instance.key_path);
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::tls_use_private_key);
}

BC_AUTO_TEST_CASE(tls_socket__initialize_context__encrypted_key__password_required)
{
    const std::filesystem::path directory{ TEST_DIRECTORY };
    system::x509::secret secret{};
    BC_REQUIRE(system::x509::decode_private_key(secret, encrypted_key, "libbitcoin"));
    make_identity(secret, "server", directory / "server.pem", directory / "server.key");
    write_text(directory / "encrypted.key", encrypted_key);

    settings::tls_server instance{ "test" };
    instance.safes = { config::authority{ "127.0.0.1:1" } };
    instance.cert_path = directory / "server.pem";
    instance.key_path = directory / "encrypted.key";
    instance.key_pass = "libbitcoin";
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::success);

    settings::tls_server wrong{ "test" };
    wrong.safes = instance.safes;
    wrong.cert_path = instance.cert_path;
    wrong.key_path = instance.key_path;
    wrong.key_pass = "bitcoin";
    BC_REQUIRE_EQUAL(wrong.initialize_context(), error::tls_use_private_key);

    settings::tls_server none{ "test" };
    none.safes = instance.safes;
    none.cert_path = instance.cert_path;
    none.key_path = instance.key_path;
    BC_REQUIRE_EQUAL(none.initialize_context(), error::tls_use_private_key);
}

BC_AUTO_TEST_CASE(tls_socket__initialize_context__authority_directory__anchors_and_verify)
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
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::success);
    BC_REQUIRE_EQUAL(instance.context->anchors().size(), 1u);
    BC_REQUIRE(instance.context->request());
    BC_REQUIRE(instance.context->require());
}

BC_AUTO_TEST_CASE(tls_socket__initialize_context__missing_authority_directory__tls_set_add_verify)
{
    const std::filesystem::path directory{ TEST_DIRECTORY };
    settings::tls_server instance{ "test" };
    instance.safes = { config::authority{ "127.0.0.1:1" } };
    instance.cert_path = directory / "server.pem";
    instance.key_path = directory / "server.key";
    instance.cert_auth = directory / "missing";
    make_identity(server_secret, "server", instance.cert_path, instance.key_path);
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::tls_set_add_verify);
}

// socket

BC_AUTO_TEST_CASE(tls_socket__accept__handshake_and_exchange__success)
{
    server_fixture server{ false };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BC_REQUIRE(peer.client.is_established());
    BC_REQUIRE_EQUAL(server.accept_result(), error::success);

    const auto response = system::to_chunk("response from the server");
    BC_REQUIRE_EQUAL(tcp_write(server.server, response), error::success);
    BC_REQUIRE_EQUAL(peer.read(response.size()), response);

    const auto request = system::to_chunk("request from the client");
    BC_REQUIRE(peer.client.write(request));
    peer.flush();
    BC_REQUIRE_EQUAL(tcp_read(server.server, request.size()), request);
}

BC_AUTO_TEST_CASE(tls_socket__accept__response_of_many_records__exchanged)
{
    server_fixture server{ false };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BC_REQUIRE_EQUAL(server.accept_result(), error::success);

    system::data_chunk response(100000);
    std::iota(response.begin(), response.end(), uint8_t{});
    const auto promise = std::make_shared<std::promise<code>>();
    auto written = promise->get_future();
    server.server->tcp_write({ response.data(), response.size() }, [=](const code& ec, size_t) NOEXCEPT
    {
        promise->set_value(ec);
    });

    BC_REQUIRE_EQUAL(peer.read(response.size()), response);
    BC_REQUIRE(written.wait_for(5s) == std::future_status::ready);
    BC_REQUIRE_EQUAL(written.get(), error::success);
}

// The count of complete records in raw bytes.
static size_t count_records(const system::data_chunk& raw)
{
    size_t count{};
    size_t offset{};
    while (raw.size() >= offset + tls::record_header_size)
    {
        const auto length = (static_cast<size_t>(raw[offset + 3]) << 8) | raw[offset + 4];
        offset += tls::record_header_size + length;
        ++count;
    }

    return count;
}

BC_AUTO_TEST_CASE(tls_socket__accept__http_response__one_record)
{
    server_fixture server{ false };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BC_REQUIRE_EQUAL(server.accept_result(), error::success);

    const std::string body{ "response from the server" };
    const std::string expected{ "HTTP/1.1 200 OK\r\nContent-Length: 24\r\n\r\n" + body };
    http::response response{ http::status::ok, 11 };
    response.body() = http::string_value{ body };
    response.content_length(body.size());
    const auto promise = std::make_shared<std::promise<code>>();
    auto written = promise->get_future();
    server.server->http_write(std::move(response), [=](const code& ec, size_t) NOEXCEPT
    {
        promise->set_value(ec);
    });

    BC_REQUIRE_EQUAL(system::to_string(peer.read(expected.size())), expected);
    BC_REQUIRE(written.wait_for(5s) == std::future_status::ready);
    BC_REQUIRE_EQUAL(written.get(), error::success);
    BC_REQUIRE_EQUAL(count_records(peer.raw), 1u);
}

BC_AUTO_TEST_CASE(tls_socket__accept__trusted_client_certificate__success)
{
    server_fixture server{ true };
    tls_peer peer{ client_options(server, true), server.port };
    peer.handshake();
    BC_REQUIRE(peer.client.is_requested());
    BC_REQUIRE(peer.client.is_established());
    BC_REQUIRE_EQUAL(server.accept_result(), error::success);
}

BC_AUTO_TEST_CASE(tls_socket__accept__absent_client_certificate__failure)
{
    server_fixture server{ true };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BC_REQUIRE(peer.client.is_requested());
    BC_REQUIRE_EQUAL(server.accept_result(), error::tls_alert_certificate_required);
}

BC_AUTO_TEST_CASE(tls_socket__lazy_stop__zero_connect_timeout__closed_without_close_notify)
{
    server_fixture server{ false };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BC_REQUIRE(peer.client.is_established());
    BC_REQUIRE_EQUAL(server.accept_result(), error::success);

    server.server->lazy_stop();
    BC_REQUIRE(server.server->stopped());

    const auto closed = peer.receive(5s);
    peer.socket.close();
    BC_REQUIRE(closed);
    BC_REQUIRE(!peer.client.is_closed());
}

// A peer is not required to answer close_notify (rfc8446 6.1).
// The connect timeout also bounds the handshake, which it may cancel.
BC_AUTO_TEST_CASE(tls_socket__lazy_stop__unanswered_connect_timeout__closed)
{
    server_fixture server{ false, 100ms };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    const auto ec = server.accept_result();
    BC_REQUIRE(ec == error::success || ec == error::operation_canceled);

    server.server->lazy_stop();
    BC_REQUIRE(server.server->stopped());

    const auto closed = peer.receive(5s);
    peer.socket.close();
    BC_REQUIRE(closed);
}

BC_AUTO_TEST_CASE(tls_socket__read__peer_close_notify__peer_disconnect)
{
    server_fixture server{ false };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BC_REQUIRE_EQUAL(server.accept_result(), error::success);

    peer.client.close();
    peer.flush();
    BC_REQUIRE_EQUAL(tcp_read_result(server.server), error::peer_disconnect);
}

BC_AUTO_TEST_CASE(tls_socket__read__transport_closed__tls_stream_truncated)
{
    server_fixture server{ false };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BC_REQUIRE_EQUAL(server.accept_result(), error::success);

    REQUIRE_SUCCESS(peer.socket, shutdown, io::socket::shutdown_type::send);
    BC_REQUIRE_EQUAL(tcp_read_result(server.server), error::tls_stream_truncated);
}

BC_AUTO_TEST_CASE(tls_socket__read__tampered_record__tls_alert_bad_record_mac)
{
    server_fixture server{ false };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BC_REQUIRE_EQUAL(server.accept_result(), error::success);

    BC_REQUIRE(peer.client.write(system::to_chunk("data")));
    peer.client.output().back() ^= 0x01;
    peer.flush();
    BC_REQUIRE_EQUAL(tcp_read_result(server.server), error::tls_alert_bad_record_mac);
}

BC_AUTO_TEST_CASE(tls_socket__read__requested_key_update__exchanged)
{
    server_fixture server{ false };
    tls_peer peer{ client_options(server, false), server.port };
    peer.handshake();
    BC_REQUIRE_EQUAL(server.accept_result(), error::success);

    const auto request = system::to_chunk("request after update");
    peer.client.update(true);
    BC_REQUIRE(peer.client.write(request));
    peer.flush();
    BC_REQUIRE_EQUAL(tcp_read(server.server, request.size()), request);

    const auto response = system::to_chunk("response after update");
    BC_REQUIRE_EQUAL(tcp_write(server.server, response), error::success);
    BC_REQUIRE_EQUAL(peer.read(response.size()), response);
}

BC_AUTO_TEST_SUITE_END()
