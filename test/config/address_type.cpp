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

using namespace config;
using namespace network::messages::peer;

BOOST_AUTO_TEST_SUITE(address_type_tests)

// address_types

BOOST_AUTO_TEST_CASE(address_type__address_types__always__bip155_networks)
{
    BOOST_REQUIRE_EQUAL(address_types, 7u);
    BOOST_REQUIRE_EQUAL(address_counts{}.size(), address_types);
}

// The alternative index is the BIP155 network identifier.

BOOST_AUTO_TEST_CASE(address_type__index__unspecified__zero)
{
    BOOST_REQUIRE_EQUAL(address_t{}.index(), 0u);
}

BOOST_AUTO_TEST_CASE(address_type__index__ipv4__one)
{
    BOOST_REQUIRE_EQUAL(address_t{ ipv4_t{} }.index(), 1u);
}

BOOST_AUTO_TEST_CASE(address_type__index__ipv6__two)
{
    BOOST_REQUIRE_EQUAL(address_t{ ipv6_t{} }.index(), 2u);
}

BOOST_AUTO_TEST_CASE(address_type__index__torv2__three)
{
    BOOST_REQUIRE_EQUAL(address_t{ torv2_t{} }.index(), 3u);
}

BOOST_AUTO_TEST_CASE(address_type__index__torv3__four)
{
    BOOST_REQUIRE_EQUAL(address_t{ torv3_t{} }.index(), 4u);
}

BOOST_AUTO_TEST_CASE(address_type__index__i2p__five)
{
    BOOST_REQUIRE_EQUAL(address_t{ i2p_t{} }.index(), 5u);
}

BOOST_AUTO_TEST_CASE(address_type__index__cjdns__six)
{
    BOOST_REQUIRE_EQUAL(address_t{ cjdns_t{} }.index(), 6u);
}

// to_address

BOOST_AUTO_TEST_CASE(address_type__to_address__loopback_v6__ipv6)
{
    BOOST_REQUIRE(to_address(loopback_ip_address) == address_t{ ipv6_t{ loopback_ip_address } });
}

BOOST_AUTO_TEST_CASE(address_type__to_address__loopback_mapped__ipv4)
{
    constexpr ip_address mapped
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 127, 0, 0, 1
    };

    BOOST_REQUIRE(to_address(mapped) == address_t{ ipv4_t{ mapped } });
}

// to_ip_address

BOOST_AUTO_TEST_CASE(address_type__to_ip_address__ipv6__expected)
{
    BOOST_REQUIRE_EQUAL(to_ip_address(address_t{ ipv6_t{ loopback_ip_address } }), loopback_ip_address);
}

BOOST_AUTO_TEST_CASE(address_type__to_ip_address__unspecified__unspecified)
{
    BOOST_REQUIRE_EQUAL(to_ip_address(address_t{}), unspecified_ip_address);
}

BOOST_AUTO_TEST_CASE(address_type__to_ip_address__torv3__unspecified)
{
    BOOST_REQUIRE_EQUAL(to_ip_address(address_t{ torv3_t{} }), unspecified_ip_address);
}

// is_unspecified

BOOST_AUTO_TEST_CASE(address_type__is_unspecified__default__true)
{
    BOOST_REQUIRE(is_unspecified(address_t{}));
}

BOOST_AUTO_TEST_CASE(address_type__is_unspecified__zero_ipv6__true)
{
    BOOST_REQUIRE(is_unspecified(address_t{ ipv6_t{ unspecified_ip_address } }));
}

BOOST_AUTO_TEST_CASE(address_type__is_unspecified__loopback__false)
{
    BOOST_REQUIRE(!is_unspecified(address_t{ ipv6_t{ loopback_ip_address } }));
}

BOOST_AUTO_TEST_SUITE_END()
