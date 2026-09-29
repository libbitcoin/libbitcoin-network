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

BOOST_AUTO_TEST_SUITE(p2p_bloom_filter_add_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(bloom_filter_add__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(bloom_filter_add::command, "filteradd");
    constexpr auto index = messages::peer::registry::index_of<bloom_filter_add>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), bloom_filter_add::command);
    BOOST_REQUIRE_EQUAL(bloom_filter_add::version_minimum, level::bip37);
    BOOST_REQUIRE_EQUAL(bloom_filter_add::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(bloom_filter_add__size__default__expected)
{
    constexpr auto expected = variable_size(zero);
    BOOST_REQUIRE_EQUAL(bloom_filter_add{}.size(level::canonical), expected);
}

static const auto payload = system::base16_chunk("04" "deadbeef");
static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BOOST_AUTO_TEST_CASE(bloom_filter_add__size__four_bytes__expected)
{
    const bloom_filter_add message{ system::base16_chunk("deadbeef") };
    BOOST_REQUIRE_EQUAL(message.size(level::bip37), payload.size());
}

BOOST_AUTO_TEST_CASE(bloom_filter_add__deserialize1__valid__expected)
{
    const auto message = bloom_filter_add::deserialize(level::bip37, payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->data, system::base16_chunk("deadbeef"));
}

BOOST_AUTO_TEST_CASE(bloom_filter_add__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!bloom_filter_add::deserialize(level::bip35, payload));
}

BOOST_AUTO_TEST_CASE(bloom_filter_add__deserialize1__excess_version__nullptr)
{
    BOOST_REQUIRE(!bloom_filter_add::deserialize(excess_version, payload));
}

BOOST_AUTO_TEST_CASE(bloom_filter_add__deserialize1__underflow__nullptr)
{
    const auto data = system::base16_chunk("04deadbe");
    BOOST_REQUIRE(!bloom_filter_add::deserialize(level::bip37, data));
}

BOOST_AUTO_TEST_CASE(bloom_filter_add__deserialize1__maximum_size__expected)
{
    const auto data = system::splice(system::base16_chunk("fd0802"), system::data_chunk(520, 0x42));
    const auto message = bloom_filter_add::deserialize(level::bip37, data);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->data.size(), 520u);
}

BOOST_AUTO_TEST_CASE(bloom_filter_add__deserialize1__excess_size__nullptr)
{
    const auto data = system::splice(system::base16_chunk("fd0902"), system::data_chunk(521, 0x42));
    BOOST_REQUIRE(!bloom_filter_add::deserialize(level::bip37, data));
}

BOOST_AUTO_TEST_CASE(bloom_filter_add__deserialize2__valid__expected)
{
    system::read::bytes::copy source(payload);
    const auto message = bloom_filter_add::deserialize(level::maximum_protocol, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(message.data, system::base16_chunk("deadbeef"));
}

BOOST_AUTO_TEST_CASE(bloom_filter_add__serialize1__valid__expected)
{
    const bloom_filter_add message{ system::base16_chunk("deadbeef") };
    system::data_chunk data(message.size(level::bip37));
    BOOST_REQUIRE(message.serialize(level::bip37, data));
    BOOST_REQUIRE_EQUAL(data, payload);
}

BOOST_AUTO_TEST_SUITE_END()
