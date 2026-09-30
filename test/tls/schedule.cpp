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
#include "rfc8448.hpp"

#include <numeric>

BOOST_AUTO_TEST_SUITE(tls_schedule_tests)

using namespace bc::system;
using namespace tls;

const schedule keys{ aes_128_gcm_sha256 };

static schedule::secret hash_of(const data_loaf& messages)
{
    return to_chunk(sha256_hash(build_chunk(messages)));
}

BOOST_AUTO_TEST_CASE(tls_schedule__early_secret__rfc8448__expected)
{
    BOOST_REQUIRE_EQUAL(keys.early_secret(), rfc8448::early_secret);
}

BOOST_AUTO_TEST_CASE(tls_schedule__x25519__rfc8448__shared_secret)
{
    const auto secret = rfc8448::array<x25519::key_size>(rfc8448::server_private);
    const auto point = rfc8448::array<x25519::key_size>(rfc8448::client_public);
    x25519::key shared{};
    BOOST_REQUIRE(x25519::multiply(shared, secret, point));
    BOOST_REQUIRE_EQUAL(to_chunk(shared), rfc8448::shared_secret);
}

BOOST_AUTO_TEST_CASE(tls_schedule__handshake_secret__rfc8448__expected)
{
    const auto secret = keys.handshake_secret(keys.early_secret(), rfc8448::shared_secret);
    BOOST_REQUIRE_EQUAL(secret, rfc8448::handshake_secret);
}

BOOST_AUTO_TEST_CASE(tls_schedule__handshake_traffic__rfc8448__expected)
{
    const auto secret = rfc8448::handshake_secret;
    const auto hash = hash_of({ rfc8448::client_hello, rfc8448::server_hello });
    BOOST_REQUIRE_EQUAL(keys.derive_secret(secret, "c hs traffic", hash), rfc8448::client_handshake_traffic);
    BOOST_REQUIRE_EQUAL(keys.derive_secret(secret, "s hs traffic", hash), rfc8448::server_handshake_traffic);
}

BOOST_AUTO_TEST_CASE(tls_schedule__master_secret__rfc8448__expected)
{
    const auto secret = rfc8448::handshake_secret;
    BOOST_REQUIRE_EQUAL(keys.master_secret(secret), rfc8448::master_secret);
}

BOOST_AUTO_TEST_CASE(tls_schedule__application_traffic__rfc8448__expected)
{
    const auto secret = rfc8448::master_secret;
    const auto hash = hash_of({ rfc8448::client_hello, rfc8448::server_hello, rfc8448::server_flight });
    BOOST_REQUIRE_EQUAL(keys.derive_secret(secret, "c ap traffic", hash), rfc8448::client_application_traffic);
    BOOST_REQUIRE_EQUAL(keys.derive_secret(secret, "s ap traffic", hash), rfc8448::server_application_traffic);
}

BOOST_AUTO_TEST_CASE(tls_schedule__traffic_keys__server_handshake__rfc8448_aes_key_and_iv)
{
    const auto secret = rfc8448::server_handshake_traffic;
    BOOST_REQUIRE_EQUAL(keys.traffic_key(secret), rfc8448::server_handshake_key);
    BOOST_REQUIRE_EQUAL(to_chunk(keys.traffic_iv(secret)), rfc8448::server_handshake_iv);
}

BOOST_AUTO_TEST_CASE(tls_schedule__traffic_keys__client_application__rfc8448_aes_key_and_iv)
{
    const auto secret = rfc8448::client_application_traffic;
    BOOST_REQUIRE_EQUAL(keys.traffic_key(secret), rfc8448::client_application_key);
    BOOST_REQUIRE_EQUAL(to_chunk(keys.traffic_iv(secret)), rfc8448::client_application_iv);
}

BOOST_AUTO_TEST_CASE(tls_schedule__traffic_key__chacha__thirty_two_bytes)
{
    const auto secret = rfc8448::client_application_traffic;
    BOOST_REQUIRE_EQUAL(schedule{ chacha20_poly1305_sha256 }.traffic_key(secret).size(), 32u);
}

BOOST_AUTO_TEST_CASE(tls_schedule__finished__server__rfc8448__expected)
{
    const auto secret = rfc8448::server_handshake_traffic;
    const auto hash = hash_of({ rfc8448::client_hello, rfc8448::server_hello, rfc8448::encrypted_extensions, rfc8448::certificate, rfc8448::certificate_verify });
    BOOST_REQUIRE_EQUAL(keys.finished(secret, hash), rfc8448::server_finished_verify);
}

BOOST_AUTO_TEST_CASE(tls_schedule__finished__client__rfc8448__expected)
{
    const auto secret = rfc8448::client_handshake_traffic;
    const auto hash = hash_of({ rfc8448::client_hello, rfc8448::server_hello, rfc8448::server_flight });
    BOOST_REQUIRE_EQUAL(keys.finished(secret, hash), rfc8448::client_finished_verify);
}

BOOST_AUTO_TEST_CASE(tls_schedule__update__secret__differs_and_is_deterministic)
{
    const auto secret = rfc8448::client_application_traffic;
    const auto updated = keys.update(secret);
    BOOST_REQUIRE(updated != secret);
    BOOST_REQUIRE(updated == keys.expand_label(secret, "traffic upd", {}));
}

// TLS_AES_256_GCM_SHA384 (sha384), expectations from an independent hkdf.

const schedule keys384{ aes_256_gcm_sha384 };
const auto early384 = base16_chunk("7ee8206f5570023e6dc7519eb1073bc4e791ad37b5c382aa10ba18e2357e716971f9362f2c2fe2a76bfd78dfec4ea9b5");
const auto handshake384 = base16_chunk("deb1becd84738bdcbf73bdd28bd2209d2fe51b84986a4fbb63d7e3902c00a7d867db2d8e1af869b952dcab5d7f482cab");
const auto transcript384 = base16_chunk("520cde83e730ef1c04fe443dc399ded36c0f275993c190b6fc9fc11db7dae644ae6073f4371fa061a7482e6cc2e90ed7");
const auto client_handshake384 = base16_chunk("960606d0a2e35fbc1310cdbb5a956be93051ae930ce7b4b17dd1036d26e60e1613dce56c6e145646d50fa154d001a8ab");

BOOST_AUTO_TEST_CASE(tls_schedule__size__suites__hash_sizes)
{
    BOOST_REQUIRE_EQUAL(keys.size(), 32u);
    BOOST_REQUIRE_EQUAL(schedule{ chacha20_poly1305_sha256 }.size(), 32u);
    BOOST_REQUIRE_EQUAL(keys384.size(), 48u);
}

BOOST_AUTO_TEST_CASE(tls_schedule__hash__sha384__expected)
{
    BOOST_REQUIRE_EQUAL(keys384.hash(to_chunk("transcript")), transcript384);
}

BOOST_AUTO_TEST_CASE(tls_schedule__early_secret__sha384__expected)
{
    BOOST_REQUIRE_EQUAL(keys384.early_secret(), early384);
}

BOOST_AUTO_TEST_CASE(tls_schedule__handshake_secret__sha384__expected)
{
    data_chunk shared(48);
    std::iota(shared.begin(), shared.end(), 0_u8);
    BOOST_REQUIRE_EQUAL(keys384.handshake_secret(early384, shared), handshake384);
}

BOOST_AUTO_TEST_CASE(tls_schedule__derive_secret__sha384__expected)
{
    BOOST_REQUIRE_EQUAL(keys384.derive_secret(handshake384, "c hs traffic", transcript384), client_handshake384);
}

BOOST_AUTO_TEST_CASE(tls_schedule__master_secret__sha384__expected)
{
    BOOST_REQUIRE_EQUAL(keys384.master_secret(handshake384), base16_chunk("78f71f66fe0d83f65f92e4665b4e2acccb43632e32ebf128e19b5d95c60d4990721065aea51bb55a9eb9f81cd7cbf3e1"));
}

BOOST_AUTO_TEST_CASE(tls_schedule__traffic_keys__sha384__aes256_key_and_iv)
{
    BOOST_REQUIRE_EQUAL(keys384.traffic_key(client_handshake384), base16_chunk("3801d45f5b49c63e3da2ed75d297bc6c8ff7bc516e652e51046aee5f0e4e47ec"));
    BOOST_REQUIRE_EQUAL(to_chunk(keys384.traffic_iv(client_handshake384)), base16_chunk("3ce78a39fdd67da26d7da0d7"));
}

BOOST_AUTO_TEST_CASE(tls_schedule__finished__sha384__expected)
{
    BOOST_REQUIRE_EQUAL(keys384.finished(client_handshake384, transcript384), base16_chunk("a7f6576df42ad919abc2cb977b0183a18899aeb7aa4081507aa83d0db0dce9d851ce1649f6381402a44e9290fa875567"));
}

BOOST_AUTO_TEST_CASE(tls_schedule__update__sha384__expected)
{
    BOOST_REQUIRE_EQUAL(keys384.update(client_handshake384), base16_chunk("370174443bc4cb174e862f78e81bbcf19525ab2df0f3b70223f4edd3f6f1c8721201aa403ee05e80ae9a08a7d935c18a"));
}

// transcript

BOOST_AUTO_TEST_CASE(tls_transcript__hash__suites__suite_hash)
{
    const auto data = to_chunk("transcript");
    transcript sha256{};
    sha256.reset(aes_128_gcm_sha256);
    sha256.write(data);
    BOOST_REQUIRE_EQUAL(sha256.hash(), to_chunk(sha256_hash(data)));

    transcript sha384{};
    sha384.reset(aes_256_gcm_sha384);
    sha384.write(to_chunk("trans"));
    sha384.write(to_chunk("cript"));
    BOOST_REQUIRE_EQUAL(sha384.hash(), transcript384);
    BOOST_REQUIRE_EQUAL(sha384.hash(), transcript384);
}

BOOST_AUTO_TEST_SUITE_END()
