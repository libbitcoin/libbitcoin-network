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

BOOST_AUTO_TEST_SUITE(p2p_bloom_filter_clear_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(bloom_filter_clear__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(bloom_filter_clear::command, "filterclear");
    constexpr auto index = messages::peer::registry::index_of<bloom_filter_clear>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), bloom_filter_clear::command);
    BOOST_REQUIRE_EQUAL(bloom_filter_clear::version_minimum, level::bip37);
    BOOST_REQUIRE_EQUAL(bloom_filter_clear::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(bloom_filter_clear__size__always__zero)
{
    BOOST_REQUIRE_EQUAL(bloom_filter_clear::size(level::canonical), zero);
}

static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BOOST_AUTO_TEST_CASE(bloom_filter_clear__deserialize1__empty__expected)
{
    BOOST_REQUIRE(bloom_filter_clear::deserialize(level::bip37, system::data_chunk{}));
}

BOOST_AUTO_TEST_CASE(bloom_filter_clear__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!bloom_filter_clear::deserialize(level::bip35, system::data_chunk{}));
}

BOOST_AUTO_TEST_CASE(bloom_filter_clear__deserialize1__excess_version__nullptr)
{
    BOOST_REQUIRE(!bloom_filter_clear::deserialize(excess_version, system::data_chunk{}));
}

BOOST_AUTO_TEST_CASE(bloom_filter_clear__deserialize2__empty__source_true)
{
    const system::data_chunk data{};
    system::read::bytes::copy source(data);
    bloom_filter_clear::deserialize(level::maximum_protocol, source);
    BOOST_REQUIRE(source);
}

BOOST_AUTO_TEST_CASE(bloom_filter_clear__serialize1__empty__true)
{
    system::data_chunk data{};
    BOOST_REQUIRE(bloom_filter_clear{}.serialize(level::bip37, data));
    BOOST_REQUIRE(data.empty());
}

BOOST_AUTO_TEST_SUITE_END()
