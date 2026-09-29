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

BOOST_AUTO_TEST_SUITE(tls_schedule_tests)

using namespace bc::system;
using namespace tls;

static schedule::secret hash_of(const data_loaf& messages)
{
    return sha256_hash(build_chunk(messages));
}

BOOST_AUTO_TEST_CASE(tls_schedule__early_secret__rfc8448__expected)
{
    BOOST_REQUIRE_EQUAL(to_chunk(schedule::early_secret()), rfc8448::early_secret);
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
    const auto secret = schedule::handshake_secret(schedule::early_secret(), rfc8448::shared_secret);
    BOOST_REQUIRE_EQUAL(to_chunk(secret), rfc8448::handshake_secret);
}

BOOST_AUTO_TEST_CASE(tls_schedule__handshake_traffic__rfc8448__expected)
{
    const auto secret = rfc8448::array<32>(rfc8448::handshake_secret);
    const auto hash = hash_of({ rfc8448::client_hello, rfc8448::server_hello });
    BOOST_REQUIRE_EQUAL(to_chunk(schedule::derive_secret(secret, "c hs traffic", hash)), rfc8448::client_handshake_traffic);
    BOOST_REQUIRE_EQUAL(to_chunk(schedule::derive_secret(secret, "s hs traffic", hash)), rfc8448::server_handshake_traffic);
}

BOOST_AUTO_TEST_CASE(tls_schedule__master_secret__rfc8448__expected)
{
    const auto secret = rfc8448::array<32>(rfc8448::handshake_secret);
    BOOST_REQUIRE_EQUAL(to_chunk(schedule::master_secret(secret)), rfc8448::master_secret);
}

BOOST_AUTO_TEST_CASE(tls_schedule__application_traffic__rfc8448__expected)
{
    const auto secret = rfc8448::array<32>(rfc8448::master_secret);
    const auto hash = hash_of({ rfc8448::client_hello, rfc8448::server_hello, rfc8448::server_flight });
    BOOST_REQUIRE_EQUAL(to_chunk(schedule::derive_secret(secret, "c ap traffic", hash)), rfc8448::client_application_traffic);
    BOOST_REQUIRE_EQUAL(to_chunk(schedule::derive_secret(secret, "s ap traffic", hash)), rfc8448::server_application_traffic);
}

BOOST_AUTO_TEST_CASE(tls_schedule__traffic_keys__server_handshake__rfc8448_aes_key_and_iv)
{
    const auto secret = rfc8448::array<32>(rfc8448::server_handshake_traffic);
    BOOST_REQUIRE_EQUAL(schedule::traffic_key(secret, 16), rfc8448::server_handshake_key);
    BOOST_REQUIRE_EQUAL(to_chunk(schedule::traffic_iv(secret)), rfc8448::server_handshake_iv);
}

BOOST_AUTO_TEST_CASE(tls_schedule__traffic_keys__client_application__rfc8448_aes_key_and_iv)
{
    const auto secret = rfc8448::array<32>(rfc8448::client_application_traffic);
    BOOST_REQUIRE_EQUAL(schedule::traffic_key(secret, 16), rfc8448::client_application_key);
    BOOST_REQUIRE_EQUAL(to_chunk(schedule::traffic_iv(secret)), rfc8448::client_application_iv);
}

BOOST_AUTO_TEST_CASE(tls_schedule__traffic_key__chacha__thirty_two_bytes)
{
    const auto secret = rfc8448::array<32>(rfc8448::client_application_traffic);
    BOOST_REQUIRE_EQUAL(schedule::traffic_key(secret, 32).size(), 32u);
}

BOOST_AUTO_TEST_CASE(tls_schedule__finished__server__rfc8448__expected)
{
    const auto secret = rfc8448::array<32>(rfc8448::server_handshake_traffic);
    const auto hash = hash_of({ rfc8448::client_hello, rfc8448::server_hello, rfc8448::encrypted_extensions, rfc8448::certificate, rfc8448::certificate_verify });
    BOOST_REQUIRE_EQUAL(to_chunk(schedule::finished(secret, hash)), rfc8448::server_finished_verify);
}

BOOST_AUTO_TEST_CASE(tls_schedule__finished__client__rfc8448__expected)
{
    const auto secret = rfc8448::array<32>(rfc8448::client_handshake_traffic);
    const auto hash = hash_of({ rfc8448::client_hello, rfc8448::server_hello, rfc8448::server_flight });
    BOOST_REQUIRE_EQUAL(to_chunk(schedule::finished(secret, hash)), rfc8448::client_finished_verify);
}

BOOST_AUTO_TEST_CASE(tls_schedule__update__secret__differs_and_is_deterministic)
{
    const auto secret = rfc8448::array<32>(rfc8448::client_application_traffic);
    const auto updated = schedule::update(secret);
    BOOST_REQUIRE(updated != secret);
    BOOST_REQUIRE(updated == schedule::expand_label(secret, "traffic upd", {}));
}

BOOST_AUTO_TEST_SUITE_END()
