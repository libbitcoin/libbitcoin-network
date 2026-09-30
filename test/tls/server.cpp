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
#include "rfc8448.hpp"

BOOST_AUTO_TEST_SUITE(tls_server_tests)

using namespace bc::system;
using namespace tls;
using namespace test;

constexpr uint64_t now = 1767225600;
const auto server_key = base16_array("c9afa9d845ba75166b5c215767b1d6934e50c3db36e89b127b8a622b120f6721");
const auto client_key = base16_array("0000000000000000000000000000000000000000000000000000000000000002");
const auto other_key = base16_array("0000000000000000000000000000000000000000000000000000000000000003");

// A self-signed secp384r1 client identity (openssl).
const auto client384_key = base16_array("6bfc6ce1ef1beb42ecb60360242e9efab64189b1277c562b58eedad3b3586ff590179425f623bd9532ebd3091eb3cbb5");
const std::string client384_chain
{
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB6zCCAXGgAwIBAgIUJtHsAFXdYgGYBXLxCgPAvtfK/PcwCgYIKoZIzj0EAwMw\n"
    "FDESMBAGA1UEAwwJY2xpZW50Mzg0MB4XDTI2MDEwMTAwMDAwMFoXDTQ5MTIzMTIz\n"
    "NTk1OVowFDESMBAGA1UEAwwJY2xpZW50Mzg0MHYwEAYHKoZIzj0CAQYFK4EEACID\n"
    "YgAECOQks2KFZdRtQWoliPUFV9bhyFan2z1R+sutT7q3fTsTHK2fUZxA9HZvriMZ\n"
    "HEKEIzdITbaBVXfFUnhH5W3UiU2P5CmOC6vwREAgpthnG8QVxJ+RqT7snfceDa7T\n"
    "l28io4GDMIGAMB0GA1UdDgQWBBTJQ+ViudEUUbSJ+d34rwXXN4emXjAfBgNVHSME\n"
    "GDAWgBTJQ+ViudEUUbSJ+d34rwXXN4emXjAPBgNVHRMBAf8EBTADAQH/MA4GA1Ud\n"
    "DwEB/wQEAwIChDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwCgYIKoZI\n"
    "zj0EAwMDaAAwZQIxAKw8qvXbX8/kRuCJQPuDpyVj5vHkOQ/T/hGqnJiXihrBvSQc\n"
    "tWKiP+AIuqd+JpCnMgIwZ21W5QpX7QNqcFdoUbWW1BxaLDekhmmKPa6+SIte9H3V\n"
    "6kxQ4IMBYC3p0j3En1DQ\n"
    "-----END CERTIFICATE-----\n"
};

struct identity
{
    std::string chain;
    std::string key;
    x509::certificate certificate;
};

static identity make_identity(const x509::secret& key, const std::string& name)
{
    x509::subject subject{};
    subject.common_name = name;
    subject.dns_names = { "localhost" };
    subject.not_before = 1735689600;
    subject.not_after = 2524608000;

    data_chunk der{};
    x509::build_self_signed(der, key, subject);

    x509::certificate certificate{};
    x509::parse(certificate, der);
    return { x509::encode_certificate(der), x509::encode_private_key(key), certificate };
}

static const identity& server_identity()
{
    static const auto value = make_identity(server_key, "server");
    return value;
}

// A server context with the server identity, optionally requesting a client.
struct server_setup
{
    server_setup(bool request=false, bool require=false)
    {
        context.set_chain(server_identity().chain);
        context.set_key(server_identity().key, {});
        context.set_verify(request, require);
        context.set_time(now);
    }

    tls::context context{};
};

// The secp384r1 identity as a server.
static const identity& server384_identity()
{
    static const auto value = []()
    {
        x509::certificates chain{};
        x509::parse(chain, client384_chain);
        const auto key = x509::encode_private_key(client384_key);
        return identity{ client384_chain, key, chain.front() };
    }();

    return value;
}

// A server context with the secp384r1 identity.
struct server384_setup
{
    server384_setup()
    {
        context.set_chain(server384_identity().chain);
        context.set_key(server384_identity().key, {});
        context.set_time(now);
    }

    tls::context context{};
};

static tls_client::options client_options()
{
    tls_client::options value{};
    value.anchors = { server_identity().certificate };
    value.time = now;
    return value;
}

// Move bytes between the peers until neither has output.
static void exchange(tls_client& client, tls::server& server)
{
    while (!client.output().empty() || !server.output().empty())
    {
        const auto to_server = std::move(client.output());
        client.output().clear();
        server.receive(to_server);

        const auto to_client = std::move(server.output());
        server.output().clear();
        client.receive(to_client);
    }
}

static data_chunk read_all(tls::server& server)
{
    data_chunk out(server.readable());
    server.read(out);
    return out;
}

// handshake

BOOST_AUTO_TEST_CASE(tls_server__handshake__default__established_aes)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    tls_client client{ client_options() };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(client.is_established());
    BOOST_REQUIRE(server.is_established());
    BOOST_REQUIRE_EQUAL(server.suite(), aes_128_gcm_sha256);
    BOOST_REQUIRE_EQUAL(client.suite(), aes_128_gcm_sha256);
    BOOST_REQUIRE(!client.is_retried());
    BOOST_REQUIRE(server.peer().empty());
}

BOOST_AUTO_TEST_CASE(tls_server__handshake__chacha_only__established_chacha)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    auto options = client_options();
    options.suites = { chacha20_poly1305_sha256 };
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(server.is_established());
    BOOST_REQUIRE_EQUAL(server.suite(), chacha20_poly1305_sha256);
}

BOOST_AUTO_TEST_CASE(tls_server__handshake__no_key_share__retried_established)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    auto options = client_options();
    options.shares = {};
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(client.is_retried());
    BOOST_REQUIRE(client.is_established());
    BOOST_REQUIRE(server.is_established());
}

BOOST_AUTO_TEST_CASE(tls_server__handshake__compatibility_session__established)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    auto options = client_options();
    options.session = 32;
    options.shares = {};
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(client.is_established());
    BOOST_REQUIRE(server.is_established());
}

BOOST_AUTO_TEST_CASE(tls_server__handshake__no_common_suite__handshake_failure)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    auto options = client_options();
    options.suites = { 0x1304 };
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(server.is_failed());
    BOOST_REQUIRE_EQUAL(server.failure(), alert::handshake_failure);
    BOOST_REQUIRE(!server.is_failure_received());
    BOOST_REQUIRE(client.is_failed());
    BOOST_REQUIRE_EQUAL(client.failure(), alert::handshake_failure);
}

// suites, groups and signature schemes

BOOST_AUTO_TEST_CASE(tls_server__handshake__aes256_only__established_aes256)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    auto options = client_options();
    options.suites = { aes_256_gcm_sha384 };
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(server.is_established());
    BOOST_REQUIRE_EQUAL(server.suite(), aes_256_gcm_sha384);

    const auto request = to_chunk("request");
    BOOST_REQUIRE(client.write(request));
    exchange(client, server);
    BOOST_REQUIRE_EQUAL(read_all(server), request);

    const data_chunk response(40000, 0x42);
    BOOST_REQUIRE(server.write(response));
    exchange(client, server);
    BOOST_REQUIRE_EQUAL(client.read(), response);
}

BOOST_AUTO_TEST_CASE(tls_server__handshake__aes256_retried__established_aes256)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    auto options = client_options();
    options.suites = { aes_256_gcm_sha384 };
    options.shares = {};
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(client.is_retried());
    BOOST_REQUIRE(server.is_established());
    BOOST_REQUIRE_EQUAL(server.suite(), aes_256_gcm_sha384);
}

BOOST_AUTO_TEST_CASE(tls_server__handshake__secp256r1_share__established_secp256r1)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    auto options = client_options();
    options.groups = { secp256r1_group };
    options.shares = { secp256r1_group };
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(!client.is_retried());
    BOOST_REQUIRE(server.is_established());
    BOOST_REQUIRE_EQUAL(server.group(), secp256r1_group);
}

BOOST_AUTO_TEST_CASE(tls_server__handshake__secp384r1_share__established_secp384r1)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    auto options = client_options();
    options.groups = { secp384r1_group };
    options.shares = { secp384r1_group };
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(!client.is_retried());
    BOOST_REQUIRE(server.is_established());
    BOOST_REQUIRE_EQUAL(server.group(), secp384r1_group);
}

BOOST_AUTO_TEST_CASE(tls_server__handshake__nist_groups_no_share__retried_secp256r1)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    auto options = client_options();
    options.groups = { secp384r1_group, secp256r1_group };
    options.shares = {};
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(client.is_retried());
    BOOST_REQUIRE(server.is_established());
    BOOST_REQUIRE_EQUAL(server.group(), secp256r1_group);
}

BOOST_AUTO_TEST_CASE(tls_server__handshake__share_of_less_preferred_group__not_retried)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    auto options = client_options();
    options.groups = { x25519_group, secp384r1_group };
    options.shares = { secp384r1_group };
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(!client.is_retried());
    BOOST_REQUIRE(server.is_established());
    BOOST_REQUIRE_EQUAL(server.group(), secp384r1_group);
}

BOOST_AUTO_TEST_CASE(tls_server__handshake__secp384r1_server__established)
{
    const server384_setup setup{};
    tls::server server{ setup.context };
    auto options = client_options();
    options.anchors = { server384_identity().certificate };
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(client.is_established());
    BOOST_REQUIRE(server.is_established());
}

BOOST_AUTO_TEST_CASE(tls_server__handshake__cnsa_client_secp384r1_server__established)
{
    const server384_setup setup{};
    tls::server server{ setup.context };
    auto options = client_options();
    options.anchors = { server384_identity().certificate };
    options.suites = { aes_256_gcm_sha384 };
    options.groups = { secp384r1_group };
    options.shares = { secp384r1_group };
    options.algorithms = { ecdsa_secp384r1_sha384 };
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(client.is_established());
    BOOST_REQUIRE(server.is_established());
    BOOST_REQUIRE_EQUAL(server.suite(), aes_256_gcm_sha384);
    BOOST_REQUIRE_EQUAL(server.group(), secp384r1_group);

    const auto request = to_chunk("request");
    BOOST_REQUIRE(client.write(request));
    exchange(client, server);
    BOOST_REQUIRE_EQUAL(read_all(server), request);
}

BOOST_AUTO_TEST_CASE(tls_server__handshake__cnsa_client_secp256r1_server__handshake_failure)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    auto options = client_options();
    options.algorithms = { ecdsa_secp384r1_sha384 };
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(server.is_failed());
    BOOST_REQUIRE_EQUAL(server.failure(), alert::handshake_failure);
}

BOOST_AUTO_TEST_CASE(tls_server__key_update__aes256__both_keys_updated)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    auto options = client_options();
    options.suites = { aes_256_gcm_sha384 };
    tls_client client{ options };
    client.start();
    exchange(client, server);

    client.update(true);
    exchange(client, server);

    const auto request = to_chunk("after update");
    BOOST_REQUIRE(client.write(request));
    exchange(client, server);
    BOOST_REQUIRE_EQUAL(read_all(server), request);

    BOOST_REQUIRE(server.write(request));
    exchange(client, server);
    BOOST_REQUIRE_EQUAL(client.read(), request);
}

// application data, key update, close

BOOST_AUTO_TEST_CASE(tls_server__application_data__both_directions__expected)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    tls_client client{ client_options() };
    client.start();
    exchange(client, server);

    const auto request = to_chunk("GET / HTTP/1.1\r\n\r\n");
    BOOST_REQUIRE(client.write(request));
    exchange(client, server);
    BOOST_REQUIRE_EQUAL(read_all(server), request);

    const data_chunk response(40000, 0x42);
    BOOST_REQUIRE(server.write(response));
    exchange(client, server);
    BOOST_REQUIRE_EQUAL(client.read(), response);
}

BOOST_AUTO_TEST_CASE(tls_server__key_update__requested__both_keys_updated)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    tls_client client{ client_options() };
    client.start();
    exchange(client, server);

    client.update(true);
    exchange(client, server);

    const auto request = to_chunk("after update");
    BOOST_REQUIRE(client.write(request));
    exchange(client, server);
    BOOST_REQUIRE_EQUAL(read_all(server), request);

    BOOST_REQUIRE(server.write(request));
    exchange(client, server);
    BOOST_REQUIRE_EQUAL(client.read(), request);
}

BOOST_AUTO_TEST_CASE(tls_server__close__both_sides__closed)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    tls_client client{ client_options() };
    client.start();
    exchange(client, server);

    client.close();
    exchange(client, server);
    BOOST_REQUIRE(server.is_closed());

    server.close();
    exchange(client, server);
    BOOST_REQUIRE(server.is_close_sent());
    BOOST_REQUIRE(client.is_closed());
    BOOST_REQUIRE(!server.write(to_chunk("late")));
}

// failures

BOOST_AUTO_TEST_CASE(tls_server__receive__application_data_before_handshake__unexpected_message)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    BOOST_REQUIRE(!server.receive(base16_chunk("170303000100")));
    BOOST_REQUIRE_EQUAL(server.failure(), alert::unexpected_message);
    BOOST_REQUIRE_EQUAL(server.output(), base16_chunk("1503030002020a"));
}

BOOST_AUTO_TEST_CASE(tls_server__receive__oversized_record__record_overflow)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    BOOST_REQUIRE(!server.receive(base16_chunk("1603034101")));
    BOOST_REQUIRE_EQUAL(server.failure(), alert::record_overflow);
}

BOOST_AUTO_TEST_CASE(tls_server__receive__hello_without_extensions__protocol_version)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    const auto hello = base16_chunk("010000290303" "0000000000000000000000000000000000000000000000000000000000000000" "00" "0002c02f" "0100");
    const auto record = splice(base16_chunk("16030300" "2d"), hello);
    BOOST_REQUIRE(!server.receive(record));
    BOOST_REQUIRE_EQUAL(server.failure(), alert::protocol_version);
}

BOOST_AUTO_TEST_CASE(tls_server__receive__tampered_record__bad_record_mac)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    tls_client client{ client_options() };
    client.start();
    exchange(client, server);

    BOOST_REQUIRE(client.write(to_chunk("data")));
    auto record = std::move(client.output());
    record.back() ^= 0x01;
    BOOST_REQUIRE(!server.receive(record));
    BOOST_REQUIRE_EQUAL(server.failure(), alert::bad_record_mac);
}

BOOST_AUTO_TEST_CASE(tls_server__receive__peer_alert__failure_received)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    BOOST_REQUIRE(!server.receive(base16_chunk("15030300020228")));
    BOOST_REQUIRE(server.is_failed());
    BOOST_REQUIRE(server.is_failure_received());
    BOOST_REQUIRE_EQUAL(server.failure(), alert::handshake_failure);
    BOOST_REQUIRE(server.output().empty());
}

BOOST_AUTO_TEST_CASE(tls_server__receive__split_records__established)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    tls_client client{ client_options() };
    client.start();

    const auto hello = std::move(client.output());
    client.output().clear();
    const auto half = hello.size() / 2u;
    BOOST_REQUIRE(server.receive({ hello.data(), half }));
    BOOST_REQUIRE(server.output().empty());
    BOOST_REQUIRE(server.receive({ std::next(hello.data(), half), hello.size() - half }));
    BOOST_REQUIRE(!server.output().empty());

    exchange(client, server);
    BOOST_REQUIRE(server.is_established());
}

// client authentication

BOOST_AUTO_TEST_CASE(tls_server__receive__user_canceled__not_failed)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    BOOST_REQUIRE(server.receive(base16_chunk("1503030002015a")));
    BOOST_REQUIRE(!server.is_failed());
    BOOST_REQUIRE(!server.is_closed());
    BOOST_REQUIRE(server.output().empty());
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate__trusted__peer_verified)
{
    const auto client_identity = make_identity(client_key, "client");
    server_setup setup{ true, true };
    setup.context.add_anchors(client_identity.chain);
    tls::server server{ setup.context };

    auto options = client_options();
    options.chain = { client_identity.certificate.encoding };
    options.key = client_key;
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(client.is_requested());
    BOOST_REQUIRE(server.is_established());
    BOOST_REQUIRE_EQUAL(server.peer().size(), 1u);
    BOOST_REQUIRE_EQUAL(server.peer().front().encoding, client_identity.certificate.encoding);
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate__secp384r1_trusted__peer_verified)
{
    x509::certificates chain{};
    BOOST_REQUIRE(x509::parse(chain, client384_chain));
    server_setup setup{ true, true };
    BOOST_REQUIRE(setup.context.add_anchors(client384_chain));
    tls::server server{ setup.context };

    auto options = client_options();
    options.chain = { chain.front().encoding };
    options.key384 = client384_key;
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(client.is_requested());
    BOOST_REQUIRE(server.is_established());
    BOOST_REQUIRE_EQUAL(server.peer().size(), 1u);
    BOOST_REQUIRE(server.peer().front().curve == x509::curve::secp384r1);
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate__required_absent__certificate_required)
{
    server_setup setup{ true, true };
    tls::server server{ setup.context };
    tls_client client{ client_options() };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(server.is_failed());
    BOOST_REQUIRE_EQUAL(server.failure(), alert::certificate_required);
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate__optional_absent__established)
{
    server_setup setup{ true, false };
    tls::server server{ setup.context };
    tls_client client{ client_options() };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(server.is_established());
    BOOST_REQUIRE(server.peer().empty());
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate__untrusted__unknown_ca)
{
    const auto client_identity = make_identity(client_key, "client");
    const auto other_identity = make_identity(other_key, "other");
    server_setup setup{ true, true };
    setup.context.add_anchors(other_identity.chain);
    tls::server server{ setup.context };

    auto options = client_options();
    options.chain = { client_identity.certificate.encoding };
    options.key = client_key;
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(server.is_failed());
    BOOST_REQUIRE_EQUAL(server.failure(), alert::unknown_ca);
}

// rfc8448

BOOST_AUTO_TEST_CASE(tls_server__receive__rfc8448_client_hello__rfc8448_server_hello_and_keys)
{
    const server_setup setup{};
    const auto random = rfc8448::array<32>(data_chunk(std::next(rfc8448::server_hello.begin(), 6), std::next(rfc8448::server_hello.begin(), 38)));
    const auto ephemeral = rfc8448::array<x25519::key_size>(rfc8448::server_private);
    tls::server server{ setup.context, random, ephemeral };
    BOOST_REQUIRE(server.receive(rfc8448::client_hello_record));

    const auto& output = server.output();
    const auto size = rfc8448::server_hello_record.size();
    BOOST_REQUIRE_EQUAL(data_chunk(output.begin(), std::next(output.begin(), size)), rfc8448::server_hello_record);

    // The rest is the flight under the rfc8448 server handshake traffic key.
    const data_chunk flight(std::next(output.begin(), size), output.end());
    const data_chunk head(flight.begin(), std::next(flight.begin(), record_header_size));
    const data_chunk tail(std::next(flight.begin(), record_header_size), flight.end());

    record receiver{};
    receiver.set_secret(aes_128_gcm_sha256, rfc8448::server_handshake_traffic);

    uint8_t type{};
    data_chunk content{};
    BOOST_REQUIRE(receiver.open(type, content, head, tail));
    BOOST_REQUIRE_EQUAL(type, content::handshake);
    BOOST_REQUIRE_EQUAL(content.front(), handshake::encrypted_extensions);
}

// protected client messages

using writer = network::tls::writer;

static data_chunk message_of(uint8_t type, const data_chunk& body)
{
    writer out{};
    out.write_8(type);
    out.write_vector_24(body);
    return out.data();
}

static data_chunk entry_of(const data_chunk& der)
{
    writer out{};
    out.write_vector_24(der);
    out.write_vector_16(data_chunk{});
    return out.data();
}

static data_chunk certificate_of(const data_chunk& request_context, const data_chunk& entries)
{
    writer out{};
    out.write_vector_8(request_context);
    out.write_vector_24(entries);
    return out.data();
}

static data_chunk verify_of(uint16_t scheme, const data_chunk& signature)
{
    writer out{};
    out.write_16(scheme);
    out.write_vector_16(signature);
    return out.data();
}

// A server keyed by the rfc8448 client hello, with a client sender under the
// rfc8448 client handshake traffic secret.
struct rfc8448_setup
{
    rfc8448_setup(bool request=false)
      : setup(request, request), server(setup.context, rfc8448::array<32>(data_chunk(std::next(rfc8448::server_hello.begin(), 6), std::next(rfc8448::server_hello.begin(), 38))), rfc8448::array<x25519::key_size>(rfc8448::server_private))
    {
        BOOST_REQUIRE(server.receive(rfc8448::client_hello_record));
        const auto& output = server.output();
        const auto start = rfc8448::server_hello_record.size();
        const data_chunk head(std::next(output.begin(), start), std::next(output.begin(), start + record_header_size));
        const data_chunk tail(std::next(output.begin(), start + record_header_size), output.end());

        record receiver{};
        receiver.set_secret(aes_128_gcm_sha256, rfc8448::server_handshake_traffic);
        uint8_t type{};
        data_chunk flight{};
        BOOST_REQUIRE(receiver.open(type, flight, head, tail));
        server.output().clear();

        transcript = to_chunk(sha256_hash(splice(splice(rfc8448::client_hello, rfc8448::server_hello), flight)));
        sender.set_secret(aes_128_gcm_sha256, rfc8448::client_handshake_traffic);
    }

    bool send(uint8_t type, const data_chunk& content)
    {
        data_chunk out{};
        sender.seal(out, type, content);
        return server.receive(out);
    }

    bool send_message(uint8_t type, const data_chunk& body)
    {
        return send(content::handshake, message_of(type, body));
    }

    bool finish()
    {
        const schedule keys{ aes_128_gcm_sha256 };
        const auto verify = keys.finished(rfc8448::client_handshake_traffic, transcript);
        const auto sent = send_message(handshake::finished, verify);
        const auto master = keys.master_secret(rfc8448::handshake_secret);
        sender.set_secret(aes_128_gcm_sha256, keys.derive_secret(master, "c ap traffic", transcript));
        return sent;
    }

    server_setup setup;
    tls::server server;
    record sender{};
    schedule::secret transcript{};
};

BOOST_AUTO_TEST_CASE(tls_server__receive__protected_finished__established_application_data)
{
    rfc8448_setup instance{};
    BOOST_REQUIRE(instance.finish());
    BOOST_REQUIRE(instance.server.is_established());
    BOOST_REQUIRE(instance.send(content::application_data, to_chunk("data")));
    BOOST_REQUIRE_EQUAL(read_all(instance.server), to_chunk("data"));
}

BOOST_AUTO_TEST_CASE(tls_server__receive__protected_bad_finished__decrypt_error)
{
    rfc8448_setup instance{};
    BOOST_REQUIRE(!instance.send_message(handshake::finished, data_chunk(32, 0x00)));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::decrypt_error);
}

BOOST_AUTO_TEST_CASE(tls_server__receive__protected_short_finished__decrypt_error)
{
    rfc8448_setup instance{};
    BOOST_REQUIRE(!instance.send_message(handshake::finished, data_chunk(31, 0x00)));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::decrypt_error);
}

BOOST_AUTO_TEST_CASE(tls_server__receive__protected_certificate_unrequested__unexpected_message)
{
    rfc8448_setup instance{};
    BOOST_REQUIRE(!instance.send_message(handshake::certificate, certificate_of({}, {})));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::unexpected_message);
}

BOOST_AUTO_TEST_CASE(tls_server__receive__protected_application_data_before_finished__unexpected_message)
{
    rfc8448_setup instance{};
    BOOST_REQUIRE(!instance.send(content::application_data, to_chunk("data")));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::unexpected_message);
}

BOOST_AUTO_TEST_CASE(tls_server__receive__plaintext_handshake_after_keys__unexpected_message)
{
    rfc8448_setup instance{};
    BOOST_REQUIRE(!instance.server.receive(base16_chunk("160303000414000000")));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::unexpected_message);
}

BOOST_AUTO_TEST_CASE(tls_server__receive__key_update_long__decode_error)
{
    rfc8448_setup instance{};
    BOOST_REQUIRE(instance.finish());
    BOOST_REQUIRE(!instance.send_message(handshake::key_update, base16_chunk("0000")));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::decode_error);
}

BOOST_AUTO_TEST_CASE(tls_server__receive__key_update_invalid_request__illegal_parameter)
{
    rfc8448_setup instance{};
    BOOST_REQUIRE(instance.finish());
    BOOST_REQUIRE(!instance.send_message(handshake::key_update, base16_chunk("02")));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::illegal_parameter);
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate__finished_instead__unexpected_message)
{
    rfc8448_setup instance{ true };
    BOOST_REQUIRE(!instance.send_message(handshake::finished, data_chunk(32, 0x00)));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::unexpected_message);
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate__request_context__illegal_parameter)
{
    rfc8448_setup instance{ true };
    BOOST_REQUIRE(!instance.send_message(handshake::certificate, certificate_of(base16_chunk("01"), {})));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::illegal_parameter);
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate__trailing_byte__decode_error)
{
    rfc8448_setup instance{ true };
    BOOST_REQUIRE(!instance.send_message(handshake::certificate, splice(certificate_of({}, {}), base16_chunk("00"))));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::decode_error);
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate__entry_without_extensions__decode_error)
{
    const auto client_identity = make_identity(client_key, "client");
    rfc8448_setup instance{ true };
    writer entry{};
    entry.write_vector_24(client_identity.certificate.encoding);
    BOOST_REQUIRE(!instance.send_message(handshake::certificate, certificate_of({}, entry.data())));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::decode_error);
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate__unparsable__bad_certificate)
{
    rfc8448_setup instance{ true };
    BOOST_REQUIRE(!instance.send_message(handshake::certificate, certificate_of({}, entry_of(base16_chunk("3000")))));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::bad_certificate);
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate__issuer_mismatch__bad_certificate)
{
    const auto client_identity = make_identity(client_key, "client");
    const auto other_identity = make_identity(other_key, "other");
    rfc8448_setup instance{ true };
    instance.setup.context.add_anchors(other_identity.chain);
    const auto entries = splice(entry_of(client_identity.certificate.encoding), entry_of(server_identity().certificate.encoding));
    BOOST_REQUIRE(!instance.send_message(handshake::certificate, certificate_of({}, entries)));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::bad_certificate);
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate__expired__certificate_expired)
{
    x509::subject subject{};
    subject.common_name = "expired";
    subject.not_before = 1735689600;
    subject.not_after = 1750000000;
    data_chunk der{};
    x509::build_self_signed(der, client_key, subject);

    rfc8448_setup instance{ true };
    instance.setup.context.add_anchors(x509::encode_certificate(der));
    BOOST_REQUIRE(!instance.send_message(handshake::certificate, certificate_of({}, entry_of(der))));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::certificate_expired);
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate_verify__finished_instead__unexpected_message)
{
    const auto client_identity = make_identity(client_key, "client");
    rfc8448_setup instance{ true };
    instance.setup.context.add_anchors(client_identity.chain);
    BOOST_REQUIRE(instance.send_message(handshake::certificate, certificate_of({}, entry_of(client_identity.certificate.encoding))));
    BOOST_REQUIRE(!instance.send_message(handshake::finished, data_chunk(32, 0x00)));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::unexpected_message);
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate_verify__trailing_byte__decode_error)
{
    const auto client_identity = make_identity(client_key, "client");
    rfc8448_setup instance{ true };
    instance.setup.context.add_anchors(client_identity.chain);
    BOOST_REQUIRE(instance.send_message(handshake::certificate, certificate_of({}, entry_of(client_identity.certificate.encoding))));
    BOOST_REQUIRE(!instance.send_message(handshake::certificate_verify, splice(verify_of(ecdsa_secp256r1_sha256, data_chunk(8, 0x01)), base16_chunk("00"))));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::decode_error);
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate_verify__scheme_mismatch__illegal_parameter)
{
    const auto client_identity = make_identity(client_key, "client");
    rfc8448_setup instance{ true };
    instance.setup.context.add_anchors(client_identity.chain);
    BOOST_REQUIRE(instance.send_message(handshake::certificate, certificate_of({}, entry_of(client_identity.certificate.encoding))));
    BOOST_REQUIRE(!instance.send_message(handshake::certificate_verify, verify_of(ecdsa_secp384r1_sha384, data_chunk(8, 0x01))));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::illegal_parameter);
}

BOOST_AUTO_TEST_CASE(tls_server__client_certificate_verify__invalid_signature__decrypt_error)
{
    const auto client_identity = make_identity(client_key, "client");
    rfc8448_setup instance{ true };
    instance.setup.context.add_anchors(client_identity.chain);
    BOOST_REQUIRE(instance.send_message(handshake::certificate, certificate_of({}, entry_of(client_identity.certificate.encoding))));
    BOOST_REQUIRE(!instance.send_message(handshake::certificate_verify, verify_of(ecdsa_secp256r1_sha256, data_chunk(8, 0x01))));
    BOOST_REQUIRE_EQUAL(instance.server.failure(), alert::decrypt_error);
}

// close

BOOST_AUTO_TEST_CASE(tls_server__close__failed__no_close_notify)
{
    const server_setup setup{};
    tls::server server{ setup.context };
    BOOST_REQUIRE(!server.receive(base16_chunk("170303000100")));
    const auto output = server.output();
    server.close();
    BOOST_REQUIRE(!server.is_close_sent());
    BOOST_REQUIRE_EQUAL(server.output(), output);
}

BOOST_AUTO_TEST_SUITE_END()
