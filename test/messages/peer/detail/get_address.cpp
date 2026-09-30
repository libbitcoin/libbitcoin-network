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

BOOST_AUTO_TEST_SUITE(p2p_get_address_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(get_address__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(get_address::command, "getaddr");
    constexpr auto index = messages::peer::registry::index_of<get_address>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), get_address::command);
    BOOST_REQUIRE_EQUAL(get_address::version_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(get_address::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(get_address__size__always__zero)
{
    BOOST_REQUIRE_EQUAL(get_address::size(level::canonical), zero);
}

static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BOOST_AUTO_TEST_CASE(get_address__deserialize1__empty__expected)
{
    BOOST_REQUIRE(get_address::deserialize(level::minimum_protocol, system::data_chunk{}));
}

BOOST_AUTO_TEST_CASE(get_address__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!get_address::deserialize(level::canonical, system::data_chunk{}));
}

BOOST_AUTO_TEST_CASE(get_address__deserialize1__excess_version__nullptr)
{
    BOOST_REQUIRE(!get_address::deserialize(excess_version, system::data_chunk{}));
}

BOOST_AUTO_TEST_CASE(get_address__deserialize2__empty__source_true)
{
    const system::data_chunk data{};
    system::read::bytes::copy source(data);
    get_address::deserialize(level::maximum_protocol, source);
    BOOST_REQUIRE(source);
}

BOOST_AUTO_TEST_CASE(get_address__serialize1__empty__true)
{
    system::data_chunk data{};
    BOOST_REQUIRE(get_address{}.serialize(level::minimum_protocol, data));
    BOOST_REQUIRE(data.empty());
}

BOOST_AUTO_TEST_SUITE_END()
