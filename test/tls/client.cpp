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
#include "fixtures.hpp"

BC_AUTO_TEST_SUITE(tls_client_tests)

using namespace bc::system;
using namespace tls;

const auto client_secret = base16_array("0101010101010101010101010101010101010101010101010101010101010101");
const auto server_secret = base16_array("0202020202020202020202020202020202020202020202020202020202020202");

// Legacy session id of client hello record.
static data_chunk session_of(const data_chunk& record)
{
    tls::reader source{ record };
    source.read_bytes(record_header_size);
    source.read_8();
    source.read_24();
    source.read_16();
    source.read_bytes(sizeof(client::random));
    const auto session = source.read_vector_8();
    return { session.begin(), session.end() };
}

static data_chunk record_of(uint8_t type, const data_chunk& content)
{
    tls::writer out{};
    out.write_8(type);
    out.write_16(legacy_version);
    out.write_vector_16(content);
    return out.data();
}

static data_chunk extension_of(uint16_t type, const data_chunk& data)
{
    tls::writer out{};
    out.write_16(type);
    out.write_vector_16(data);
    return out.data();
}

static data_chunk versions_extension()
{
    return extension_of(extension::supported_versions, base16_chunk("0304"));
}

static data_chunk share_extension(uint16_t group, const data_chunk& key)
{
    tls::writer share{};
    share.write_16(group);
    share.write_vector_16(key);
    return extension_of(extension::key_share, share.data());
}

static data_chunk retry_extension(uint16_t group)
{
    tls::writer share{};
    share.write_16(group);
    return extension_of(extension::key_share, share.data());
}

static data_chunk cookie_extension(const data_chunk& cookie)
{
    tls::writer value{};
    value.write_vector_16(cookie);
    return extension_of(extension::cookie, value.data());
}

// ServerHello, or HelloRetryRequest by its random (4.1.3).
static data_chunk hello_of(const data_chunk& session,
    const data_chunk& extensions, const data_array<32>& random={},
    uint16_t suite=aes_128_gcm_sha256, uint16_t version=legacy_version,
    uint8_t compression=0)
{
    tls::writer body{};
    body.write_16(version);
    body.write_bytes(random);
    body.write_vector_8(session);
    body.write_16(suite);
    body.write_8(compression);
    body.write_vector_16(extensions);
    return message_of(handshake::server_hello, body.data());
}

static data_chunk hello_record(const data_chunk& session,
    const data_chunk& extensions, const data_array<32>& random={},
    uint16_t suite=aes_128_gcm_sha256, uint16_t version=legacy_version,
    uint8_t compression=0)
{
    const auto hello = hello_of(session, extensions, random, suite, version,
        compression);
    return record_of(content::handshake, hello);
}

static data_chunk x25519_share()
{
    return data_chunk(x25519::key_size, 0x09);
}

// Client (fixed x25519) past server hello, sender under server handshake key.
struct server_flight
{
    server_flight()
      : client(setup.context, {}, client_secret)
    {
        const auto hello = std::move(client.output());
        client.output().clear();
        const auto start = std::next(hello.begin(), record_header_size);
        const data_chunk client_hello{ start, hello.end() };

        x25519::key server_public{}, client_public{}, shared{};
        x25519::multiply(server_public, server_secret);
        x25519::multiply(client_public, client_secret);
        x25519::multiply(shared, server_secret, client_public);

        const auto share = share_extension(x25519_group,
            to_chunk(server_public));
        const auto extensions = splice(versions_extension(), share);
        const auto server_hello = hello_of(session_of(hello), extensions);
        BC_REQUIRE(client.receive(record_of(content::handshake,
            server_hello)));

        transcript.reset(aes_128_gcm_sha256);
        transcript.write(client_hello);
        transcript.write(server_hello);

        const schedule keys{ aes_128_gcm_sha256 };
        const auto secret = keys.handshake_secret(keys.early_secret(),
            shared);
        const auto traffic = keys.derive_secret(secret, "s hs traffic",
            transcript.hash());
        sender.set_secret(aes_128_gcm_sha256, traffic);
    }

    bool send(uint8_t type, const data_chunk& body)
    {
        const auto message = message_of(type, body);
        transcript.write(message);

        data_chunk out{};
        sender.seal(out, content::handshake, message);
        return client.receive(out);
    }

    bool send_extensions(const data_chunk& extensions={})
    {
        tls::writer body{};
        body.write_vector_16(extensions);
        return send(handshake::encrypted_extensions, body.data());
    }

    bool send_certificate()
    {
        const auto& der = server_identity().certificate.encoding;
        return send(handshake::certificate, certificate_of({}, entry_of(der)));
    }

    client_setup setup{};
    tls::client client;
    record sender{};
    tls::transcript transcript{};
};

// handshake

BC_AUTO_TEST_CASE(tls_client__handshake__default__established)
{
    const server_setup setup{};
    const client_setup config{};
    tls::server server{ setup.context };
    tls::client client{ config.context };
    exchange(client, server);

    BC_REQUIRE(client.is_established());
    BC_REQUIRE(server.is_established());
    BC_REQUIRE(!client.is_retried());
    BC_REQUIRE(!client.is_requested());
    BC_REQUIRE_EQUAL(client.suite(), aes_128_gcm_sha256);
    BC_REQUIRE_EQUAL(client.group(), x25519_group);
    BC_REQUIRE_EQUAL(client.peer().size(), 1u);
    BC_REQUIRE_EQUAL(client.peer().front().encoding, server_identity().certificate.encoding);
}

BC_AUTO_TEST_CASE(tls_client__handshake__no_session__established)
{
    const server_setup setup{};
    client_setup config{};
    auto offer = config.context.offered();
    offer.session = 0;
    config.context.set_offer(offer);
    tls::server server{ setup.context };
    tls::client client{ config.context };
    exchange(client, server);

    BC_REQUIRE(client.is_established());
    BC_REQUIRE(server.is_established());
}

BC_AUTO_TEST_CASE(tls_client__handshake__server_name__sent_established)
{
    const server_setup setup{};
    client_setup config{};
    config.context.set_name("localhost");
    tls::server server{ setup.context };
    tls::client client{ config.context };
    const auto name = to_chunk("localhost");
    const auto& hello = client.output();
    const auto sent = std::search(hello.begin(), hello.end(), name.begin(), name.end()) != hello.end();
    exchange(client, server);

    BC_REQUIRE(sent);
    BC_REQUIRE(client.is_established());
}

BC_AUTO_TEST_CASE(tls_client__handshake__unanchored__peer_exposed)
{
    const server_setup setup{};
    tls::context context{};
    context.set_unanchored(true);
    tls::server server{ setup.context };
    tls::client client{ context };
    exchange(client, server);

    BC_REQUIRE(client.is_established());
    BC_REQUIRE_EQUAL(client.peer().size(), 1u);
    BC_REQUIRE_EQUAL(client.peer().front().encoding, server_identity().certificate.encoding);
}

BC_AUTO_TEST_CASE(tls_client__handshake__untrusted__unknown_ca)
{
    const server_setup setup{};
    const client_setup config{ make_identity(other_key, "other").chain };
    tls::server server{ setup.context };
    tls::client client{ config.context };
    exchange(client, server);

    BC_REQUIRE(client.is_failed());
    BC_REQUIRE(!client.is_failure_received());
    BC_REQUIRE_EQUAL(client.failure(), alert::unknown_ca);
    BC_REQUIRE(server.is_failure_received());
}

BC_AUTO_TEST_CASE(tls_client__handshake__no_share__retried_established)
{
    const server_setup setup{};
    client_setup config{};
    auto offer = config.context.offered();
    offer.shares = {};
    config.context.set_offer(offer);
    tls::server server{ setup.context };
    tls::client client{ config.context };
    exchange(client, server);

    BC_REQUIRE(client.is_retried());
    BC_REQUIRE(client.is_established());
    BC_REQUIRE_EQUAL(client.group(), x25519_group);
}

BC_AUTO_TEST_CASE(tls_client__handshake__requested_without_certificate__established)
{
    const server_setup setup{ true, false };
    const client_setup config{};
    tls::server server{ setup.context };
    tls::client client{ config.context };
    exchange(client, server);

    BC_REQUIRE(client.is_requested());
    BC_REQUIRE(client.is_established());
    BC_REQUIRE(server.peer().empty());
}

BC_AUTO_TEST_CASE(tls_client__key_update__server_requested__both_keys_updated)
{
    const server_setup setup{};
    const client_setup config{};
    tls::server server{ setup.context };
    tls::client client{ config.context };
    exchange(client, server);

    const auto updated = server.update(true);
    exchange(client, server);

    const auto request = to_chunk("after update");
    const auto written = client.write(request) && server.write(request);
    exchange(client, server);

    BC_REQUIRE(updated);
    BC_REQUIRE(written);
    BC_REQUIRE_EQUAL(read_all(server), request);
    BC_REQUIRE_EQUAL(read_all(client), request);
}

// server hello

BC_AUTO_TEST_CASE(tls_client__server_hello__session_mismatch__illegal_parameter)
{
    const client_setup config{};
    tls::client client{ config.context };
    const auto extensions = splice(versions_extension(), share_extension(x25519_group, x25519_share()));
    BC_REQUIRE(!client.receive(hello_record(base16_chunk("00"), extensions)));
    BC_REQUIRE_EQUAL(client.failure(), alert::illegal_parameter);
}

BC_AUTO_TEST_CASE(tls_client__server_hello__unoffered_suite__illegal_parameter)
{
    const client_setup config{};
    tls::client client{ config.context };
    const auto session = session_of(client.output());
    const auto extensions = splice(versions_extension(), share_extension(x25519_group, x25519_share()));
    BC_REQUIRE(!client.receive(hello_record(session, extensions, {}, 0x1304)));
    BC_REQUIRE_EQUAL(client.failure(), alert::illegal_parameter);
}

BC_AUTO_TEST_CASE(tls_client__server_hello__compression__illegal_parameter)
{
    const client_setup config{};
    tls::client client{ config.context };
    const auto session = session_of(client.output());
    const auto extensions = splice(versions_extension(), share_extension(x25519_group, x25519_share()));
    BC_REQUIRE(!client.receive(hello_record(session, extensions, {}, aes_128_gcm_sha256, legacy_version, 1)));
    BC_REQUIRE_EQUAL(client.failure(), alert::illegal_parameter);
}

BC_AUTO_TEST_CASE(tls_client__server_hello__legacy_version__protocol_version)
{
    const client_setup config{};
    tls::client client{ config.context };
    const auto session = session_of(client.output());
    const auto extensions = splice(versions_extension(), share_extension(x25519_group, x25519_share()));
    BC_REQUIRE(!client.receive(hello_record(session, extensions, {}, aes_128_gcm_sha256, 0x0302)));
    BC_REQUIRE_EQUAL(client.failure(), alert::protocol_version);
}

BC_AUTO_TEST_CASE(tls_client__server_hello__without_supported_versions__protocol_version)
{
    const client_setup config{};
    tls::client client{ config.context };
    const auto session = session_of(client.output());
    const auto extensions = share_extension(x25519_group, x25519_share());
    BC_REQUIRE(!client.receive(hello_record(session, extensions)));
    BC_REQUIRE_EQUAL(client.failure(), alert::protocol_version);
}

BC_AUTO_TEST_CASE(tls_client__server_hello__unsupported_extension__unsupported_extension)
{
    const client_setup config{};
    tls::client client{ config.context };
    const auto session = session_of(client.output());
    const auto extensions = build_chunk({ versions_extension(), share_extension(x25519_group, x25519_share()), extension_of(extension::server_name, {}) });
    BC_REQUIRE(!client.receive(hello_record(session, extensions)));
    BC_REQUIRE_EQUAL(client.failure(), alert::unsupported_extension);
}

BC_AUTO_TEST_CASE(tls_client__server_hello__duplicate_extension__illegal_parameter)
{
    const client_setup config{};
    tls::client client{ config.context };
    const auto session = session_of(client.output());
    const auto extensions = build_chunk({ versions_extension(), versions_extension(), share_extension(x25519_group, x25519_share()) });
    BC_REQUIRE(!client.receive(hello_record(session, extensions)));
    BC_REQUIRE_EQUAL(client.failure(), alert::illegal_parameter);
}

BC_AUTO_TEST_CASE(tls_client__server_hello__unshared_group__illegal_parameter)
{
    const client_setup config{};
    tls::client client{ config.context };
    const auto session = session_of(client.output());
    const auto extensions = splice(versions_extension(), share_extension(secp256r1_group, x25519_share()));
    BC_REQUIRE(!client.receive(hello_record(session, extensions)));
    BC_REQUIRE_EQUAL(client.failure(), alert::illegal_parameter);
}

BC_AUTO_TEST_CASE(tls_client__server_hello__without_key_share__missing_extension)
{
    const client_setup config{};
    tls::client client{ config.context };
    const auto session = session_of(client.output());
    BC_REQUIRE(!client.receive(hello_record(session, versions_extension())));
    BC_REQUIRE_EQUAL(client.failure(), alert::missing_extension);
}

BC_AUTO_TEST_CASE(tls_client__server_hello__truncated__decode_error)
{
    const client_setup config{};
    tls::client client{ config.context };
    const auto record = record_of(content::handshake, message_of(handshake::server_hello, base16_chunk("0303")));
    BC_REQUIRE(!client.receive(record));
    BC_REQUIRE_EQUAL(client.failure(), alert::decode_error);
}

// hello retry request

BC_AUTO_TEST_CASE(tls_client__retry__shared_group__illegal_parameter)
{
    const client_setup config{};
    tls::client client{ config.context };
    const auto session = session_of(client.output());
    const auto extensions = splice(versions_extension(), retry_extension(x25519_group));
    BC_REQUIRE(!client.receive(hello_record(session, extensions, retry_random)));
    BC_REQUIRE_EQUAL(client.failure(), alert::illegal_parameter);
}

BC_AUTO_TEST_CASE(tls_client__retry__unoffered_group__illegal_parameter)
{
    client_setup config{};
    auto offer = config.context.offered();
    offer.groups = { x25519_group };
    config.context.set_offer(offer);
    tls::client client{ config.context };
    const auto session = session_of(client.output());
    const auto extensions = splice(versions_extension(), retry_extension(secp256r1_group));
    BC_REQUIRE(!client.receive(hello_record(session, extensions, retry_random)));
    BC_REQUIRE_EQUAL(client.failure(), alert::illegal_parameter);
}

BC_AUTO_TEST_CASE(tls_client__retry__unchanged__illegal_parameter)
{
    const client_setup config{};
    tls::client client{ config.context };
    const auto session = session_of(client.output());
    BC_REQUIRE(!client.receive(hello_record(session, versions_extension(), retry_random)));
    BC_REQUIRE_EQUAL(client.failure(), alert::illegal_parameter);
}

BC_AUTO_TEST_CASE(tls_client__retry__twice__unexpected_message)
{
    const client_setup config{};
    tls::client client{ config.context };
    const auto session = session_of(client.output());
    const auto first = splice(versions_extension(), retry_extension(secp256r1_group));
    const auto second = splice(versions_extension(), retry_extension(secp384r1_group));
    BC_REQUIRE(client.receive(hello_record(session, first, retry_random)));
    BC_REQUIRE(!client.receive(hello_record(session, second, retry_random)));
    BC_REQUIRE_EQUAL(client.failure(), alert::unexpected_message);
}

BC_AUTO_TEST_CASE(tls_client__retry__cookie__echoed)
{
    const client_setup config{};
    tls::client client{ config.context };
    const auto session = session_of(client.output());
    client.output().clear();
    const auto cookie = base16_chunk("c00c1e");
    const auto extensions = build_chunk({ versions_extension(), retry_extension(secp256r1_group), cookie_extension(cookie) });
    const auto received = client.receive(hello_record(session, extensions, retry_random));
    const auto& hello = client.output();
    const auto echoed = std::search(hello.begin(), hello.end(), cookie.begin(), cookie.end()) != hello.end();

    BC_REQUIRE(received);
    BC_REQUIRE(echoed);
    BC_REQUIRE(client.is_retried());
}

BC_AUTO_TEST_CASE(tls_client__server_hello__cookie__unsupported_extension)
{
    const client_setup config{};
    tls::client client{ config.context };
    const auto session = session_of(client.output());
    const auto extensions = build_chunk({ versions_extension(), share_extension(x25519_group, x25519_share()), cookie_extension(base16_chunk("c00c1e")) });
    BC_REQUIRE(!client.receive(hello_record(session, extensions)));
    BC_REQUIRE_EQUAL(client.failure(), alert::unsupported_extension);
}

// encrypted server flight

BC_AUTO_TEST_CASE(tls_client__encrypted_extensions__server_groups__accepted)
{
    server_flight flight{};
    tls::writer groups{};
    groups.write_vector_16(base16_chunk("001d"));
    BC_REQUIRE(flight.send_extensions(extension_of(extension::supported_groups, groups.data())));
    BC_REQUIRE(!flight.client.is_failed());
}

BC_AUTO_TEST_CASE(tls_client__encrypted_extensions__unrequested__unsupported_extension)
{
    server_flight flight{};
    BC_REQUIRE(!flight.send_extensions(extension_of(16, base16_chunk("0000"))));
    BC_REQUIRE_EQUAL(flight.client.failure(), alert::unsupported_extension);
}

BC_AUTO_TEST_CASE(tls_client__encrypted_extensions__unsent_server_name__unsupported_extension)
{
    server_flight flight{};
    BC_REQUIRE(!flight.send_extensions(extension_of(extension::server_name, {})));
    BC_REQUIRE_EQUAL(flight.client.failure(), alert::unsupported_extension);
}

BC_AUTO_TEST_CASE(tls_client__certificate__empty__decode_error)
{
    server_flight flight{};
    BC_REQUIRE(flight.send_extensions());
    BC_REQUIRE(!flight.send(handshake::certificate, certificate_of({}, {})));
    BC_REQUIRE_EQUAL(flight.client.failure(), alert::decode_error);
}

BC_AUTO_TEST_CASE(tls_client__certificate__request_context__illegal_parameter)
{
    server_flight flight{};
    BC_REQUIRE(flight.send_extensions());
    BC_REQUIRE(!flight.send(handshake::certificate, certificate_of(base16_chunk("01"), {})));
    BC_REQUIRE_EQUAL(flight.client.failure(), alert::illegal_parameter);
}

BC_AUTO_TEST_CASE(tls_client__certificate_request__without_algorithms__missing_extension)
{
    server_flight flight{};
    BC_REQUIRE(flight.send_extensions());
    BC_REQUIRE(!flight.send(handshake::certificate_request, base16_chunk("000000")));
    BC_REQUIRE_EQUAL(flight.client.failure(), alert::missing_extension);
}

BC_AUTO_TEST_CASE(tls_client__certificate_request__after_certificate__unexpected_message)
{
    server_flight flight{};
    BC_REQUIRE(flight.send_extensions());
    BC_REQUIRE(flight.send_certificate());
    BC_REQUIRE(!flight.send(handshake::certificate_request, base16_chunk("000000")));
    BC_REQUIRE_EQUAL(flight.client.failure(), alert::unexpected_message);
}

BC_AUTO_TEST_CASE(tls_client__certificate_verify__unoffered_scheme__illegal_parameter)
{
    server_flight flight{};
    BC_REQUIRE(flight.send_extensions());
    BC_REQUIRE(flight.send_certificate());
    BC_REQUIRE(!flight.send(handshake::certificate_verify, verify_of(0x0807, data_chunk(64, 0x01))));
    BC_REQUIRE_EQUAL(flight.client.failure(), alert::illegal_parameter);
}

BC_AUTO_TEST_CASE(tls_client__certificate_verify__invalid_signature__decrypt_error)
{
    server_flight flight{};
    BC_REQUIRE(flight.send_extensions());
    BC_REQUIRE(flight.send_certificate());
    BC_REQUIRE(!flight.send(handshake::certificate_verify, verify_of(ecdsa_secp256r1_sha256, data_chunk(8, 0x01))));
    BC_REQUIRE_EQUAL(flight.client.failure(), alert::decrypt_error);
}

BC_AUTO_TEST_CASE(tls_client__certificate_verify__rsa_scheme_ec_key__decrypt_error)
{
    server_flight flight{};
    BC_REQUIRE(flight.send_extensions());
    BC_REQUIRE(flight.send_certificate());
    BC_REQUIRE(!flight.send(handshake::certificate_verify, verify_of(rsa_pss_rsae_sha256, data_chunk(256, 0x01))));
    BC_REQUIRE_EQUAL(flight.client.failure(), alert::decrypt_error);
}

BC_AUTO_TEST_CASE(tls_client__finished__before_certificate__unexpected_message)
{
    server_flight flight{};
    BC_REQUIRE(flight.send_extensions());
    BC_REQUIRE(!flight.send(handshake::finished, data_chunk(32, 0x00)));
    BC_REQUIRE_EQUAL(flight.client.failure(), alert::unexpected_message);
}

BC_AUTO_TEST_SUITE_END()
