/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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

using namespace config;

BOOST_AUTO_TEST_SUITE(utilities_tests)

// to_literal

BOOST_AUTO_TEST_CASE(utilities__to_literal__empty_zero__empty)
{
    BOOST_REQUIRE(to_literal("", 0).empty());
}

BOOST_AUTO_TEST_CASE(utilities__to_literal__single_token_zero__not_bracketed)
{
    BOOST_REQUIRE_EQUAL(to_literal("foobar", 0), "foobar");
}

BOOST_AUTO_TEST_CASE(utilities__to_literal__dotted_zero__not_bracketed)
{
    BOOST_REQUIRE_EQUAL(to_literal("foo.bar", 0), "foo.bar");
}

BOOST_AUTO_TEST_CASE(utilities__to_literal__coloned_zero__bracketed)
{
    BOOST_REQUIRE_EQUAL(to_literal("foo:bar", 0), "[foo:bar]");
}

BOOST_AUTO_TEST_CASE(utilities__to_literal__bracketed_coloned_zero__single_bracketed)
{
    BOOST_REQUIRE_EQUAL(to_literal("[foo:bar]", 0), "[foo:bar]");
}

BOOST_AUTO_TEST_CASE(utilities__to_literal__empty_nonzero__expected)
{
    BOOST_REQUIRE_EQUAL(to_literal("", 42), ":42");
}

BOOST_AUTO_TEST_CASE(utilities__to_literal__single_token_nonzero__not_bracketed_port)
{
    BOOST_REQUIRE_EQUAL(to_literal("foobar", 42), "foobar:42");
}

BOOST_AUTO_TEST_CASE(utilities__to_literal__dotted_nonzero__not_bracketed_port)
{
    BOOST_REQUIRE_EQUAL(to_literal("foo.bar", 42), "foo.bar:42");
}

BOOST_AUTO_TEST_CASE(utilities__to_literal__coloned_nonzero__bracketed_port)
{
    BOOST_REQUIRE_EQUAL(to_literal("foo:bar", 42), "[foo:bar]:42");
}

BOOST_AUTO_TEST_CASE(utilities__to_literal__bracketed_coloned_nonzero__single_bracketed_port)
{
    BOOST_REQUIRE_EQUAL(to_literal("[foo:bar]", 42), "[foo:bar]:42");
}

// to_host

BOOST_AUTO_TEST_CASE(utilities__to_host__default__unspecified_v4)
{
    BOOST_REQUIRE_EQUAL(to_host(asio::address{}), "0.0.0.0");
}

// from_host

BOOST_AUTO_TEST_CASE(utilities__from_host__default__default)
{
    BOOST_REQUIRE_EQUAL(from_host(std::string{}), asio::address{});
}

// to_address

BOOST_AUTO_TEST_CASE(utilities__to_address__default_ipv6__default)
{
    BOOST_REQUIRE_EQUAL(to_address(asio::ipv6{}), messages::ip_address{});
}

// from_address

BOOST_AUTO_TEST_CASE(utilities__from_address__default__default_ipv6)
{
    BOOST_REQUIRE_EQUAL(from_address(messages::ip_address{}), asio::ipv6{});
}

// is_valid

BOOST_AUTO_TEST_CASE(utilities__is_valid__default__false)
{
    BOOST_REQUIRE(!is_valid(messages::address_item{}));
}

BOOST_AUTO_TEST_SUITE_END()
