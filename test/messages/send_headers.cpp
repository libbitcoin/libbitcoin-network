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

BOOST_AUTO_TEST_SUITE(send_headers_tests)

using namespace bc::network::messages;

BOOST_AUTO_TEST_CASE(send_headers__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(send_headers::command, "sendheaders");
    BOOST_REQUIRE(send_headers::id == identifier::send_headers);
    BOOST_REQUIRE_EQUAL(send_headers::version_minimum, level::bip130);
    BOOST_REQUIRE_EQUAL(send_headers::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(send_headers__size__always__zero)
{
    BOOST_REQUIRE_EQUAL(send_headers::size(level::canonical), zero);
}

BOOST_AUTO_TEST_SUITE_END()
