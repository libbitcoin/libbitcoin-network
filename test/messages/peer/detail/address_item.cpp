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

BOOST_AUTO_TEST_SUITE(p2p_address_item_tests)

using namespace bc::system;
using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(address_item__loopback_ip_address__always__expected)
{
    constexpr ip_address expected
    {
        {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
        }
    };


    BOOST_REQUIRE_EQUAL(loopback_ip_address, expected);
}

// is_v4/is_v6

constexpr ip_address mapped_ip_address
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 127, 0, 0, 1
};

BOOST_AUTO_TEST_CASE(address_item__is_v4__default__false)
{
    BOOST_REQUIRE(!is_v4(ip_address{}));
}

BOOST_AUTO_TEST_CASE(address_item__is_v4__loopback_v6__false)
{
    BOOST_REQUIRE(!is_v4(loopback_ip_address));
}

BOOST_AUTO_TEST_CASE(address_item__is_v4__loopback_mapped__true)
{
    BOOST_REQUIRE(is_v4(mapped_ip_address));
}

BOOST_AUTO_TEST_CASE(address_item__is_v6__default__true)
{
    BOOST_REQUIRE(is_v6(ip_address{}));
}

BOOST_AUTO_TEST_CASE(address_item__is_v6__loopback_v6__true)
{
    BOOST_REQUIRE(is_v6(loopback_ip_address));
}

BOOST_AUTO_TEST_CASE(address_item__is_v6__loopback_mapped__false)
{
    BOOST_REQUIRE(!is_v6(mapped_ip_address));
}

BOOST_AUTO_TEST_CASE(address_item__is_v4__ipv4_address__true)
{
    BOOST_REQUIRE(is_v4(address_t{ ipv4_t{ mapped_ip_address } }));
    BOOST_REQUIRE(!is_v4(address_t{ ipv6_t{ loopback_ip_address } }));
    BOOST_REQUIRE(!is_v4(address_t{}));
}

BOOST_AUTO_TEST_CASE(address_item__is_v6__ipv6_address__true)
{
    BOOST_REQUIRE(is_v6(address_t{ ipv6_t{ loopback_ip_address } }));
    BOOST_REQUIRE(!is_v6(address_t{ ipv4_t{ mapped_ip_address } }));
    BOOST_REQUIRE(!is_v6(address_t{}));
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
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
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
    BOOST_REQUIRE(is_unspecified(unspecified_address_item.address));
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

// is_specified

BOOST_AUTO_TEST_CASE(address_item__is_specified__default__false)
{
    BOOST_REQUIRE(!is_specified(messages::peer::address_item{}));
}

BOOST_AUTO_TEST_CASE(address_item__is_specified__loopback__true)
{
    const messages::peer::address_item item{ 0, 0, messages::peer::ipv6_t{ messages::peer::loopback_ip_address }, 42 };
    BOOST_REQUIRE(is_specified(item));
}

BOOST_AUTO_TEST_CASE(address_item__is_specified__i2p_zero_port__true)
{
    const messages::peer::address_item item{ 0, 0, messages::peer::i2p_t{ { 0x01 } }, 0 };
    BOOST_REQUIRE(is_specified(item));
}

BOOST_AUTO_TEST_CASE(address_item__is_specified__i2p_unspecified__false)
{
    const messages::peer::address_item item{ 0, 0, messages::peer::i2p_t{}, 0 };
    BOOST_REQUIRE(!is_specified(item));
}

BOOST_AUTO_TEST_CASE(address_item__is_specified__onion_zero_port__false)
{
    const messages::peer::address_item item{ 0, 0, messages::peer::torv3_t{ { 0x01 } }, 0 };
    BOOST_REQUIRE(!is_specified(item));
}

// equality

BOOST_AUTO_TEST_CASE(address_item__equality__default_default__true)
{
    const address_item item1{};
    const address_item item2{};
    BOOST_REQUIRE(item1 == item2);
}

BOOST_AUTO_TEST_CASE(address_item__equality__same__true)
{
    constexpr address_item item1{ 1, 2, ipv6_t{ unspecified_ip_address }, 3 };
    constexpr address_item item2{ 1, 2, ipv6_t{ unspecified_ip_address }, 3 };
    BOOST_REQUIRE(item1 == item2);
}

BOOST_AUTO_TEST_CASE(address_item__equality__distinct_port__false)
{
    constexpr address_item item1{ 1, 2, ipv6_t{ unspecified_ip_address }, 3 };
    constexpr address_item item2{ 1, 2, ipv6_t{ unspecified_ip_address }, 4 };
    BOOST_REQUIRE(!(item1 == item2));
}

BOOST_AUTO_TEST_CASE(address_item__equality__distinct_ip__false)
{
    constexpr address_item item1{ 1, 2, ipv6_t{ loopback_ip_address }, 3 };
    constexpr address_item item2{ 1, 2, ipv6_t{ unspecified_ip_address }, 3 };
    BOOST_REQUIRE(!(item1 == item2));
}

BOOST_AUTO_TEST_CASE(address_item__equality__distinct_services__true)
{
    constexpr address_item item1{ 1, 2, ipv6_t{ unspecified_ip_address }, 3 };
    constexpr address_item item2{ 1, 4, ipv6_t{ unspecified_ip_address }, 3 };
    BOOST_REQUIRE(item1 == item2);
}

BOOST_AUTO_TEST_CASE(address_item__equality__distinct_timestamp__true)
{
    constexpr address_item item1{ 1, 2, ipv6_t{ unspecified_ip_address }, 3 };
    constexpr address_item item2{ 4, 2, ipv6_t{ unspecified_ip_address }, 3 };
    BOOST_REQUIRE(item1 == item2);
}

// inequality

BOOST_AUTO_TEST_CASE(address_item__inequality__default_default__false)
{
    const address_item item1{};
    const address_item item2{};
    BOOST_REQUIRE(!(item1 != item2));
}

BOOST_AUTO_TEST_CASE(address_item__inequality__same__false)
{
    constexpr address_item item1{ 1, 2, ipv6_t{ unspecified_ip_address }, 3 };
    constexpr address_item item2{ 1, 2, ipv6_t{ unspecified_ip_address }, 3 };
    BOOST_REQUIRE(!(item1 != item2));
}

BOOST_AUTO_TEST_CASE(address_item__inequality__distinct_port__true)
{
    constexpr address_item item1{ 1, 2, ipv6_t{ unspecified_ip_address }, 3 };
    constexpr address_item item2{ 1, 2, ipv6_t{ unspecified_ip_address }, 4 };
    BOOST_REQUIRE(item1 != item2);
}

BOOST_AUTO_TEST_CASE(address_item__inequality__distinct_ip__true)
{
    constexpr address_item item1{ 1, 2, ipv6_t{ loopback_ip_address }, 3 };
    constexpr address_item item2{ 1, 2, ipv6_t{ unspecified_ip_address }, 3 };
    BOOST_REQUIRE(item1 != item2);
}

BOOST_AUTO_TEST_CASE(address_item__inequality__distinct_services__false)
{
    constexpr address_item item1{ 1, 2, ipv6_t{ unspecified_ip_address }, 3 };
    constexpr address_item item2{ 1, 4, ipv6_t{ unspecified_ip_address }, 3 };
    BOOST_REQUIRE(!(item1 != item2));
}

BOOST_AUTO_TEST_CASE(address_item__inequality__distinct_timestamp__false)
{
    constexpr address_item item1{ 1, 2, ipv6_t{ unspecified_ip_address }, 3 };
    constexpr address_item item2{ 4, 2, ipv6_t{ unspecified_ip_address }, 3 };
    BOOST_REQUIRE(!(item1 != item2));
}

// address v2 (BIP155) entry codec

constexpr auto v2_ipv4 = base16_array("00000000000000000000ffff01020304");
constexpr auto v2_ipv6 = base16_array("1a1b2a2b3a3b4a4b5a5b6a6b7a7b8a8b");
constexpr auto v2_torv2 = base16_array("f1f2f3f4f5f6f7f8f9fa");
constexpr auto v2_torv3 = base16_array("79bcc625184b05194975c28b66b66b0469f7f6556fb1ac3189a79b40dda32f1f");
constexpr auto v2_i2p = base16_array("a2894dabaec08c0051a481a6dac88b64f98232ae42d4b6fd2fa81952dfe36a87");
constexpr auto v2_cjdns = base16_array("fc000001000200030004000500060007");

BOOST_AUTO_TEST_CASE(address_item__size_v2__unspecified__expected)
{
    const address_item item{ 1, 0, {}, 0 };
    BOOST_REQUIRE_EQUAL(item.size_v2(level::bip155), 9u);
}

BOOST_AUTO_TEST_CASE(address_item__size_v2__ipv4__expected)
{
    const address_item item{ 1, 1, ipv4_t{ v2_ipv4 }, 8333 };
    BOOST_REQUIRE_EQUAL(item.size_v2(level::bip155), 13u);
}

BOOST_AUTO_TEST_CASE(address_item__size_v2__ipv6__expected)
{
    const address_item item{ 1, 1, ipv6_t{ v2_ipv6 }, 8333 };
    BOOST_REQUIRE_EQUAL(item.size_v2(level::bip155), 25u);
}

BOOST_AUTO_TEST_CASE(address_item__size_v2__torv2__expected)
{
    const address_item item{ 1, 1, torv2_t{ v2_torv2 }, 8333 };
    BOOST_REQUIRE_EQUAL(item.size_v2(level::bip155), 19u);
}

BOOST_AUTO_TEST_CASE(address_item__size_v2__torv3__expected)
{
    const address_item item{ 1, 1, torv3_t{ v2_torv3 }, 8333 };
    BOOST_REQUIRE_EQUAL(item.size_v2(level::bip155), 41u);
}

BOOST_AUTO_TEST_CASE(address_item__size_v2__i2p__expected)
{
    const address_item item{ 1, 1, i2p_t{ v2_i2p }, 8333 };
    BOOST_REQUIRE_EQUAL(item.size_v2(level::bip155), 41u);
}

BOOST_AUTO_TEST_CASE(address_item__size_v2__cjdns__expected)
{
    const address_item item{ 1, 1, cjdns_t{ v2_cjdns }, 8333 };
    BOOST_REQUIRE_EQUAL(item.size_v2(level::bip155), 25u);
}

BOOST_AUTO_TEST_CASE(address_item__size_v2__large_services__variable_encoded)
{
    const address_item item{ 1, 0xfd, ipv4_t{ v2_ipv4 }, 8333 };
    BOOST_REQUIRE_EQUAL(item.size_v2(level::bip155), 15u);
}

// serialize_v2

BOOST_AUTO_TEST_CASE(address_item__serialize_v2__unspecified__reserved_network_empty)
{
    const address_item item{ 1, 0, {}, 0 };
    data_chunk data(item.size_v2(level::bip155));
    system::write::bytes::copy sink(data);
    item.serialize_v2(level::bip155, sink);
    BOOST_REQUIRE(sink);
    BOOST_REQUIRE_EQUAL(data, base16_chunk("010000000000000000"));
}

BOOST_AUTO_TEST_CASE(address_item__serialize_v2__ipv4__four_byte_address)
{
    const address_item item{ 0x12345678_u32, 1, ipv4_t{ v2_ipv4 }, 8333 };
    data_chunk data(item.size_v2(level::bip155));
    system::write::bytes::copy sink(data);
    item.serialize_v2(level::bip155, sink);
    BOOST_REQUIRE(sink);
    BOOST_REQUIRE_EQUAL(data, base16_chunk("7856341201010401020304208d"));
}

BOOST_AUTO_TEST_CASE(address_item__serialize_v2__ipv6__expected)
{
    const address_item item{ 0x12345678_u32, 1, ipv6_t{ v2_ipv6 }, 8333 };
    data_chunk data(item.size_v2(level::bip155));
    system::write::bytes::copy sink(data);
    item.serialize_v2(level::bip155, sink);
    BOOST_REQUIRE(sink);
    BOOST_REQUIRE_EQUAL(data, base16_chunk("785634120102101a1b2a2b3a3b4a4b5a5b6a6b7a7b8a8b208d"));
}

BOOST_AUTO_TEST_CASE(address_item__serialize_v2__torv3__expected)
{
    const address_item item{ 0x12345678_u32, 1, torv3_t{ v2_torv3 }, 8333 };
    data_chunk data(item.size_v2(level::bip155));
    system::write::bytes::copy sink(data);
    item.serialize_v2(level::bip155, sink);
    BOOST_REQUIRE(sink);
    BOOST_REQUIRE_EQUAL(data, base16_chunk("7856341201042079bcc625184b05194975c28b66b66b0469f7f6556fb1ac3189a79b40dda32f1f208d"));
}

BOOST_AUTO_TEST_CASE(address_item__serialize_v2__i2p__expected)
{
    const address_item item{ 0x12345678_u32, 1, i2p_t{ v2_i2p }, 8333 };
    data_chunk data(item.size_v2(level::bip155));
    system::write::bytes::copy sink(data);
    item.serialize_v2(level::bip155, sink);
    BOOST_REQUIRE(sink);
    BOOST_REQUIRE_EQUAL(data, base16_chunk("78563412010520a2894dabaec08c0051a481a6dac88b64f98232ae42d4b6fd2fa81952dfe36a87208d"));
}

BOOST_AUTO_TEST_CASE(address_item__serialize_v2__cjdns__expected)
{
    const address_item item{ 0x12345678_u32, 1, cjdns_t{ v2_cjdns }, 8333 };
    data_chunk data(item.size_v2(level::bip155));
    system::write::bytes::copy sink(data);
    item.serialize_v2(level::bip155, sink);
    BOOST_REQUIRE(sink);
    BOOST_REQUIRE_EQUAL(data, base16_chunk("78563412010610fc000001000200030004000500060007208d"));
}

// deserialize_v2

BOOST_AUTO_TEST_CASE(address_item__deserialize_v2__ipv4__mapped_address)
{
    constexpr auto payload = base16_array("7856341201010401020304208d");
    system::read::bytes::copy source(payload);
    const auto item = address_item::deserialize_v2(level::bip155, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE(source.is_exhausted());
    BOOST_REQUIRE_EQUAL(item.timestamp, 0x12345678_u32);
    BOOST_REQUIRE_EQUAL(item.services, 1u);
    BOOST_REQUIRE_EQUAL(item.port, 8333u);
    BOOST_REQUIRE(item.address == address_t{ ipv4_t{ v2_ipv4 } });
}

BOOST_AUTO_TEST_CASE(address_item__deserialize_v2__ipv6__expected)
{
    constexpr auto payload = base16_array("785634120102101a1b2a2b3a3b4a4b5a5b6a6b7a7b8a8b208d");
    system::read::bytes::copy source(payload);
    const auto item = address_item::deserialize_v2(level::bip155, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE(source.is_exhausted());
    BOOST_REQUIRE(item.address == address_t{ ipv6_t{ v2_ipv6 } });
}

BOOST_AUTO_TEST_CASE(address_item__deserialize_v2__torv2__expected)
{
    constexpr auto payload = base16_array("7856341201030af1f2f3f4f5f6f7f8f9fa208d");
    system::read::bytes::copy source(payload);
    const auto item = address_item::deserialize_v2(level::bip155, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE(item.address == address_t{ torv2_t{ v2_torv2 } });
}

BOOST_AUTO_TEST_CASE(address_item__deserialize_v2__torv3__expected)
{
    constexpr auto payload = base16_array("7856341201042079bcc625184b05194975c28b66b66b0469f7f6556fb1ac3189a79b40dda32f1f208d");
    system::read::bytes::copy source(payload);
    const auto item = address_item::deserialize_v2(level::bip155, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE(source.is_exhausted());
    BOOST_REQUIRE(item.address == address_t{ torv3_t{ v2_torv3 } });
}

BOOST_AUTO_TEST_CASE(address_item__deserialize_v2__i2p__expected)
{
    constexpr auto payload = base16_array("78563412010520a2894dabaec08c0051a481a6dac88b64f98232ae42d4b6fd2fa81952dfe36a87208d");
    system::read::bytes::copy source(payload);
    const auto item = address_item::deserialize_v2(level::bip155, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE(item.address == address_t{ i2p_t{ v2_i2p } });
}

BOOST_AUTO_TEST_CASE(address_item__deserialize_v2__cjdns__expected)
{
    constexpr auto payload = base16_array("78563412010610fc000001000200030004000500060007208d");
    system::read::bytes::copy source(payload);
    const auto item = address_item::deserialize_v2(level::bip155, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE(item.address == address_t{ cjdns_t{ v2_cjdns } });
}

BOOST_AUTO_TEST_CASE(address_item__deserialize_v2__unknown_network__unspecified_and_consumed)
{
    constexpr auto payload = base16_array("78563412014202dead208d");
    system::read::bytes::copy source(payload);
    const auto item = address_item::deserialize_v2(level::bip155, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE(source.is_exhausted());
    BOOST_REQUIRE(is_unspecified(item.address));
    BOOST_REQUIRE_EQUAL(item.port, 8333u);
}

BOOST_AUTO_TEST_CASE(address_item__deserialize_v2__reserved_network__unspecified_and_consumed)
{
    constexpr auto payload = base16_array("78563412010000208d");
    system::read::bytes::copy source(payload);
    const auto item = address_item::deserialize_v2(level::bip155, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE(source.is_exhausted());
    BOOST_REQUIRE(is_unspecified(item.address));
    BOOST_REQUIRE_EQUAL(item.port, 8333u);
}

BOOST_AUTO_TEST_CASE(address_item__deserialize_v2__short_ipv4__invalid)
{
    constexpr auto payload = base16_array("78563412010103010203208d");
    system::read::bytes::copy source(payload);
    address_item::deserialize_v2(level::bip155, source);
    BOOST_REQUIRE(!source);
}

BOOST_AUTO_TEST_CASE(address_item__deserialize_v2__short_torv3__invalid)
{
    constexpr auto payload = base16_array("7856341201041079bcc625184b05194975c28b66b66b04208d");
    system::read::bytes::copy source(payload);
    address_item::deserialize_v2(level::bip155, source);
    BOOST_REQUIRE(!source);
}

BOOST_AUTO_TEST_CASE(address_item__deserialize_v2__oversized_address__invalid)
{
    constexpr auto payload = base16_array("78563412014afd0102");
    system::read::bytes::copy source(payload);
    address_item::deserialize_v2(level::bip155, source);
    BOOST_REQUIRE(!source);
}

BOOST_AUTO_TEST_SUITE_END()
