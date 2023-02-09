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
using namespace boost::program_options;

BOOST_AUTO_TEST_SUITE(utilities_tests)

////bool contains4(const boost::asio::ip::address& address) NOEXCEPT
////{
////    using namespace system;
////    using namespace boost::asio;
////    
////    // CIDR notation.
////
////    try
////    {
////        // Throwing.
////        // Host identifier zero implies subnet.
////        // Host identifier non-zero implies host and its subnet.
////        const ip::network_v4 subnet4 = ip::make_network_v4("192.168.0.0/29");
////
////        // hosts() excludes the network   address, not a host (eg: /29 excludes .0).
////        // hosts() excludes the broadcast address, not a host (eg: /29 excludes .7).
////        const ip::address_v4_range hosts4 = subnet4.hosts();
////        return hosts4.find(address.to_v4()) != hosts4.end();
////    }
////    catch (std::exception)
////    {
////        return false;
////    }
////}
////
////bool contains6(const boost::asio::ip::address& address) NOEXCEPT
////{
////    using namespace system;
////    using namespace boost::asio;
////
////    try
////    {
////        // Throwing.
////        const ip::network_v6 subnet6 = ip::make_network_v6("::ffff:192.168.0.0/24");
////
////        const ip::address_v6_range hosts6 = subnet6.hosts();
////        return hosts6.find(address.to_v6()) != hosts6.end();
////    }
////    catch (std::exception)
////    {
////        return false;
////    }
////}
////
////BOOST_AUTO_TEST_CASE(contains_test)
////{
////    using namespace boost::asio;
////    ////BOOST_REQUIRE(contains4(ip::make_address("192.168.0.0"))); // only valid at /32
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.1"))); // 31 (0..0=1)
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.2")));
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.3"))); // 30 (0..3=4)
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.4")));
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.5")));
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.6")));
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.7"))); // 29 (0..7=8)
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.8")));
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.9")));
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.10")));
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.11")));
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.12")));
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.13")));
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.14")));
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.15"))); // 28 (0..15=16)
////    BOOST_REQUIRE(contains4(ip::make_address("192.168.0.16")));
////
////    ////BOOST_REQUIRE(contains6(ip::make_address("::ffff:192.168.0.1")));
////}

// to_host

BOOST_AUTO_TEST_CASE(utilities__to_host__default__unspecified_v4)
{
    BOOST_REQUIRE_EQUAL(to_host(asio::address{}), "0.0.0.0");
}

// from_host

BOOST_AUTO_TEST_CASE(utilities__from_host__default__throws_invalid_option_value)
{
    BOOST_REQUIRE_THROW(from_host(std::string{}), invalid_option_value);
}

// to_literal

BOOST_AUTO_TEST_CASE(utilities__to_literal__default__unspecified_v4)
{
    BOOST_REQUIRE_EQUAL(to_host(asio::address{}), "0.0.0.0");
}

// from_literal

BOOST_AUTO_TEST_CASE(utilities__from_literal__default__throws_invalid_option_value)
{
    BOOST_REQUIRE_THROW(from_literal(std::string{}), invalid_option_value);
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

// is_mapped

BOOST_AUTO_TEST_CASE(utilities__is_mapped__default__false)
{
    BOOST_REQUIRE(!is_mapped(asio::ipv6{}));
}

BOOST_AUTO_TEST_SUITE_END()
