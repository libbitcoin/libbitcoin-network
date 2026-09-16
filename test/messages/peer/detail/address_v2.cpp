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
#include "../../../test.hpp"

BOOST_AUTO_TEST_SUITE(p2p_address_v2_tests)

using namespace bc::system;
using namespace network::messages::peer;

constexpr auto mapped = base16_array("00000000000000000000ffff01020304");
constexpr auto onion = base16_array("79bcc625184b05194975c28b66b66b0469f7f6556fb1ac3189a79b40dda32f1f");

BOOST_AUTO_TEST_CASE(address_v2__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(address_v2::command, "addrv2");
    constexpr auto index = messages::peer::registry::index_of<address_v2>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), address_v2::command);
    BOOST_REQUIRE_EQUAL(address_v2::identifier, identifiers::address_v2);
    BOOST_REQUIRE_EQUAL(address_v2::version_minimum, level::bip155);
    BOOST_REQUIRE_EQUAL(address_v2::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(address_v2__size__default__expected)
{
    constexpr auto expected = variable_size(zero);
    BOOST_REQUIRE_EQUAL(address_v2{}.size(level::bip155), expected);
}

BOOST_AUTO_TEST_CASE(address_v2__size__two__expected)
{
    const address_v2 message{ { { 1, 1, ipv4_t{ mapped }, 8333 }, { 1, 1, torv3_t{ onion }, 8333 } } };
    BOOST_REQUIRE_EQUAL(message.size(level::bip155), variable_size(two) + 13u + 41u);
}

BOOST_AUTO_TEST_CASE(address_v2__serialize__two__expected)
{
    const address_v2 message{ { { 0x12345678_u32, 1, ipv4_t{ mapped }, 8333 }, { 0x12345678_u32, 1, torv3_t{ onion }, 8333 } } };
    data_chunk data(message.size(level::bip155));
    BOOST_REQUIRE(message.serialize(level::bip155, data));
    BOOST_REQUIRE_EQUAL(data, base16_chunk("027856341201010401020304208d7856341201042079bcc625184b05194975c28b66b66b0469f7f6556fb1ac3189a79b40dda32f1f208d"));
}

BOOST_AUTO_TEST_CASE(address_v2__deserialize__empty__expected)
{
    constexpr auto payload = base16_array("00");
    system::read::bytes::copy source(payload);
    const auto message = address_v2::deserialize(level::bip155, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE(message.addresses.empty());
}

BOOST_AUTO_TEST_CASE(address_v2__deserialize__two__expected)
{
    constexpr auto payload = base16_array("027856341201010401020304208d7856341201042079bcc625184b05194975c28b66b66b0469f7f6556fb1ac3189a79b40dda32f1f208d");
    system::read::bytes::copy source(payload);
    const auto message = address_v2::deserialize(level::bip155, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE(source.is_exhausted());
    BOOST_REQUIRE_EQUAL(message.addresses.size(), two);
    BOOST_REQUIRE(message.addresses.front().address == address_t{ ipv4_t{ mapped } });
    BOOST_REQUIRE(message.addresses.back().address == address_t{ torv3_t{ onion } });
}
BOOST_AUTO_TEST_CASE(address_v2__deserialize__insufficient_version__invalid)
{
    constexpr auto payload = base16_array("00");
    system::read::bytes::copy source(payload);
    address_v2::deserialize(level::bip155 - 1u, source);
    BOOST_REQUIRE(!source);
}

BOOST_AUTO_TEST_SUITE_END()
