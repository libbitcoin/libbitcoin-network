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
    options.share = false;
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
    options.share = false;
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
    options.suites = { 0x1302 };
    tls_client client{ options };
    client.start();
    exchange(client, server);
    BOOST_REQUIRE(server.is_failed());
    BOOST_REQUIRE_EQUAL(server.failure(), alert::handshake_failure);
    BOOST_REQUIRE(!server.is_failure_received());
    BOOST_REQUIRE(client.is_failed());
    BOOST_REQUIRE_EQUAL(client.failure(), alert::handshake_failure);
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
    receiver.set_secret(aes_128_gcm_sha256, rfc8448::array<32>(rfc8448::server_handshake_traffic));

    uint8_t type{};
    data_chunk content{};
    BOOST_REQUIRE(receiver.open(type, content, head, tail));
    BOOST_REQUIRE_EQUAL(type, content::handshake);
    BOOST_REQUIRE_EQUAL(content.front(), handshake::encrypted_extensions);
}

BOOST_AUTO_TEST_SUITE_END()
