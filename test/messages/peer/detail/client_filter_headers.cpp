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
#include "../../../test.hpp"

BOOST_AUTO_TEST_SUITE(p2p_client_filter_headers_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(client_filter_headers__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(client_filter_headers::command, "cfheaders");
    constexpr auto index = messages::peer::registry::index_of<client_filter_headers>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), client_filter_headers::command);
    BOOST_REQUIRE_EQUAL(client_filter_headers::version_minimum, level::bip157);
    BOOST_REQUIRE_EQUAL(client_filter_headers::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(client_filter_headers__size__default__expected)
{
    constexpr auto expected = sizeof(uint8_t)
        + system::hash_size
        + system::hash_size
        + variable_size(zero);

    BOOST_REQUIRE_EQUAL(client_filter_headers{}.size(level::canonical), expected);
}

// bitcoin/src/test/data/blockfilters.json (testnet block 2)
static const auto stop_hash = system::base16_hash("000000006c02c8ea6e4ff69651f7fcde348fb9d557a06e6957b65552002a7820");
static const auto previous_header = system::base16_hash("d7bdac13a59d745b1add0d2ce852f1a0442e8945fc1bf3848d3cbffd88c24fe1");
static const auto filter_hash = system::base16_hash("3cd1fafd2aa8b5b3ca58c8a3459cb27ec9fc78329fcb0d379a234b4c92adc8eb");
static const auto payload = system::base16_chunk(
    "00"
    "20782a005255b657696ea057d5b98f34defcf75196f64f6eeac8026c00000000"
    "e14fc288fdbf3c8d84f31bfc45892e44a0f152e82c0ddd1a5b749da513acbdd7"
    "01"
    "ebc8ad924c4b239a370dcb9f3278fcc97eb29c45a3c858cab3b5a82afdfad13c");
static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BOOST_AUTO_TEST_CASE(client_filter_headers__size__one__expected)
{
    const client_filter_headers message{ client_filter::type_id::neutrino, stop_hash, previous_header, { filter_hash } };
    BOOST_REQUIRE_EQUAL(message.size(level::bip157), payload.size());
}

BOOST_AUTO_TEST_CASE(client_filter_headers__deserialize1__one__expected)
{
    const auto message = client_filter_headers::deserialize(level::bip157, payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->filter_type, client_filter::type_id::neutrino);
    BOOST_REQUIRE_EQUAL(message->stop_hash, stop_hash);
    BOOST_REQUIRE_EQUAL(message->previous_filter_header, previous_header);
    BOOST_REQUIRE_EQUAL(message->filter_hashes.size(), one);
    BOOST_REQUIRE_EQUAL(message->filter_hashes.front(), filter_hash);
}

BOOST_AUTO_TEST_CASE(client_filter_headers__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!client_filter_headers::deserialize(level::bip152, payload));
}

BOOST_AUTO_TEST_CASE(client_filter_headers__deserialize1__excess_version__nullptr)
{
    BOOST_REQUIRE(!client_filter_headers::deserialize(excess_version, payload));
}

BOOST_AUTO_TEST_CASE(client_filter_headers__deserialize1__underflow__nullptr)
{
    const system::data_chunk data{ payload.begin(), std::prev(payload.end()) };
    BOOST_REQUIRE(!client_filter_headers::deserialize(level::bip157, data));
}

BOOST_AUTO_TEST_CASE(client_filter_headers__deserialize2__one__expected)
{
    system::read::bytes::copy source(payload);
    const auto message = client_filter_headers::deserialize(level::maximum_protocol, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(message.previous_filter_header, previous_header);
}

BOOST_AUTO_TEST_CASE(client_filter_headers__serialize1__one__expected)
{
    const client_filter_headers message{ client_filter::type_id::neutrino, stop_hash, previous_header, { filter_hash } };
    system::data_chunk data(message.size(level::bip157));
    BOOST_REQUIRE(message.serialize(level::bip157, data));
    BOOST_REQUIRE_EQUAL(data, payload);
}

BOOST_AUTO_TEST_SUITE_END()
