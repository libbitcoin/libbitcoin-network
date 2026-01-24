/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(p2p_block_tests)

using namespace network::messages;

BOOST_AUTO_TEST_CASE(block__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(peer::block::command, "block");
    BOOST_REQUIRE(peer::block::id == peer::identifier::block);
    BOOST_REQUIRE_EQUAL(peer::block::version_minimum, peer::level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(peer::block::version_maximum, peer::level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(block__size__default__zero)
{
    BOOST_REQUIRE_EQUAL(peer::block{}.size(peer::level::canonical, true), zero);
    BOOST_REQUIRE_EQUAL(peer::block{}.size(peer::level::canonical, false), zero);
}

BOOST_AUTO_TEST_SUITE_END()
