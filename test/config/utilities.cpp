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

// denormalize

BOOST_AUTO_TEST_CASE(utilities__denormalize__defaults__unchanged)
{
    BOOST_REQUIRE(denormalize({ asio::ipv4{} }).is_v4());
    BOOST_REQUIRE(denormalize({ asio::ipv6{} }).is_v6());
}

BOOST_AUTO_TEST_CASE(utilities__denormalize__mapped__unmapped)
{
    constexpr system::data_array<16> mapped
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 1, 2, 3, 4
    };

    const asio::address ip{ asio::ipv6{ mapped } };
    BOOST_REQUIRE(denormalize(ip).is_v4());
}

BOOST_AUTO_TEST_CASE(utilities__denormalize__unmapped__unchanged)
{
    constexpr system::data_array<16> unmapped
    {
        0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xab, 0xcd, 1, 2, 3, 4
    };

    const asio::address ip{ asio::ipv6{ unmapped } };
    BOOST_REQUIRE(denormalize(ip) == ip);
}

// is_valid

BOOST_AUTO_TEST_CASE(utilities__is_valid__default__false)
{
    BOOST_REQUIRE(!is_valid(messages::address_item{}));
}

BOOST_AUTO_TEST_CASE(utilities__is_valid__loopback__true)
{
    const messages::address_item item{ 0, 0, messages::loopback_ip_address, 42 };
    BOOST_REQUIRE(is_valid(item));
}

// is_v4

BOOST_AUTO_TEST_CASE(utilities__is_v4__default__false)
{
    BOOST_REQUIRE(!is_v4(messages::ip_address{}));
}

BOOST_AUTO_TEST_CASE(utilities__is_v4__loopback_v6__false)
{
    BOOST_REQUIRE(!is_v4(messages::loopback_ip_address));
}

BOOST_AUTO_TEST_CASE(utilities__is_v4__loopback_mapped__true)
{
    constexpr system::data_array<16> loopback_mapped
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 127, 0, 0, 1
    };

    BOOST_REQUIRE(is_v4(loopback_mapped));
}


// is_member

BOOST_AUTO_TEST_CASE(utilities__is_member__defaults_zero__false)
{
    BOOST_REQUIRE(!is_member(asio::address{}, asio::address{}, 0));
}

BOOST_AUTO_TEST_CASE(utilities__is_member__defaults_nonzero__false)
{
    BOOST_REQUIRE(!is_member(asio::address{}, asio::address{}, 1));
}

BOOST_AUTO_TEST_CASE(utilities__is_member__ipv4_defaults_nonzero__false)
{
    BOOST_REQUIRE(!is_member(asio::ipv4{}, asio::ipv4{}, 24));
}

BOOST_AUTO_TEST_CASE(utilities__is_member__ipv6_defaults_nonzero__true)
{
    BOOST_REQUIRE(is_member(asio::ipv6{}, asio::ipv6{}, 56));
}

using namespace boost::asio::ip;

BOOST_AUTO_TEST_CASE(utilities__is_member__zero_cidr__expected)
{
    // zero CIDR valid for ipv4, invalid for ipv6
    BOOST_REQUIRE(is_member(make_address_v4("42.42.42.42"), make_address_v4("99.99.99.99"), 0));
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("9999:9999:9999:9999:9999:9999:9999:9999"), 0));
}

BOOST_AUTO_TEST_CASE(utilities__is_member__ipv4_member__true)
{
    BOOST_REQUIRE(is_member(make_address_v4("42.42.42.42"), make_address_v4("42.99.99.99"), 8));
    BOOST_REQUIRE(is_member(make_address_v4("42.42.42.42"), make_address_v4("42.42.99.99"), 16));
    BOOST_REQUIRE(is_member(make_address_v4("42.42.42.42"), make_address_v4("42.42.42.99"), 24));
    BOOST_REQUIRE(is_member(make_address_v4("42.42.42.42"), make_address_v4("42.42.42.42"), 32));
}

BOOST_AUTO_TEST_CASE(utilities__is_member__not_ipv4_member__false)
{
    BOOST_REQUIRE(!is_member(make_address_v4("42.42.42.42"), make_address_v4("99.99.99.99"), 8));
    BOOST_REQUIRE(!is_member(make_address_v4("42.42.42.42"), make_address_v4("42.99.99.99"), 16));
    BOOST_REQUIRE(!is_member(make_address_v4("42.42.42.42"), make_address_v4("42.42.99.99"), 24));
    BOOST_REQUIRE(!is_member(make_address_v4("42.42.42.42"), make_address_v4("42.42.42.99"), 32));
}

BOOST_AUTO_TEST_CASE(utilities__is_member__ipv6_non_member__true)
{
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("ab99:9999:9999:9999:9999:9999:9999:9999"), 8));
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:9999:9999:9999:9999:9999:9999:9999"), 16));
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:ab99:9999:9999:9999:9999:9999:9999"), 24));
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:9999:9999:9999:9999:9999:9999"), 32));
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:ab99:9999:9999:9999:9999:9999"), 40));
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:9999:9999:9999:9999:9999"), 48));
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:ab99:9999:9999:9999:9999"), 56));
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:9999:9999:9999:9999"), 64));
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:ab99:9999:9999:9999"), 72));
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:abcd:9999:9999:9999"), 80));
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:abcd:ab99:9999:9999"), 88));
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:9999:9999"), 96));
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:ab99:9999"), 104));
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:9999"), 112));
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:ab99"), 120));
    BOOST_REQUIRE(is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), 128));
}

BOOST_AUTO_TEST_CASE(utilities__is_member__ipv6_non_member__false)
{
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("ab99:9999:9999:9999:9999:9999:9999:9999"), 16));
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:9999:9999:9999:9999:9999:9999:9999"), 24));
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:ab99:9999:9999:9999:9999:9999:9999"), 32));
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:9999:9999:9999:9999:9999:9999"), 40));
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:ab99:9999:9999:9999:9999:9999"), 48));
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:9999:9999:9999:9999:9999"), 56));
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:ab99:9999:9999:9999:9999"), 64));
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:9999:9999:9999:9999"), 72));
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:ab99:9999:9999:9999"), 80));
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:abcd:9999:9999:9999"), 88));
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:abcd:ab99:9999:9999"), 96));
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:9999:9999"), 104));
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:ab99:9999"), 112));
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:9999"), 120));
    BOOST_REQUIRE(!is_member(make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address_v6("abcd:abcd:abcd:abcd:abcd:abcd:abcd:ab99"), 128));
}

BOOST_AUTO_TEST_SUITE_END()
