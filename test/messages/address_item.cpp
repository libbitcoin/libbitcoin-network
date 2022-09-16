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

BOOST_AUTO_TEST_SUITE(address_item_tests)

using namespace bc::network::messages;

BOOST_AUTO_TEST_CASE(address_item__null_ip_address__always__expected)
{
    constexpr ip_address expected
    {
        {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        }
    };


    BOOST_REQUIRE_EQUAL(null_ip_address, expected);
}

BOOST_AUTO_TEST_CASE(address_item__unspecified_timestamp__always__expected)
{
    BOOST_REQUIRE_EQUAL(unspecified_timestamp, 0u);
}

BOOST_AUTO_TEST_CASE(address_item__unspecified_ip_address__always__expected)
{
    constexpr ip_address expected
    {
        {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00
        }
    };

    BOOST_REQUIRE_EQUAL(unspecified_ip_address, expected);
}

BOOST_AUTO_TEST_CASE(address_item__unspecified_ip_port__always__expected)
{
    BOOST_REQUIRE_EQUAL(unspecified_ip_port, 0u);
}

BOOST_AUTO_TEST_CASE(address_item__unspecified_address_item__always__expected)
{
    BOOST_REQUIRE_EQUAL(unspecified_address_item.timestamp, unspecified_timestamp);
    BOOST_REQUIRE_EQUAL(unspecified_address_item.services, service::node_none);
    BOOST_REQUIRE_EQUAL(unspecified_address_item.ip, unspecified_ip_address);
    BOOST_REQUIRE_EQUAL(unspecified_address_item.port, unspecified_ip_port);
}

BOOST_AUTO_TEST_CASE(address_item__size__with_timestamp__expected)
{
    constexpr auto expected = sizeof(uint32_t)
        + sizeof(uint64_t)
        + std::tuple_size<ip_address>::value
        + sizeof(uint16_t);

    BOOST_REQUIRE_EQUAL(address_item::size(level::canonical, true), expected);
}

BOOST_AUTO_TEST_CASE(address_item__size__without_timestamp__expected)
{
    constexpr auto expected = sizeof(uint64_t)
        + std::tuple_size<ip_address>::value
        + sizeof(uint16_t);

    BOOST_REQUIRE_EQUAL(address_item::size(level::canonical, false), expected);
}

BOOST_AUTO_TEST_SUITE_END()
