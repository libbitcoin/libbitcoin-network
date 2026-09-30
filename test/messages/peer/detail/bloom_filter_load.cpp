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

BOOST_AUTO_TEST_SUITE(p2p_bloom_filter_load_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(bloom_filter_load__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(bloom_filter_load::command, "filterload");
    constexpr auto index = messages::peer::registry::index_of<bloom_filter_load>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), bloom_filter_load::command);
    BOOST_REQUIRE_EQUAL(bloom_filter_load::version_minimum, level::bip37);
    BOOST_REQUIRE_EQUAL(bloom_filter_load::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(bloom_filter_load__size__default__xpected)
{
    constexpr auto expected = variable_size(zero)
        + sizeof(uint32_t)
        + sizeof(uint32_t)
        + sizeof(uint8_t);

    BOOST_REQUIRE_EQUAL(bloom_filter_load{}.size(level::canonical), expected);
}

// bitcoin/src/test/bloom_tests.cpp
static const auto payload = system::base16_chunk("03614e9b050000000000000001");
static const auto payload_tweak = system::base16_chunk("03ce4299050000000100008001");
static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BOOST_AUTO_TEST_CASE(bloom_filter_load__size__three_bytes__expected)
{
    const bloom_filter_load message{ system::base16_chunk("614e9b"), 5, 0, 1 };
    BOOST_REQUIRE_EQUAL(message.size(level::bip37), payload.size());
}

BOOST_AUTO_TEST_CASE(bloom_filter_load__deserialize1__bitcoind_sample__expected)
{
    const auto message = bloom_filter_load::deserialize(level::bip37, payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->filter, system::base16_chunk("614e9b"));
    BOOST_REQUIRE_EQUAL(message->hash_functions, 5u);
    BOOST_REQUIRE_EQUAL(message->tweak, 0u);
    BOOST_REQUIRE_EQUAL(message->flags, 1u);
}

BOOST_AUTO_TEST_CASE(bloom_filter_load__deserialize1__bitcoind_tweak_sample__expected)
{
    const auto message = bloom_filter_load::deserialize(level::bip37, payload_tweak);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->filter, system::base16_chunk("ce4299"));
    BOOST_REQUIRE_EQUAL(message->hash_functions, 5u);
    BOOST_REQUIRE_EQUAL(message->tweak, 2147483649u);
    BOOST_REQUIRE_EQUAL(message->flags, 1u);
}

BOOST_AUTO_TEST_CASE(bloom_filter_load__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!bloom_filter_load::deserialize(level::bip35, payload));
}

BOOST_AUTO_TEST_CASE(bloom_filter_load__deserialize1__excess_version__nullptr)
{
    BOOST_REQUIRE(!bloom_filter_load::deserialize(excess_version, payload));
}

BOOST_AUTO_TEST_CASE(bloom_filter_load__deserialize1__underflow__nullptr)
{
    const auto data = system::base16_chunk("03614e9b0500000000000000");
    BOOST_REQUIRE(!bloom_filter_load::deserialize(level::bip37, data));
}

BOOST_AUTO_TEST_CASE(bloom_filter_load__deserialize1__maximum_hash_functions__expected)
{
    const auto data = system::base16_chunk("03614e9b320000000000000001");
    const auto message = bloom_filter_load::deserialize(level::bip37, data);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->hash_functions, 50u);
}

BOOST_AUTO_TEST_CASE(bloom_filter_load__deserialize1__excess_hash_functions__nullptr)
{
    const auto data = system::base16_chunk("03614e9b330000000000000001");
    BOOST_REQUIRE(!bloom_filter_load::deserialize(level::bip37, data));
}

BOOST_AUTO_TEST_CASE(bloom_filter_load__deserialize1__excess_filter_size__nullptr)
{
    const auto data = system::base16_chunk("fea18c0000");
    BOOST_REQUIRE(!bloom_filter_load::deserialize(level::bip37, data));
}

BOOST_AUTO_TEST_CASE(bloom_filter_load__deserialize2__bitcoind_sample__expected)
{
    system::read::bytes::copy source(payload);
    const auto message = bloom_filter_load::deserialize(level::maximum_protocol, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(message.filter, system::base16_chunk("614e9b"));
}

BOOST_AUTO_TEST_CASE(bloom_filter_load__serialize1__bitcoind_tweak_sample__expected)
{
    const bloom_filter_load message{ system::base16_chunk("ce4299"), 5, 2147483649, 1 };
    system::data_chunk data(message.size(level::bip37));
    BOOST_REQUIRE(message.serialize(level::bip37, data));
    BOOST_REQUIRE_EQUAL(data, payload_tweak);
}

BOOST_AUTO_TEST_SUITE_END()
