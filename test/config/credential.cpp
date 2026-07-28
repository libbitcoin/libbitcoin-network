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
#include <sstream>

BOOST_AUTO_TEST_SUITE(credential_tests)

using namespace config;
using namespace boost::program_options;

// sha256("Basic " + base64("username:password"))
constexpr auto test_digest = system::base16_array("4240434cbe9ea94026f98f66517966155d06954b15b2f8912f55948dd70569a1");

// sha256("Basic " + base64(":"))
constexpr auto test_empty_digest = system::base16_array("d9fa291efa0c924851f3ea14e73b1847506ab451fb834b7101e5a1a88f94f500");

// construct

BOOST_AUTO_TEST_CASE(credential__construct__default__empty)
{
    const credential instance{};
    BOOST_REQUIRE(instance.username().empty());
    BOOST_REQUIRE(instance.password().empty());
    BOOST_REQUIRE(instance.methods().empty());
    BOOST_REQUIRE_EQUAL(instance.digest(), system::hash_digest{});
}

BOOST_AUTO_TEST_CASE(credential__construct__untokenized__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(credential instance("username"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(credential__construct__overtokenized__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(credential instance("username:password:getblock:extra"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(credential__construct__empty_tokens__expected_digest)
{
    const credential instance{ ":" };
    BOOST_REQUIRE(instance.username().empty());
    BOOST_REQUIRE(instance.password().empty());
    BOOST_REQUIRE(instance.methods().empty());
    BOOST_REQUIRE_EQUAL(instance.digest(), test_empty_digest);
}

BOOST_AUTO_TEST_CASE(credential__construct__unmethoded__expected_digest)
{
    const credential instance{ "username:password" };
    BOOST_REQUIRE_EQUAL(instance.username(), "username");
    BOOST_REQUIRE_EQUAL(instance.password(), "password");
    BOOST_REQUIRE(instance.methods().empty());
    BOOST_REQUIRE_EQUAL(instance.digest(), test_digest);
}

BOOST_AUTO_TEST_CASE(credential__construct__methoded__expected_digest)
{
    const credential instance{ "username:password:getblock,getblockcount" };
    BOOST_REQUIRE_EQUAL(instance.username(), "username");
    BOOST_REQUIRE_EQUAL(instance.password(), "password");
    BOOST_REQUIRE_EQUAL(instance.digest(), test_digest);
    BOOST_REQUIRE_EQUAL(instance.methods().size(), 2u);
    BOOST_REQUIRE_EQUAL(instance.methods().front(), "getblock");
    BOOST_REQUIRE_EQUAL(instance.methods().back(), "getblockcount");
}

BOOST_AUTO_TEST_CASE(credential__construct__empty_methods__unmethoded)
{
    const credential instance{ "username:password:" };
    BOOST_REQUIRE(instance.methods().empty());
    BOOST_REQUIRE_EQUAL(instance.digest(), test_digest);
}

// permitted

BOOST_AUTO_TEST_CASE(credential__permitted__unmethoded__true)
{
    const credential instance{ "username:password" };
    BOOST_REQUIRE(instance.permitted("getblock"));
    BOOST_REQUIRE(instance.permitted(""));
}

BOOST_AUTO_TEST_CASE(credential__permitted__methoded__expected)
{
    const credential instance{ "username:password:getblock,getblockcount" };
    BOOST_REQUIRE(instance.permitted("getblock"));
    BOOST_REQUIRE(instance.permitted("getblockcount"));
    BOOST_REQUIRE(!instance.permitted("getblockhash"));
    BOOST_REQUIRE(!instance.permitted(""));
}

// to_string

BOOST_AUTO_TEST_CASE(credential__to_string__unmethoded__round_trips)
{
    const credential instance{ "username:password" };
    BOOST_REQUIRE_EQUAL(instance.to_string(), "username:password");
}

BOOST_AUTO_TEST_CASE(credential__to_string__methoded__round_trips)
{
    const credential instance{ "username:password:getblock,getblockcount" };
    BOOST_REQUIRE_EQUAL(instance.to_string(), "username:password:getblock,getblockcount");
}

// operators

BOOST_AUTO_TEST_CASE(credential__istream__methoded__expected)
{
    credential instance{};
    std::stringstream("username:password:getblock") >> instance;
    BOOST_REQUIRE_EQUAL(instance.digest(), test_digest);
    BOOST_REQUIRE_EQUAL(instance.methods().size(), 1u);
    BOOST_REQUIRE_EQUAL(instance.methods().front(), "getblock");
}

BOOST_AUTO_TEST_CASE(credential__ostream__methoded__expected)
{
    std::stringstream out{};
    out << credential{ "username:password:getblock" };
    BOOST_REQUIRE_EQUAL(out.str(), "username:password:getblock");
}

BOOST_AUTO_TEST_SUITE_END()
