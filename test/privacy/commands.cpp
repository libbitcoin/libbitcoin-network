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

BOOST_AUTO_TEST_SUITE(privacy_commands_tests)

using namespace network::privacy;

BOOST_AUTO_TEST_CASE(privacy_commands__to_command__all_assigned__expected)
{
    BOOST_REQUIRE_EQUAL(to_command(1), "addr");
    BOOST_REQUIRE_EQUAL(to_command(2), "block");
    BOOST_REQUIRE_EQUAL(to_command(13), "headers");
    BOOST_REQUIRE_EQUAL(to_command(14), "inv");
    BOOST_REQUIRE_EQUAL(to_command(18), "ping");
    BOOST_REQUIRE_EQUAL(to_command(19), "pong");
    BOOST_REQUIRE_EQUAL(to_command(21), "tx");
    BOOST_REQUIRE_EQUAL(to_command(28), "addrv2");
}

BOOST_AUTO_TEST_CASE(privacy_commands__to_command__unassigned__empty)
{
    BOOST_REQUIRE(to_command(0).empty());
    BOOST_REQUIRE(to_command(29).empty());
    BOOST_REQUIRE(to_command(255).empty());
}

BOOST_AUTO_TEST_CASE(privacy_commands__to_identifier__round_trip__expected)
{
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(1)), 1u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(2)), 2u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(3)), 3u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(4)), 4u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(5)), 5u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(6)), 6u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(7)), 7u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(8)), 8u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(9)), 9u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(10)), 10u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(11)), 11u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(12)), 12u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(13)), 13u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(14)), 14u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(15)), 15u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(16)), 16u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(17)), 17u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(18)), 18u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(19)), 19u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(20)), 20u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(21)), 21u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(22)), 22u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(23)), 23u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(24)), 24u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(25)), 25u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(26)), 26u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(27)), 27u);
    BOOST_REQUIRE_EQUAL(to_identifier(to_command(28)), 28u);
}

BOOST_AUTO_TEST_CASE(privacy_commands__to_identifier__unmapped__zero)
{
    BOOST_REQUIRE_EQUAL(to_identifier("version"), 0u);
    BOOST_REQUIRE_EQUAL(to_identifier("verack"), 0u);
    BOOST_REQUIRE_EQUAL(to_identifier("sendaddrv2"), 0u);
    BOOST_REQUIRE_EQUAL(to_identifier(""), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
