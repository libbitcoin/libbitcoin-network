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

BOOST_FIXTURE_TEST_SUITE(tls_socket_tests, test::directory_setup_fixture)

using namespace std::chrono_literals;

const auto server_secret = system::base16_array("c9afa9d845ba75166b5c215767b1d6934e50c3db36e89b127b8a622b120f6721");
const auto client_secret = system::base16_array("0000000000000000000000000000000000000000000000000000000000000002");

static void write_text(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream file(path, std::ios::binary);
    file.write(text.data(), system::possible_narrow_sign_cast<std::streamsize>(text.size()));
}

static system::x509::certificate make_identity(const system::x509::secret& key, const std::string& name, const std::filesystem::path& certificate, const std::filesystem::path& private_key)
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
    server_fixture(bool authenticate)
      : pool(2), tls("test"), strand(pool.service().get_executor()), acceptor(strand)
    {
        const std::filesystem::path directory{ TEST_DIRECTORY };
        tls.cert_path = directory / "server.pem";
        tls.key_path = directory / "server.key";
        tls.safes = { config::authority{ "127.0.0.1:1" } };
        certificate = make_identity(server_secret, "server", tls.cert_path, tls.key_path);

        const auto clients = directory / "clients";
        std::filesystem::create_directories(clients);
        client = make_identity(client_secret, "client", clients / "client.pem", directory / "client.key");
        tls.cert_auth = authenticate ? clients : std::filesystem::path{};

        BOOST_REQUIRE_EQUAL(tls.initialize_context(), error::success);

        boost_code ec{};
        const asio::endpoint bind(asio::ipv4::loopback(), 0);
        acceptor.open(bind.protocol(), ec);
        acceptor.bind(bind, ec);
        acceptor.listen(1, ec);
        port = acceptor.local_endpoint().port();

        socket::parameters params{};
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

    boost::asio::io_context io{};
    boost::asio::ip::tcp::socket socket;
    test::tls_client client;
};

static test::tls_client::options client_options(const server_fixture& server, bool present)
{
    test::tls_client::options value{};
    value.anchors = { server.certificate };
    value.time = 1767225600;
    value.chain = present ? std::vector<system::data_chunk>{ server.client.encoding } : std::vector<system::data_chunk>{};
    value.key = client_secret;
    return value;
}

static code tcp_write(const socket::ptr& server, const system::data_chunk& data)
{
    const auto promise = std::make_shared<std::promise<code>>();
    auto future = promise->get_future();
    server->tcp_write({ data.data(), data.size() }, [=](const code& ec, size_t) NOEXCEPT
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
    server->tcp_read({ buffer->data(), buffer->size() }, [=](const code& ec, size_t) NOEXCEPT
    {
        promise->set_value(ec);
    });

    BOOST_REQUIRE(future.wait_for(5s) == std::future_status::ready);
    BOOST_REQUIRE_EQUAL(future.get(), error::success);
    return *buffer;
}

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
    BOOST_REQUIRE_EQUAL(tcp_write(server.server, response), error::success);
    BOOST_REQUIRE_EQUAL(peer.read(response.size()), response);
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

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(tls_openssl_tests, test::directory_setup_fixture)

using namespace bc::system;

static const auto shim_server_secret = base16_array("c9afa9d845ba75166b5c215767b1d6934e50c3db36e89b127b8a622b120f6721");
static const auto shim_client_secret = base16_array("0000000000000000000000000000000000000000000000000000000000000002");
static const auto shim_other_secret = base16_array("0000000000000000000000000000000000000000000000000000000000000003");
static const std::string shim_password{ "libbitcoin" };

// aes-128-cbc pbes2 key of password shim_password (openssl 3.5.7).
static const std::string shim_encrypted_key
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

static void write_file(const std::string& path, const std::string& text)
{
    std::ofstream file(std::filesystem::path{ path }, std::ios::binary);
    file.write(text.data(), possible_narrow_sign_cast<std::streamsize>(text.size()));
}

static x509::certificate write_identity(const x509::secret& key, const std::string& name, const std::string& certificate, const std::string& private_key)
{
    x509::subject subject{};
    subject.common_name = name;
    subject.dns_names = { "localhost" };
    subject.not_before = 1735689600;
    subject.not_after = 2524608000;

    data_chunk der{};
    x509::build_self_signed(der, key, subject);
    write_file(certificate, x509::encode_certificate(der));
    write_file(private_key, x509::encode_private_key(key));

    x509::certificate out{};
    x509::parse(out, der);
    return out;
}

static int copy_password(char* buffer, int, int, void* userdata)
{
    const auto& value = *static_cast<const std::string*>(userdata);
    std::copy_n(value.begin(), value.size(), buffer);
    return possible_narrow_sign_cast<int>(value.size());
}

static int accept_all(int, X509_STORE_CTX*)
{
    return 1;
}

static unsigned long alert_error(uint8_t alert)
{
    return ERR_PACK(ERR_LIB_SSL, 0, SSL_AD_REASON_OFFSET + alert);
}

static std::string error_string(unsigned long code, size_t size)
{
    std::string buffer(size, 'x');
    ERR_error_string_n(code, buffer.data(), size);
    return { buffer.c_str() };
}

// Server and client identity files and a TLS server method context.
struct shim_setup
{
    shim_setup()
      : context(SSL_CTX_new(TLS_server_method()))
    {
        server = write_identity(shim_server_secret, "server", certificate_path, key_path);
        std::filesystem::create_directories(clients_path);
        client = write_identity(shim_client_secret, "client", client_path, TEST_DIRECTORY + "/client.key");
        write_file(clients_path + "/readme.txt", "not a certificate");
        ERR_clear_error();
    }

    ~shim_setup()
    {
        SSL_CTX_free(context);
        ERR_clear_error();
    }

    bool load() const
    {
        return (SSL_CTX_use_certificate_chain_file(context, certificate_path.c_str()) == 1) && (SSL_CTX_use_PrivateKey_file(context, key_path.c_str(), SSL_FILETYPE_PEM) == 1);
    }

    test::tls_client::options options(bool present) const
    {
        test::tls_client::options value{};
        value.anchors = { server };
        value.time = 1767225600;
        value.chain = present ? std::vector<data_chunk>{ client.encoding } : std::vector<data_chunk>{};
        value.key = shim_client_secret;
        return value;
    }

    const std::string certificate_path{ TEST_DIRECTORY + "/server.pem" };
    const std::string key_path{ TEST_DIRECTORY + "/server.key" };
    const std::string clients_path{ TEST_DIRECTORY + "/clients" };
    const std::string client_path{ TEST_DIRECTORY + "/clients/client.pem" };
    x509::certificate server{};
    x509::certificate client{};
    SSL_CTX* context;
};

// An SSL on the internal end of a bio pair, the test on the external end.
struct shim_connection
{
    shim_connection(SSL_CTX* context)
      : ssl(SSL_new(context))
    {
        BIO_new_bio_pair(&internal, 0, &external, 0);
        SSL_set_bio(ssl, internal, internal);
    }

    ~shim_connection()
    {
        SSL_free(ssl);
        BIO_free(external);
    }

    void send(test::tls_client& client) const
    {
        BIO_write(external, client.output().data(), possible_narrow_sign_cast<int>(client.output().size()));
        client.output().clear();
    }

    void receive(test::tls_client& client) const
    {
        data_chunk data(BIO_ctrl_pending(external));
        BIO_read(external, data.data(), possible_narrow_sign_cast<int>(data.size()));
        client.receive(data);
    }

    int handshake(test::tls_client& client) const
    {
        client.start();
        send(client);
        BOOST_REQUIRE_EQUAL(SSL_accept(ssl), -1);
        BOOST_REQUIRE_EQUAL(SSL_get_error(ssl, -1), SSL_ERROR_WANT_READ);
        receive(client);
        send(client);
        return SSL_accept(ssl);
    }

    SSL* ssl;
    BIO* internal{};
    BIO* external{};
};

// library

BOOST_AUTO_TEST_CASE(tls_openssl__library__version__tls13)
{
    BOOST_REQUIRE_EQUAL(std::string{ OpenSSL_version(OPENSSL_VERSION) }, "libbitcoin tls 1.3");
    CONF_modules_unload(1);
    OPENSSL_free(nullptr);
    OPENSSL_free(std::malloc(1));
}

// methods and contexts

BOOST_AUTO_TEST_CASE(tls_openssl__methods__aliases__same_roles)
{
    BOOST_REQUIRE(TLS_method() == SSLv23_method());
    BOOST_REQUIRE(TLS_client_method() == SSLv23_client_method());
    BOOST_REQUIRE(TLS_server_method() == SSLv23_server_method());
    BOOST_REQUIRE(TLS_method() != TLS_client_method());
    BOOST_REQUIRE(TLS_method() != TLS_server_method());
    BOOST_REQUIRE(TLS_client_method() != TLS_server_method());
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_new__null_method__null)
{
    BOOST_REQUIRE(is_null(SSL_CTX_new(nullptr)));
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_options__set_clear__accumulated)
{
    const shim_setup setup{};
    BOOST_REQUIRE_EQUAL(SSL_CTX_get_options(setup.context), 0u);
    BOOST_REQUIRE_EQUAL(SSL_CTX_set_options(setup.context, SSL_OP_NO_SSLv3), static_cast<unsigned long>(SSL_OP_NO_SSLv3));
    BOOST_REQUIRE_EQUAL(SSL_CTX_set_options(setup.context, SSL_OP_NO_TLSv1), static_cast<unsigned long>(SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1));
    BOOST_REQUIRE_EQUAL(SSL_CTX_clear_options(setup.context, SSL_OP_NO_SSLv3), static_cast<unsigned long>(SSL_OP_NO_TLSv1));
    BOOST_REQUIRE_EQUAL(SSL_CTX_get_options(setup.context), static_cast<unsigned long>(SSL_OP_NO_TLSv1));
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_proto_version__tls13_range__included)
{
    const shim_setup setup{};
    BOOST_REQUIRE_EQUAL(SSL_CTX_set_min_proto_version(setup.context, 0), 1);
    BOOST_REQUIRE_EQUAL(SSL_CTX_set_min_proto_version(setup.context, TLS1_2_VERSION), 1);
    BOOST_REQUIRE_EQUAL(SSL_CTX_set_min_proto_version(setup.context, TLS1_3_VERSION), 1);
    BOOST_REQUIRE_EQUAL(SSL_CTX_set_min_proto_version(setup.context, TLS1_3_VERSION + 1), 0);
    BOOST_REQUIRE_EQUAL(SSL_CTX_set_max_proto_version(setup.context, 0), 1);
    BOOST_REQUIRE_EQUAL(SSL_CTX_set_max_proto_version(setup.context, TLS1_3_VERSION), 1);
    BOOST_REQUIRE_EQUAL(SSL_CTX_set_max_proto_version(setup.context, TLS1_2_VERSION), 0);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_ex_data__index_zero__only)
{
    const shim_setup setup{};
    int value{};
    BOOST_REQUIRE(is_null(SSL_CTX_get_app_data(setup.context)));
    BOOST_REQUIRE_EQUAL(SSL_CTX_set_app_data(setup.context, &value), 1);
    BOOST_REQUIRE(SSL_CTX_get_app_data(setup.context) == &value);
    BOOST_REQUIRE_EQUAL(SSL_CTX_set_ex_data(setup.context, 1, &value), 0);
    BOOST_REQUIRE(is_null(SSL_CTX_get_ex_data(setup.context, 1)));
}

// context verification

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_set_verify__peer__mode_and_callback)
{
    const shim_setup setup{};
    SSL_CTX_set_verify(setup.context, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, &accept_all);
    SSL_CTX_set_verify_depth(setup.context, 4);
    BOOST_REQUIRE_EQUAL(SSL_CTX_get_verify_mode(setup.context), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT);
    BOOST_REQUIRE(SSL_CTX_get_verify_callback(setup.context) == &accept_all);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_load_verify_locations__neither__zero)
{
    const shim_setup setup{};
    BOOST_REQUIRE_EQUAL(SSL_CTX_load_verify_locations(setup.context, nullptr, nullptr), 0);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_load_verify_locations__missing_file__zero_error_queued)
{
    const shim_setup setup{};
    const std::string missing{ TEST_DIRECTORY + "/missing.pem" };
    BOOST_REQUIRE_EQUAL(SSL_CTX_load_verify_locations(setup.context, missing.c_str(), nullptr), 0);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), ERR_PACK(ERR_LIB_SSL, 0, 0));
    BOOST_REQUIRE_EQUAL(ERR_get_error(), 0u);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_load_verify_locations__not_certificate__zero_error_queued)
{
    const shim_setup setup{};
    const std::string junk{ setup.clients_path + "/readme.txt" };
    BOOST_REQUIRE_EQUAL(SSL_CTX_load_verify_locations(setup.context, junk.c_str(), nullptr), 0);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), ERR_PACK(ERR_LIB_SSL, 0, 0));
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_load_verify_locations__file__one)
{
    const shim_setup setup{};
    BOOST_REQUIRE_EQUAL(SSL_CTX_load_verify_locations(setup.context, setup.client_path.c_str(), nullptr), 1);
    BOOST_REQUIRE_EQUAL(ERR_peek_error(), 0u);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_load_verify_locations__directory__one)
{
    const shim_setup setup{};
    BOOST_REQUIRE_EQUAL(SSL_CTX_load_verify_locations(setup.context, nullptr, setup.clients_path.c_str()), 1);
    BOOST_REQUIRE_EQUAL(ERR_peek_error(), 0u);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_load_verify_locations__missing_directory__zero_error_queued)
{
    const shim_setup setup{};
    const std::string missing{ TEST_DIRECTORY + "/missing" };
    BOOST_REQUIRE_EQUAL(SSL_CTX_load_verify_locations(setup.context, nullptr, missing.c_str()), 0);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), ERR_PACK(ERR_LIB_SSL, 0, 0));
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_set_default_verify_paths__no_system_store__zero_error_queued)
{
    const shim_setup setup{};
    BOOST_REQUIRE_EQUAL(SSL_CTX_set_default_verify_paths(setup.context), 0);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), ERR_PACK(ERR_LIB_SSL, 0, 0));
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_cert_store__add_cert__unsupported)
{
    const shim_setup setup{};
    const auto store = SSL_CTX_get_cert_store(setup.context);
    BOOST_REQUIRE(!is_null(store));
    BOOST_REQUIRE(SSL_CTX_get_cert_store(setup.context) == store);
    BOOST_REQUIRE_EQUAL(X509_STORE_add_cert(store, nullptr), 0);
}

// context credentials

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_use_certificate_chain_file__valid__one)
{
    const shim_setup setup{};
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_certificate_chain_file(setup.context, setup.certificate_path.c_str()), 1);
    BOOST_REQUIRE_EQUAL(ERR_peek_error(), 0u);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_use_certificate_chain_file__null__zero_error_queued)
{
    const shim_setup setup{};
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_certificate_chain_file(setup.context, nullptr), 0);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), ERR_PACK(ERR_LIB_SSL, 0, 0));
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_use_certificate_chain_file__key_file__zero_error_queued)
{
    const shim_setup setup{};
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_certificate_chain_file(setup.context, setup.key_path.c_str()), 0);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), ERR_PACK(ERR_LIB_SSL, 0, 0));
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_use_certificate_file__pem__one)
{
    const shim_setup setup{};
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_certificate_file(setup.context, setup.certificate_path.c_str(), SSL_FILETYPE_PEM), 1);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_use_certificate_file__asn1__zero)
{
    const shim_setup setup{};
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_certificate_file(setup.context, setup.certificate_path.c_str(), SSL_FILETYPE_ASN1), 0);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_use_private_key_file__pem__one)
{
    const shim_setup setup{};
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_PrivateKey_file(setup.context, setup.key_path.c_str(), SSL_FILETYPE_PEM), 1);
    BOOST_REQUIRE_EQUAL(ERR_peek_error(), 0u);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_use_private_key_file__asn1__zero_error_queued)
{
    const shim_setup setup{};
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_PrivateKey_file(setup.context, setup.key_path.c_str(), SSL_FILETYPE_ASN1), 0);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), ERR_PACK(ERR_LIB_SSL, 0, 0));
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_use_private_key_file__missing__zero_error_queued)
{
    const shim_setup setup{};
    const std::string missing{ TEST_DIRECTORY + "/missing.key" };
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_PrivateKey_file(setup.context, missing.c_str(), SSL_FILETYPE_PEM), 0);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), ERR_PACK(ERR_LIB_SSL, 0, 0));
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_use_private_key_file__mismatched_chain__zero_error_queued)
{
    const shim_setup setup{};
    const std::string other{ TEST_DIRECTORY + "/other.key" };
    write_file(other, x509::encode_private_key(shim_other_secret));
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_certificate_chain_file(setup.context, setup.certificate_path.c_str()), 1);
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_PrivateKey_file(setup.context, other.c_str(), SSL_FILETYPE_PEM), 0);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), ERR_PACK(ERR_LIB_SSL, 0, 0));
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_use_private_key_file__encrypted_password_callback__one)
{
    const shim_setup setup{};
    const std::string encrypted{ TEST_DIRECTORY + "/encrypted.key" };
    write_file(encrypted, shim_encrypted_key);
    auto userdata = shim_password;
    SSL_CTX_set_default_passwd_cb(setup.context, &copy_password);
    SSL_CTX_set_default_passwd_cb_userdata(setup.context, &userdata);
    BOOST_REQUIRE(SSL_CTX_get_default_passwd_cb(setup.context) == &copy_password);
    BOOST_REQUIRE(SSL_CTX_get_default_passwd_cb_userdata(setup.context) == &userdata);
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_PrivateKey_file(setup.context, encrypted.c_str(), SSL_FILETYPE_PEM), 1);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_use_private_key_file__encrypted_wrong_password__zero)
{
    const shim_setup setup{};
    const std::string encrypted{ TEST_DIRECTORY + "/encrypted.key" };
    write_file(encrypted, shim_encrypted_key);
    std::string userdata{ "wrong" };
    SSL_CTX_set_default_passwd_cb(setup.context, &copy_password);
    SSL_CTX_set_default_passwd_cb_userdata(setup.context, &userdata);
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_PrivateKey_file(setup.context, encrypted.c_str(), SSL_FILETYPE_PEM), 0);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_use_private_key_file__encrypted_empty_password__zero)
{
    const shim_setup setup{};
    const std::string encrypted{ TEST_DIRECTORY + "/encrypted.key" };
    write_file(encrypted, shim_encrypted_key);
    std::string userdata{};
    SSL_CTX_set_default_passwd_cb(setup.context, &copy_password);
    SSL_CTX_set_default_passwd_cb_userdata(setup.context, &userdata);
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_PrivateKey_file(setup.context, encrypted.c_str(), SSL_FILETYPE_PEM), 0);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_use_private_key_file__encrypted_no_callback__zero)
{
    const shim_setup setup{};
    const std::string encrypted{ TEST_DIRECTORY + "/encrypted.key" };
    write_file(encrypted, shim_encrypted_key);
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_PrivateKey_file(setup.context, encrypted.c_str(), SSL_FILETYPE_PEM), 0);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ctx_credential_objects__unsupported)
{
    const shim_setup setup{};
    const uint8_t data{};
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_certificate(setup.context, nullptr), 0);
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_certificate_ASN1(setup.context, 1, &data), 0);
    BOOST_REQUIRE_EQUAL(SSL_CTX_add_extra_chain_cert(setup.context, nullptr), 0);
    BOOST_REQUIRE_EQUAL(SSL_CTX_clear_chain_certs(setup.context), 1);
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_PrivateKey(setup.context, nullptr), 0);
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_RSAPrivateKey(setup.context, nullptr), 0);
    BOOST_REQUIRE_EQUAL(SSL_CTX_use_RSAPrivateKey_file(setup.context, setup.key_path.c_str(), SSL_FILETYPE_PEM), 0);
    BOOST_REQUIRE_EQUAL(SSL_CTX_set_tmp_dh(setup.context, nullptr), 0);
}

// connections

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_new__null_context__null)
{
    BOOST_REQUIRE(is_null(SSL_new(nullptr)));
    SSL_free(nullptr);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_new__context_verify__inherited)
{
    const shim_setup setup{};
    SSL_CTX_set_verify(setup.context, SSL_VERIFY_PEER, &accept_all);
    const auto ssl = SSL_new(setup.context);
    BOOST_REQUIRE(SSL_get_SSL_CTX(ssl) == setup.context);
    BOOST_REQUIRE_EQUAL(SSL_get_verify_mode(ssl), SSL_VERIFY_PEER);
    BOOST_REQUIRE(SSL_get_verify_callback(ssl) == &accept_all);
    SSL_set_verify(ssl, SSL_VERIFY_NONE, nullptr);
    SSL_set_verify_depth(ssl, 4);
    BOOST_REQUIRE_EQUAL(SSL_get_verify_mode(ssl), SSL_VERIFY_NONE);
    BOOST_REQUIRE(is_null(SSL_get_verify_callback(ssl)));
    BOOST_REQUIRE_EQUAL(SSL_CTX_get_verify_mode(setup.context), SSL_VERIFY_PEER);
    SSL_free(ssl);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_properties__new__defaults)
{
    const shim_setup setup{};
    const auto ssl = SSL_new(setup.context);
    BOOST_REQUIRE_EQUAL(SSL_set_mode(ssl, SSL_MODE_ENABLE_PARTIAL_WRITE), SSL_MODE_ENABLE_PARTIAL_WRITE);
    BOOST_REQUIRE_EQUAL(SSL_version(ssl), TLS1_3_VERSION);
    BOOST_REQUIRE_EQUAL(SSL_get_shutdown(ssl), 0);
    BOOST_REQUIRE_EQUAL(SSL_get_verify_result(ssl), X509_V_OK);
    BOOST_REQUIRE(is_null(SSL_get_peer_certificate(ssl)));
    BOOST_REQUIRE_EQUAL(SSL_get_ex_data_X509_STORE_CTX_idx(), 0);
    SSL_free(ssl);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_ex_data__index_zero__only)
{
    const shim_setup setup{};
    const auto ssl = SSL_new(setup.context);
    int value{};
    BOOST_REQUIRE(is_null(SSL_get_app_data(ssl)));
    BOOST_REQUIRE_EQUAL(SSL_set_app_data(ssl, &value), 1);
    BOOST_REQUIRE(SSL_get_app_data(ssl) == &value);
    BOOST_REQUIRE_EQUAL(SSL_set_ex_data(ssl, 1, &value), 0);
    BOOST_REQUIRE(is_null(SSL_get_ex_data(ssl, 1)));
    SSL_free(ssl);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_set_bio__replaced__previous_freed)
{
    const shim_setup setup{};
    const auto ssl = SSL_new(setup.context);
    BIO* first{};
    BIO* first_peer{};
    BIO* second{};
    BIO* second_peer{};
    BOOST_REQUIRE_EQUAL(BIO_new_bio_pair(&first, 0, &first_peer, 0), 1);
    BOOST_REQUIRE_EQUAL(BIO_new_bio_pair(&second, 0, &second_peer, 0), 1);
    SSL_set_bio(ssl, first, first);
    SSL_set_bio(ssl, first, first);
    SSL_set_bio(ssl, second, second);
    SSL_free(ssl);
    BOOST_REQUIRE_EQUAL(BIO_free(first_peer), 1);
    BOOST_REQUIRE_EQUAL(BIO_free(second_peer), 1);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_accept__client_method__internal_error)
{
    const shim_setup setup{};
    const auto context = SSL_CTX_new(TLS_client_method());
    const auto ssl = SSL_new(context);
    BOOST_REQUIRE_EQUAL(SSL_accept(ssl), -1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(ssl, -1), SSL_ERROR_SSL);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), alert_error(tls::alert::internal_error));
    SSL_free(ssl);
    SSL_CTX_free(context);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_accept__no_credentials__internal_error)
{
    const shim_setup setup{};
    const auto ssl = SSL_new(setup.context);
    BOOST_REQUIRE_EQUAL(SSL_accept(ssl), -1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(ssl, -1), SSL_ERROR_SSL);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), alert_error(tls::alert::internal_error));
    SSL_free(ssl);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_accept__no_bio__want_read)
{
    const shim_setup setup{};
    BOOST_REQUIRE(setup.load());
    const auto ssl = SSL_new(setup.context);
    BOOST_REQUIRE_EQUAL(SSL_accept(ssl), -1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(ssl, -1), SSL_ERROR_WANT_READ);
    BOOST_REQUIRE_EQUAL(SSL_read(ssl, nullptr, 0), -1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(ssl, -1), SSL_ERROR_WANT_READ);
    BOOST_REQUIRE_EQUAL(SSL_write(ssl, nullptr, 0), -1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(ssl, -1), SSL_ERROR_SSL);
    BOOST_REQUIRE_EQUAL(SSL_shutdown(ssl), 1);
    SSL_free(ssl);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_connect__client_role__handshake_failure)
{
    const shim_setup setup{};
    const auto ssl = SSL_new(setup.context);
    BOOST_REQUIRE_EQUAL(SSL_connect(ssl), -1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(ssl, -1), SSL_ERROR_SSL);
    BOOST_REQUIRE_EQUAL(SSL_get_error(ssl, 1), SSL_ERROR_NONE);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), alert_error(tls::alert::handshake_failure));
    SSL_free(ssl);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_write__not_established__error)
{
    const shim_setup setup{};
    BOOST_REQUIRE(setup.load());
    const shim_connection connection{ setup.context };
    BOOST_REQUIRE_EQUAL(SSL_accept(connection.ssl), -1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(connection.ssl, -1), SSL_ERROR_WANT_READ);
    const auto data = to_chunk("early");
    BOOST_REQUIRE_EQUAL(SSL_write(connection.ssl, data.data(), 5), -1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(connection.ssl, -1), SSL_ERROR_SSL);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), ERR_PACK(ERR_LIB_SSL, 0, 0));
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_accept__handshake__established)
{
    const shim_setup setup{};
    BOOST_REQUIRE(setup.load());
    const shim_connection connection{ setup.context };
    test::tls_client client{ setup.options(false) };
    BOOST_REQUIRE_EQUAL(connection.handshake(client), 1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(connection.ssl, 1), SSL_ERROR_NONE);
    BOOST_REQUIRE(client.is_established());
    BOOST_REQUIRE(!client.is_requested());
    BOOST_REQUIRE_EQUAL(SSL_accept(connection.ssl), 1);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_accept__no_common_suite__handshake_failure)
{
    const shim_setup setup{};
    BOOST_REQUIRE(setup.load());
    const shim_connection connection{ setup.context };
    auto options = setup.options(false);
    options.suites = { 0x1302 };
    test::tls_client client{ options };
    client.start();
    connection.send(client);
    BOOST_REQUIRE_EQUAL(SSL_accept(connection.ssl), -1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(connection.ssl, -1), SSL_ERROR_SSL);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), alert_error(tls::alert::handshake_failure));
    connection.receive(client);
    BOOST_REQUIRE(client.is_failed());
    BOOST_REQUIRE_EQUAL(client.failure(), tls::alert::handshake_failure);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_accept__trusted_client_certificate__established)
{
    const shim_setup setup{};
    BOOST_REQUIRE(setup.load());
    BOOST_REQUIRE_EQUAL(SSL_CTX_load_verify_locations(setup.context, nullptr, setup.clients_path.c_str()), 1);
    SSL_CTX_set_verify(setup.context, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    const shim_connection connection{ setup.context };
    test::tls_client client{ setup.options(true) };
    BOOST_REQUIRE_EQUAL(connection.handshake(client), 1);
    BOOST_REQUIRE(client.is_requested());
    BOOST_REQUIRE(client.is_established());
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_accept__absent_client_certificate__certificate_required)
{
    const shim_setup setup{};
    BOOST_REQUIRE(setup.load());
    BOOST_REQUIRE_EQUAL(SSL_CTX_load_verify_locations(setup.context, setup.client_path.c_str(), nullptr), 1);
    SSL_CTX_set_verify(setup.context, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    const shim_connection connection{ setup.context };
    test::tls_client client{ setup.options(false) };
    BOOST_REQUIRE_EQUAL(connection.handshake(client), -1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(connection.ssl, -1), SSL_ERROR_SSL);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), alert_error(tls::alert::certificate_required));
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_read_write__established__exchanged)
{
    const shim_setup setup{};
    BOOST_REQUIRE(setup.load());
    const shim_connection connection{ setup.context };
    test::tls_client client{ setup.options(false) };
    BOOST_REQUIRE_EQUAL(connection.handshake(client), 1);

    const auto response = to_chunk("response from the server");
    BOOST_REQUIRE_EQUAL(SSL_write(connection.ssl, response.data(), possible_narrow_sign_cast<int>(response.size())), possible_narrow_sign_cast<int>(response.size()));
    BOOST_REQUIRE_EQUAL(SSL_get_error(connection.ssl, 0), SSL_ERROR_NONE);
    BOOST_REQUIRE_EQUAL(BIO_wpending(connection.internal), BIO_ctrl_pending(connection.external));
    connection.receive(client);
    BOOST_REQUIRE_EQUAL(client.read(), response);

    data_chunk buffer(8);
    BOOST_REQUIRE_EQUAL(SSL_read(connection.ssl, buffer.data(), 8), -1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(connection.ssl, -1), SSL_ERROR_WANT_READ);

    BOOST_REQUIRE(client.write(to_chunk("request!request")));
    connection.send(client);
    BOOST_REQUIRE_EQUAL(SSL_read(connection.ssl, buffer.data(), 8), 8);
    BOOST_REQUIRE_EQUAL(buffer, to_chunk("request!"));
    BOOST_REQUIRE_EQUAL(SSL_read(connection.ssl, buffer.data(), 8), 7);
    BOOST_REQUIRE_EQUAL(data_chunk(buffer.begin(), std::next(buffer.begin(), 7)), to_chunk("request"));
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_read__tampered_record__bad_record_mac)
{
    const shim_setup setup{};
    BOOST_REQUIRE(setup.load());
    const shim_connection connection{ setup.context };
    test::tls_client client{ setup.options(false) };
    BOOST_REQUIRE_EQUAL(connection.handshake(client), 1);

    BOOST_REQUIRE(client.write(to_chunk("data")));
    client.output().back() ^= 0x01;
    connection.send(client);
    data_chunk buffer(8);
    BOOST_REQUIRE_EQUAL(SSL_read(connection.ssl, buffer.data(), 8), -1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(connection.ssl, -1), SSL_ERROR_SSL);
    BOOST_REQUIRE_EQUAL(ERR_get_error(), alert_error(tls::alert::bad_record_mac));
    BOOST_REQUIRE_EQUAL(SSL_write(connection.ssl, buffer.data(), 8), -1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(connection.ssl, -1), SSL_ERROR_SSL);
    BOOST_REQUIRE_EQUAL(SSL_shutdown(connection.ssl), 1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(connection.ssl, 1), SSL_ERROR_NONE);
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_read__peer_close_notify__zero_return)
{
    const shim_setup setup{};
    BOOST_REQUIRE(setup.load());
    const shim_connection connection{ setup.context };
    test::tls_client client{ setup.options(false) };
    BOOST_REQUIRE_EQUAL(connection.handshake(client), 1);

    client.close();
    connection.send(client);
    data_chunk buffer(8);
    BOOST_REQUIRE_EQUAL(SSL_read(connection.ssl, buffer.data(), 8), 0);
    BOOST_REQUIRE_EQUAL(SSL_get_error(connection.ssl, 0), SSL_ERROR_ZERO_RETURN);
    BOOST_REQUIRE_EQUAL(SSL_get_shutdown(connection.ssl), SSL_RECEIVED_SHUTDOWN);
    BOOST_REQUIRE_EQUAL(SSL_shutdown(connection.ssl), 1);
    BOOST_REQUIRE_EQUAL(SSL_get_shutdown(connection.ssl), SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
    connection.receive(client);
    BOOST_REQUIRE(client.is_closed());
}

BOOST_AUTO_TEST_CASE(tls_openssl__ssl_shutdown__server_first__zero_then_one)
{
    const shim_setup setup{};
    BOOST_REQUIRE(setup.load());
    const shim_connection connection{ setup.context };
    test::tls_client client{ setup.options(false) };
    BOOST_REQUIRE_EQUAL(connection.handshake(client), 1);

    BOOST_REQUIRE_EQUAL(SSL_shutdown(connection.ssl), 0);
    BOOST_REQUIRE_EQUAL(SSL_get_shutdown(connection.ssl), SSL_SENT_SHUTDOWN);
    connection.receive(client);
    BOOST_REQUIRE(client.is_closed());
    BOOST_REQUIRE_EQUAL(SSL_shutdown(connection.ssl), -1);
    BOOST_REQUIRE_EQUAL(SSL_get_error(connection.ssl, -1), SSL_ERROR_WANT_READ);

    client.close();
    connection.send(client);
    BOOST_REQUIRE_EQUAL(SSL_shutdown(connection.ssl), 1);
    BOOST_REQUIRE_EQUAL(SSL_get_shutdown(connection.ssl), SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
}

// memory bio pairs and files

BOOST_AUTO_TEST_CASE(tls_openssl__bio_pair__write_read__crossed)
{
    BIO* first{};
    BIO* second{};
    BOOST_REQUIRE_EQUAL(BIO_new_bio_pair(&first, 0, &second, 0), 1);
    std::array<uint8_t, 4> buffer{};
    BOOST_REQUIRE_EQUAL(BIO_read(second, buffer.data(), 4), -1);
    BOOST_REQUIRE_EQUAL(BIO_write(first, "abcdef", 6), 6);
    BOOST_REQUIRE_EQUAL(BIO_wpending(first), 6u);
    BOOST_REQUIRE_EQUAL(BIO_ctrl_pending(first), 0u);
    BOOST_REQUIRE_EQUAL(BIO_ctrl_pending(second), 6u);
    BOOST_REQUIRE_EQUAL(BIO_read(second, buffer.data(), 4), 4);
    BOOST_REQUIRE_EQUAL(to_chunk(buffer), to_chunk("abcd"));
    BOOST_REQUIRE_EQUAL(BIO_read(second, buffer.data(), 4), 2);
    BOOST_REQUIRE_EQUAL(BIO_ctrl_pending(second), 0u);
    BOOST_REQUIRE_EQUAL(BIO_write(second, "xy", 2), 2);
    BOOST_REQUIRE_EQUAL(BIO_ctrl_pending(first), 2u);
    BOOST_REQUIRE_EQUAL(BIO_free(first), 1);
    BOOST_REQUIRE_EQUAL(BIO_free(second), 1);
}

BOOST_AUTO_TEST_CASE(tls_openssl__bio_new__file_and_memory__unsupported)
{
    const shim_setup setup{};
    BOOST_REQUIRE(is_null(BIO_new_file(setup.certificate_path.c_str(), "r")));
    BOOST_REQUIRE(is_null(BIO_new_mem_buf("text", 4)));
}

// error queue

BOOST_AUTO_TEST_CASE(tls_openssl__err_queue__two_errors__fifo)
{
    ERR_clear_error();
    BOOST_REQUIRE_EQUAL(ERR_get_error(), 0u);
    BOOST_REQUIRE_EQUAL(ERR_peek_error(), 0u);
    BOOST_REQUIRE_EQUAL(ERR_peek_last_error(), 0u);

    const shim_setup setup{};
    const auto ssl = SSL_new(setup.context);
    SSL_connect(ssl);
    SSL_accept(ssl);
    BOOST_REQUIRE_EQUAL(ERR_peek_error(), alert_error(tls::alert::handshake_failure));
    BOOST_REQUIRE_EQUAL(ERR_peek_last_error(), alert_error(tls::alert::internal_error));
    BOOST_REQUIRE_EQUAL(ERR_get_error(), alert_error(tls::alert::handshake_failure));
    BOOST_REQUIRE_EQUAL(ERR_get_error(), alert_error(tls::alert::internal_error));
    BOOST_REQUIRE_EQUAL(ERR_get_error(), 0u);
    SSL_connect(ssl);
    ERR_clear_error();
    BOOST_REQUIRE_EQUAL(ERR_peek_error(), 0u);
    SSL_free(ssl);
}

BOOST_AUTO_TEST_CASE(tls_openssl__err_lib_error_string__ssl__ssl_routines)
{
    BOOST_REQUIRE_EQUAL(std::string{ ERR_lib_error_string(ERR_PACK(ERR_LIB_SSL, 0, 0)) }, "SSL routines");
    BOOST_REQUIRE(is_null(ERR_lib_error_string(ERR_PACK(ERR_LIB_SYS, 0, 0))));
    BOOST_REQUIRE(is_null(ERR_func_error_string(ERR_PACK(ERR_LIB_SSL, 0, 0))));
}

BOOST_AUTO_TEST_CASE(tls_openssl__err_reason_error_string__non_alert__tls_failure)
{
    BOOST_REQUIRE(is_null(ERR_reason_error_string(0)));
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(ERR_PACK(ERR_LIB_SSL, 0, 0)) }, "tls failure");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(ERR_PACK(ERR_LIB_SSL, 0, SSL_R_SHORT_READ)) }, "tls failure");
}

BOOST_AUTO_TEST_CASE(tls_openssl__err_reason_error_string__alerts__alert_names)
{
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::unexpected_message)) }, "tls alert unexpected message");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::bad_record_mac)) }, "tls alert bad record mac");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::record_overflow)) }, "tls alert record overflow");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::handshake_failure)) }, "tls alert handshake failure");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::bad_certificate)) }, "tls alert bad certificate");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::unsupported_certificate)) }, "tls alert unsupported certificate");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::certificate_expired)) }, "tls alert certificate expired");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::certificate_unknown)) }, "tls alert certificate unknown");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::illegal_parameter)) }, "tls alert illegal parameter");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::unknown_ca)) }, "tls alert unknown ca");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::decode_error)) }, "tls alert decode error");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::decrypt_error)) }, "tls alert decrypt error");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::protocol_version)) }, "tls alert protocol version");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::internal_error)) }, "tls alert internal error");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::missing_extension)) }, "tls alert missing extension");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::certificate_required)) }, "tls alert certificate required");
    BOOST_REQUIRE_EQUAL(std::string{ ERR_reason_error_string(alert_error(tls::alert::user_canceled)) }, "tls alert alert 90");
}

BOOST_AUTO_TEST_CASE(tls_openssl__err_error_string_n__sizes__truncated_terminated)
{
    const auto code = alert_error(tls::alert::handshake_failure);
    BOOST_REQUIRE_EQUAL(error_string(code, 64), "tls alert handshake failure");
    BOOST_REQUIRE_EQUAL(error_string(code, 8), "tls ale");
    BOOST_REQUIRE_EQUAL(error_string(code, 1), "");
    BOOST_REQUIRE_EQUAL(error_string(0, 8), "");
    char unchanged{ 'x' };
    ERR_error_string_n(code, &unchanged, 0);
    BOOST_REQUIRE_EQUAL(unchanged, 'x');
}

// keys, certificates and verification stubs

BOOST_AUTO_TEST_CASE(tls_openssl__object_stubs__unsupported__null)
{
    BOOST_REQUIRE(is_null(PEM_read_bio_X509(nullptr, nullptr, nullptr, nullptr)));
    BOOST_REQUIRE(is_null(PEM_read_bio_X509_AUX(nullptr, nullptr, nullptr, nullptr)));
    BOOST_REQUIRE(is_null(PEM_read_bio_PrivateKey(nullptr, nullptr, nullptr, nullptr)));
    BOOST_REQUIRE(is_null(PEM_read_bio_RSAPrivateKey(nullptr, nullptr, nullptr, nullptr)));
    BOOST_REQUIRE(is_null(PEM_read_bio_DHparams(nullptr, nullptr, nullptr, nullptr)));
    BOOST_REQUIRE(is_null(d2i_PrivateKey_bio(nullptr, nullptr)));
    BOOST_REQUIRE(is_null(d2i_RSAPrivateKey_bio(nullptr, nullptr)));
    X509_free(nullptr);
    EVP_PKEY_free(nullptr);
    RSA_free(nullptr);
    DH_free(nullptr);
    sk_X509_pop_free(nullptr, &X509_free);
}

BOOST_AUTO_TEST_CASE(tls_openssl__verification_stubs__unsupported__empty)
{
    BOOST_REQUIRE(is_null(X509_STORE_CTX_get_ex_data(nullptr, 0)));
    BOOST_REQUIRE(is_null(X509_STORE_CTX_get_current_cert(nullptr)));
    BOOST_REQUIRE_EQUAL(X509_STORE_CTX_get_error_depth(nullptr), 0);
    BOOST_REQUIRE_EQUAL(X509_check_host(nullptr, "localhost", 9, 0, nullptr), 0);
    BOOST_REQUIRE_EQUAL(X509_check_ip_asc(nullptr, "127.0.0.1", 0), 0);
}

BOOST_AUTO_TEST_SUITE_END()
