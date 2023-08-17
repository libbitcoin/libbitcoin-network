/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
    constexpr asio::ipv6::bytes_type mapped
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 127, 0, 0, 1
    };

    BOOST_REQUIRE(is_v4(mapped));
}

// is_v6

BOOST_AUTO_TEST_CASE(utilities__is_v6__default__true)
{
    BOOST_REQUIRE(is_v6(messages::ip_address{}));
}

BOOST_AUTO_TEST_CASE(utilities__is_v6__loopback_v6__true)
{
    BOOST_REQUIRE(is_v6(messages::loopback_ip_address));
}

BOOST_AUTO_TEST_CASE(utilities__is_v6__loopback_mapped__false)
{
    constexpr asio::ipv6::bytes_type mapped
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 127, 0, 0, 1
    };

    BOOST_REQUIRE(!is_v6(mapped));
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

// denormalize

BOOST_AUTO_TEST_CASE(utilities__denormalize__defaults__unchanged)
{
    BOOST_REQUIRE(denormalize({ asio::ipv4{} }).is_v4());
    BOOST_REQUIRE(denormalize({ asio::ipv6{} }).is_v6());
}

BOOST_AUTO_TEST_CASE(utilities__denormalize__mapped__unmapped)
{
    constexpr asio::ipv6::bytes_type mapped
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 1, 2, 3, 4
    };

    const asio::address ip{ asio::ipv6{ mapped } };
    BOOST_REQUIRE(denormalize(ip).is_v4());
}

BOOST_AUTO_TEST_CASE(utilities__denormalize__unmapped__unchanged)
{
    constexpr asio::ipv6::bytes_type unmapped
    {
        0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xab, 0xcd, 1, 2, 3, 4
    };

    const asio::address ip{ asio::ipv6{ unmapped } };
    BOOST_REQUIRE(denormalize(ip) == ip);
}

// to_host

using namespace boost::asio::ip;

BOOST_AUTO_TEST_CASE(utilities__to_host__default__unspecified_v4)
{
    // Default asio address is ipv4.
    const auto host = to_host(asio::address{});
    BOOST_REQUIRE_EQUAL(host, "0.0.0.0");
}

BOOST_AUTO_TEST_CASE(utilities__to_host__default_v4__unspecified_v4)
{
    const auto host = to_host(asio::ipv4{});
    BOOST_REQUIRE_EQUAL(host, "0.0.0.0");
}

BOOST_AUTO_TEST_CASE(utilities__to_host__default_v6__unspecified_v6)
{
    const auto host = to_host(asio::ipv6{});
    BOOST_REQUIRE_EQUAL(host, "::");
}

BOOST_AUTO_TEST_CASE(utilities__to_host__v4__expected_v4)
{
    const auto host = to_host(make_address("42.42.42.42"));
    BOOST_REQUIRE_EQUAL(host, "42.42.42.42");
}

BOOST_AUTO_TEST_CASE(utilities__to_host__v6__expected_v6)
{
    const auto host = to_host(make_address("42:42::42:42"));
    BOOST_REQUIRE_EQUAL(host, "42:42::42:42");
}

// from_host

BOOST_AUTO_TEST_CASE(utilities__from_host__empty__throws_invalid_option_value)
{
    BOOST_REQUIRE_THROW(from_host({}), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(utilities__from_host__v4_port__throws_invalid_option_value)
{
    BOOST_REQUIRE_THROW(from_host("127.0.0.1:42"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(utilities__from_host__v4_cidr__throws_invalid_option_value)
{
    BOOST_REQUIRE_THROW(from_host("127.0.0.1/24"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(utilities__from_host__v6_port__throws_invalid_option_value)
{
    BOOST_REQUIRE_THROW(from_host("[42:42::42:42]:42"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(utilities__from_host__v6_cidr__throws_invalid_option_value)
{
    BOOST_REQUIRE_THROW(from_host("[42:42::42:42]/24"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(utilities__from_host__unbracketed_default_v6__throws_invalid_option_value)
{
    BOOST_REQUIRE_THROW(from_host("::"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(utilities__from_host__unbracketed_v6__throws_invalid_option_value)
{
    BOOST_REQUIRE_THROW(from_host("4242::4242"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(utilities__from_host__mapped_v6__throws_invalid_option_value)
{
    BOOST_REQUIRE_THROW(from_host("[::ffff:127.0.0.1]"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(utilities__from_host__unspecified_v4__expected_v4)
{
    BOOST_REQUIRE_EQUAL(from_host("0.0.0.0"), asio::ipv4{});
}

BOOST_AUTO_TEST_CASE(utilities__from_host__unspecified_v6__expected_v6)
{
    const asio::ipv6 expected{};
    BOOST_REQUIRE_EQUAL(from_host("[::]"), asio::ipv6{});
}

BOOST_AUTO_TEST_CASE(utilities__from_host__v4__expected_v4)
{
    const asio::ipv4 expected{ asio::ipv4::bytes_type{ 42, 42, 42, 42 } };
    BOOST_REQUIRE_EQUAL(from_host("42.42.42.42"), expected);
}

BOOST_AUTO_TEST_CASE(utilities__from_host__v6__expected_v6)
{
    const asio::ipv6 expected
    {
        asio::ipv6::bytes_type
        {
            0x42, 0x42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x42, 0x42
        }
    };

    BOOST_REQUIRE_EQUAL(from_host("[4242::4242]"), expected);
}

////BOOST_AUTO_TEST_CASE(utilities__from_host__compatible__expected_v4)
////{
////    const asio::address expected{ asio::ipv4{ asio::ipv4::bytes_type{ 127, 0, 1, 1 } } };
////    BOOST_REQUIRE_EQUAL(make_address_v6("[::127.0.0.1]:8333"), expected);
////}
////
////BOOST_AUTO_TEST_CASE(utilities__from_host__mapped__expected_v4)
////{
////    const asio::address expected{ asio::ipv4{ asio::ipv4::bytes_type{ 127, 0, 1, 1 } } };
////    BOOST_REQUIRE_EQUAL(make_address_v6("[::ffff.127.0.0.1]:8333"), expected);
////}

// to_literal

BOOST_AUTO_TEST_CASE(utilities__to_literal__default__unspecified_v4)
{
    // Default asio address is ipv4.
    const auto host = to_literal(asio::address{});
    BOOST_REQUIRE_EQUAL(host, "0.0.0.0");
}

BOOST_AUTO_TEST_CASE(utilities__to_literal__default_v4__unspecified_v4)
{
    const auto host = to_literal(asio::ipv4{});
    BOOST_REQUIRE_EQUAL(host, "0.0.0.0");
}

BOOST_AUTO_TEST_CASE(utilities__to_literal__default_v6__unspecified_literal_v6)
{
    const auto host = to_literal(asio::ipv6{});
    BOOST_REQUIRE_EQUAL(host, "[::]");
}

BOOST_AUTO_TEST_CASE(utilities__to_literal__v4__expected_v4)
{
    const auto host = to_literal(make_address("42.42.42.42"));
    BOOST_REQUIRE_EQUAL(host, "42.42.42.42");
}

BOOST_AUTO_TEST_CASE(utilities__to_literal__v6__expected_literal_v6)
{
    const auto host = to_literal(make_address("42:42::42:42"));
    BOOST_REQUIRE_EQUAL(host, "[42:42::42:42]");
}

// to_address

BOOST_AUTO_TEST_CASE(utilities__to_address__default__non_default)
{
    // default address is v4 and ip_address is v6, to_address is not denormalizing.
    BOOST_REQUIRE_NE(to_address(asio::address{}), messages::ip_address{});
}

BOOST_AUTO_TEST_CASE(utilities__to_address__default_v4__not_default)
{
    BOOST_REQUIRE_NE(to_address(asio::ipv4{}), messages::ip_address{});
}

BOOST_AUTO_TEST_CASE(utilities__to_address__default_v6__default)
{
    BOOST_REQUIRE_EQUAL(to_address(asio::ipv6{}), messages::ip_address{});
}

// from_address

BOOST_AUTO_TEST_CASE(utilities__from_address__default__default_v6)
{
    BOOST_REQUIRE_EQUAL(from_address(messages::ip_address{}), asio::ipv6{});
}

BOOST_AUTO_TEST_CASE(utilities__from_address__v6_mapped_loopback__loopback_v4)
{
    constexpr messages::ip_address loopback_v4
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 127, 0, 0, 1
    };

    const auto ip = from_address(loopback_v4);
    BOOST_REQUIRE(ip.is_v6());

    // Does not recognize mapped context.
    BOOST_REQUIRE(!ip.to_v6().is_loopback());

    // Cannot go straight to ip.to_v4(), must extract the ipv6 object first.
    // address.to_v4() is not the same as ipv6.to_v4(), the former is a getter
    // while the latter is a convertor.
    BOOST_REQUIRE(ip.to_v6().is_v4_mapped());
    BOOST_REQUIRE(ip.to_v6().to_v4().is_loopback());
}

BOOST_AUTO_TEST_CASE(utilities__from_address__loopback_v6__loopback_v6)
{
    constexpr messages::ip_address loopback_v6
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
    };

    const auto ip = from_address(loopback_v6);
    BOOST_REQUIRE(ip.is_v6());
    BOOST_REQUIRE(ip.is_loopback());
    BOOST_REQUIRE(!ip.to_v6().is_v4_mapped());
}

// parse_authority

BOOST_AUTO_TEST_CASE(utilities__parse_authority__empty__false)
{
    asio::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BOOST_REQUIRE(!parse_authority(ip, port, cidr, ""));
}

BOOST_AUTO_TEST_CASE(utilities__parse_authority__v4__expected)
{
    asio::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BOOST_REQUIRE(parse_authority(ip, port, cidr, "127.0.0.1"));
    BOOST_REQUIRE(ip.is_v4());
    BOOST_REQUIRE(ip.is_loopback());
    BOOST_REQUIRE_EQUAL(port, 0u);
    BOOST_REQUIRE_EQUAL(cidr, 0u);
}

BOOST_AUTO_TEST_CASE(utilities__parse_authority__v4_port__expected)
{
    asio::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BOOST_REQUIRE(parse_authority(ip, port, cidr, "127.0.0.1:65535"));
    BOOST_REQUIRE(ip.is_v4());
    BOOST_REQUIRE(ip.is_loopback());
    BOOST_REQUIRE_EQUAL(port, 65535u);
    BOOST_REQUIRE_EQUAL(cidr, 0u);
}

BOOST_AUTO_TEST_CASE(utilities__parse_authority__v4_cidr__expected)
{
    asio::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BOOST_REQUIRE(parse_authority(ip, port, cidr, "127.0.0.1/24"));
    BOOST_REQUIRE(ip.is_v4());
    BOOST_REQUIRE(ip.is_loopback());
    BOOST_REQUIRE_EQUAL(port, 0u);
    BOOST_REQUIRE_EQUAL(cidr, 24u);
}

BOOST_AUTO_TEST_CASE(utilities__parse_authority__v4_port_cidr__expected)
{
    asio::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BOOST_REQUIRE(parse_authority(ip, port, cidr, "127.0.0.1:42/24"));
    BOOST_REQUIRE(ip.is_v4());
    BOOST_REQUIRE(ip.is_loopback());
    BOOST_REQUIRE_EQUAL(port, 42u);
    BOOST_REQUIRE_EQUAL(cidr, 24u);
}

BOOST_AUTO_TEST_CASE(utilities__parse_authority__v4_invalids__false)
{
    asio::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BOOST_REQUIRE(!parse_authority(ip, port, cidr, "[127.0.0.1]"));
    BOOST_REQUIRE(!parse_authority(ip, port, cidr, "127.0.0.1:65536"));
    BOOST_REQUIRE(!parse_authority(ip, port, cidr, "127.0.0.1/0"));
    BOOST_REQUIRE(!parse_authority(ip, port, cidr, "127.0.0.1/33"));
}

BOOST_AUTO_TEST_CASE(utilities__parse_authority__v6__expected)
{
    asio::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BOOST_REQUIRE(parse_authority(ip, port, cidr, "[::1]"));
    BOOST_REQUIRE(ip.is_v6());
    BOOST_REQUIRE(ip.is_loopback());
    BOOST_REQUIRE_EQUAL(port, 0u);
    BOOST_REQUIRE_EQUAL(cidr, 0u);
}

BOOST_AUTO_TEST_CASE(utilities__parse_authority__v6_port__expected)
{
    asio::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BOOST_REQUIRE(parse_authority(ip, port, cidr, "[::1]:65535"));
    BOOST_REQUIRE(ip.is_v6());
    BOOST_REQUIRE(ip.is_loopback());
    BOOST_REQUIRE_EQUAL(port, 65535u);
    BOOST_REQUIRE_EQUAL(cidr, 0u);
}

BOOST_AUTO_TEST_CASE(utilities__parse_authority__v6_cidr__expected)
{
    asio::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BOOST_REQUIRE(parse_authority(ip, port, cidr, "[::1]/64"));
    BOOST_REQUIRE(ip.is_v6());
    BOOST_REQUIRE(ip.is_loopback());
    BOOST_REQUIRE_EQUAL(port, 0u);
    BOOST_REQUIRE_EQUAL(cidr, 64u);
}

BOOST_AUTO_TEST_CASE(utilities__parse_authority__v6_port_cidr__expected)
{
    asio::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BOOST_REQUIRE(parse_authority(ip, port, cidr, "[::1]:42/64"));
    BOOST_REQUIRE(ip.is_v6());
    BOOST_REQUIRE(ip.is_loopback());
    BOOST_REQUIRE_EQUAL(port, 42u);
    BOOST_REQUIRE_EQUAL(cidr, 64u);
}

BOOST_AUTO_TEST_CASE(utilities__parse_authority__v6_invalids__false)
{
    asio::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BOOST_REQUIRE(!parse_authority(ip, port, cidr, "::"));
    BOOST_REQUIRE(!parse_authority(ip, port, cidr, "::1"));
    BOOST_REQUIRE(!parse_authority(ip, port, cidr, "4242::4242"));
    BOOST_REQUIRE(!parse_authority(ip, port, cidr, "[::1]:65536"));
    BOOST_REQUIRE(!parse_authority(ip, port, cidr, "[::1]/0"));
    BOOST_REQUIRE(!parse_authority(ip, port, cidr, "[::1]/129"));
}

// parse_endpoint

BOOST_AUTO_TEST_CASE(utilities__parse_endpoint__full__true_expected)
{
    std::string scheme{};
    std::string host{};
    uint16_t port{};
    BOOST_REQUIRE(parse_endpoint(scheme, host, port, "tcp://foo.bar:42"));
    BOOST_REQUIRE_EQUAL(scheme, "tcp");
    BOOST_REQUIRE_EQUAL(host, "foo.bar");
    BOOST_REQUIRE_EQUAL(port, 42u);
}

BOOST_AUTO_TEST_CASE(utilities__parse_endpoint__host_only__true_expected)
{
    std::string scheme{};
    std::string host{};
    uint16_t port{};
    BOOST_REQUIRE(parse_endpoint(scheme, host, port, "foo.bar"));
    BOOST_REQUIRE(scheme.empty());
    BOOST_REQUIRE_EQUAL(host, "foo.bar");
    BOOST_REQUIRE_EQUAL(port, 0u);
}

BOOST_AUTO_TEST_CASE(utilities__parse_endpoint__host_port__true_expected)
{
    std::string scheme{};
    std::string host{};
    uint16_t port{};
    BOOST_REQUIRE(parse_endpoint(scheme, host, port, "foo.bar:65535"));
    BOOST_REQUIRE(scheme.empty());
    BOOST_REQUIRE_EQUAL(host, "foo.bar");
    BOOST_REQUIRE_EQUAL(port, 65535u);
}

BOOST_AUTO_TEST_CASE(utilities__parse_endpoint__invalids__false_expected)
{
    std::string scheme{};
    std::string host{};
    uint16_t port{};
    BOOST_REQUIRE(!parse_endpoint(scheme, host, port, "tcp://foo.bar:65536"));
    BOOST_REQUIRE(!parse_endpoint(scheme, host, port, "foobar://foo.bar:42"));
    BOOST_REQUIRE(!parse_endpoint(scheme, host, port, "tcp://:42"));
    BOOST_REQUIRE(!parse_endpoint(scheme, host, port, ":42"));
    BOOST_REQUIRE(!parse_endpoint(scheme, host, port, ""));
}

BOOST_AUTO_TEST_SUITE_END()
