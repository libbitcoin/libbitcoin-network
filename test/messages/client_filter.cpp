/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(client_filter_tests)

using namespace bc::network::messages;

BOOST_AUTO_TEST_CASE(client_filter__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(client_filter::command, "cfilter");
    BOOST_REQUIRE(client_filter::id == identifier::client_filter);
    BOOST_REQUIRE_EQUAL(client_filter::version_minimum, level::bip157);
    BOOST_REQUIRE_EQUAL(client_filter::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(client_filter__size__default__expected)
{
    constexpr auto expected = sizeof(uint8_t)
        + system::hash_size
        + variable_size(zero);

    BOOST_REQUIRE_EQUAL(client_filter{}.size(level::canonical), expected);
}

BOOST_AUTO_TEST_SUITE_END()
