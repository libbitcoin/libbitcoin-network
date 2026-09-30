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

BOOST_AUTO_TEST_SUITE(tls_record_tests)

using namespace bc::system;
using namespace tls;

static data_chunk head(const data_chunk& record)
{
    return { record.begin(), std::next(record.begin(), record_header_size) };
}

static data_chunk tail(const data_chunk& record)
{
    return { std::next(record.begin(), record_header_size), record.end() };
}

BOOST_AUTO_TEST_CASE(tls_record__seal__cleartext_server_hello__rfc8448_record)
{
    record sender{};
    data_chunk out{};
    sender.seal(out, content::handshake, rfc8448::server_hello);
    BOOST_REQUIRE(!sender.is_protected());
    BOOST_REQUIRE_EQUAL(out, rfc8448::server_hello_record);
}

BOOST_AUTO_TEST_CASE(tls_record__seal__server_flight__rfc8448_record)
{
    record sender{};
    sender.set_secret(aes_128_gcm_sha256, rfc8448::server_handshake_traffic);

    data_chunk out{};
    sender.seal(out, content::handshake, rfc8448::server_flight);
    BOOST_REQUIRE(sender.is_protected());
    BOOST_REQUIRE_EQUAL(out, rfc8448::server_flight_record);
}

BOOST_AUTO_TEST_CASE(tls_record__open__client_finished__rfc8448_finished)
{
    record receiver{};
    receiver.set_secret(aes_128_gcm_sha256, rfc8448::client_handshake_traffic);

    uint8_t type{};
    data_chunk content{};
    BOOST_REQUIRE(receiver.open(type, content, head(rfc8448::client_finished_record), tail(rfc8448::client_finished_record)));
    BOOST_REQUIRE_EQUAL(type, content::handshake);
    BOOST_REQUIRE_EQUAL(content, rfc8448::client_finished);
}

BOOST_AUTO_TEST_CASE(tls_record__seal_open__application_data__rfc8448_records)
{
    record client{};
    client.set_secret(aes_128_gcm_sha256, rfc8448::client_application_traffic);

    data_chunk sealed{};
    client.seal(sealed, content::application_data, rfc8448::client_data);
    BOOST_REQUIRE_EQUAL(sealed, rfc8448::client_data_record);

    record server{};
    server.set_secret(aes_128_gcm_sha256, rfc8448::client_application_traffic);

    uint8_t type{};
    data_chunk content{};
    BOOST_REQUIRE(server.open(type, content, head(sealed), tail(sealed)));
    BOOST_REQUIRE_EQUAL(type, content::application_data);
    BOOST_REQUIRE_EQUAL(content, rfc8448::client_data);
}

BOOST_AUTO_TEST_CASE(tls_record__seal__sequence__server_data_then_alert_rfc8448_records)
{
    record server{};
    server.set_secret(aes_128_gcm_sha256, rfc8448::server_application_traffic);

    // The trace server sends a session ticket first (sequence zero).
    data_chunk ignored{};
    server.seal(ignored, content::handshake, data_chunk(205, 0x00));

    data_chunk data{};
    server.seal(data, content::application_data, rfc8448::server_data);
    BOOST_REQUIRE_EQUAL(data, rfc8448::server_data_record);

    data_chunk alert{};
    server.seal(alert, content::alert, rfc8448::server_alert);
    BOOST_REQUIRE_EQUAL(alert, rfc8448::server_alert_record);
}

BOOST_AUTO_TEST_CASE(tls_record__open__tampered__false)
{
    auto tampered = rfc8448::client_data_record;
    tampered.back() ^= 0x01;

    record receiver{};
    receiver.set_secret(aes_128_gcm_sha256, rfc8448::client_application_traffic);

    uint8_t type{};
    data_chunk content{};
    BOOST_REQUIRE(!receiver.open(type, content, head(tampered), tail(tampered)));
}

BOOST_AUTO_TEST_CASE(tls_record__open__tag_only__false)
{
    record receiver{};
    receiver.set_secret(aes_128_gcm_sha256, rfc8448::client_application_traffic);

    uint8_t type{};
    data_chunk content{};
    const data_chunk fragment(record::tag_size, 0x00);
    BOOST_REQUIRE(!receiver.open(type, content, head(rfc8448::client_data_record), fragment));
}

BOOST_AUTO_TEST_CASE(tls_record__seal_open__chacha__round_trip)
{
    const auto secret = rfc8448::client_application_traffic;
    record sender{};
    record receiver{};
    sender.set_secret(chacha20_poly1305_sha256, secret);
    receiver.set_secret(chacha20_poly1305_sha256, secret);

    data_chunk sealed{};
    sender.seal(sealed, content::application_data, rfc8448::client_data);
    BOOST_REQUIRE_EQUAL(sealed.size(), rfc8448::client_data_record.size());

    uint8_t type{};
    data_chunk content{};
    BOOST_REQUIRE(receiver.open(type, content, head(sealed), tail(sealed)));
    BOOST_REQUIRE_EQUAL(type, content::application_data);
    BOOST_REQUIRE_EQUAL(content, rfc8448::client_data);
}

BOOST_AUTO_TEST_CASE(tls_record__seal_open__aes256__round_trip)
{
    const auto secret = base16_chunk("960606d0a2e35fbc1310cdbb5a956be93051ae930ce7b4b17dd1036d26e60e1613dce56c6e145646d50fa154d001a8ab");
    record sender{};
    record receiver{};
    sender.set_secret(aes_256_gcm_sha384, secret);
    receiver.set_secret(aes_256_gcm_sha384, secret);

    data_chunk sealed{};
    sender.seal(sealed, content::application_data, rfc8448::client_data);
    BOOST_REQUIRE_EQUAL(sealed.size(), rfc8448::client_data_record.size());

    uint8_t type{};
    data_chunk content{};
    BOOST_REQUIRE(receiver.open(type, content, head(sealed), tail(sealed)));
    BOOST_REQUIRE_EQUAL(type, content::application_data);
    BOOST_REQUIRE_EQUAL(content, rfc8448::client_data);
}

BOOST_AUTO_TEST_CASE(tls_record__sequence__seal_and_set_secret__counts_and_resets)
{
    const auto secret = rfc8448::client_application_traffic;
    record sender{};
    BOOST_REQUIRE_EQUAL(sender.sequence(), 0u);

    sender.set_secret(aes_128_gcm_sha256, secret);
    data_chunk sealed{};
    sender.seal(sealed, content::application_data, rfc8448::client_data);
    sender.seal(sealed, content::application_data, rfc8448::client_data);
    BOOST_REQUIRE_EQUAL(sender.sequence(), 2u);

    sender.set_secret(aes_128_gcm_sha256, secret);
    BOOST_REQUIRE_EQUAL(sender.sequence(), 0u);
}

BOOST_AUTO_TEST_CASE(tls_record__open__padding_only__zero_type)
{
    const auto secret = rfc8448::client_application_traffic;
    record sender{};
    record receiver{};
    sender.set_secret(aes_128_gcm_sha256, secret);
    receiver.set_secret(aes_128_gcm_sha256, secret);

    data_chunk sealed{};
    sender.seal(sealed, 0x00, data_chunk(4, 0x00));

    uint8_t type{ 0xff };
    data_chunk content{ 0x42 };
    BOOST_REQUIRE(receiver.open(type, content, head(sealed), tail(sealed)));
    BOOST_REQUIRE_EQUAL(type, 0u);
    BOOST_REQUIRE(content.empty());
}

BOOST_AUTO_TEST_SUITE_END()
