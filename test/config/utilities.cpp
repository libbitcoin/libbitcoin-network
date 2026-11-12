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
using namespace system::config;
using namespace system::ip;

BC_AUTO_TEST_SUITE(utilities_tests)

static system::ip::address make_address(const std::string_view& text)
{
    system::ip::address out{};
    BC_REQUIRE(system::ip::address::from_string(out, text));
    return out;
}

// is_member

BC_AUTO_TEST_CASE(utilities__is_member__defaults_zero__false)
{
    BC_REQUIRE(!is_member(system::ip::address{}, system::ip::address{}, 0));
}

BC_AUTO_TEST_CASE(utilities__is_member__defaults_nonzero__false)
{
    BC_REQUIRE(!is_member(system::ip::address{}, system::ip::address{}, 1));
}

BC_AUTO_TEST_CASE(utilities__is_member__ipv4_defaults_nonzero__false)
{
    BC_REQUIRE(!is_member(system::ip::ipv4{}, system::ip::ipv4{}, 24));
}

BC_AUTO_TEST_CASE(utilities__is_member__ipv6_defaults_nonzero__true)
{
    BC_REQUIRE(is_member(system::ip::ipv6{}, system::ip::ipv6{}, 56));
}

BC_AUTO_TEST_CASE(utilities__is_member__zero_cidr__expected)
{
    // zero CIDR valid for ipv4, invalid for ipv6
    BC_REQUIRE(is_member(make_address("42.42.42.42"), make_address("99.99.99.99"), 0));
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("9999:9999:9999:9999:9999:9999:9999:9999"), 0));
}

BC_AUTO_TEST_CASE(utilities__is_member__ipv4_member__true)
{
    BC_REQUIRE(is_member(make_address("42.42.42.42"), make_address("42.99.99.99"), 8));
    BC_REQUIRE(is_member(make_address("42.42.42.42"), make_address("42.42.99.99"), 16));
    BC_REQUIRE(is_member(make_address("42.42.42.42"), make_address("42.42.42.99"), 24));
    BC_REQUIRE(is_member(make_address("42.42.42.42"), make_address("42.42.42.42"), 32));
}

BC_AUTO_TEST_CASE(utilities__is_member__not_ipv4_member__false)
{
    BC_REQUIRE(!is_member(make_address("42.42.42.42"), make_address("99.99.99.99"), 8));
    BC_REQUIRE(!is_member(make_address("42.42.42.42"), make_address("42.99.99.99"), 16));
    BC_REQUIRE(!is_member(make_address("42.42.42.42"), make_address("42.42.99.99"), 24));
    BC_REQUIRE(!is_member(make_address("42.42.42.42"), make_address("42.42.42.99"), 32));
}

BC_AUTO_TEST_CASE(utilities__is_member__ipv6_non_member__true)
{
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("ab99:9999:9999:9999:9999:9999:9999:9999"), 8));
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:9999:9999:9999:9999:9999:9999:9999"), 16));
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:ab99:9999:9999:9999:9999:9999:9999"), 24));
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:9999:9999:9999:9999:9999:9999"), 32));
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:ab99:9999:9999:9999:9999:9999"), 40));
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:9999:9999:9999:9999:9999"), 48));
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:ab99:9999:9999:9999:9999"), 56));
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:9999:9999:9999:9999"), 64));
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:ab99:9999:9999:9999"), 72));
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:abcd:9999:9999:9999"), 80));
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:abcd:ab99:9999:9999"), 88));
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:abcd:abcd:9999:9999"), 96));
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:abcd:abcd:ab99:9999"), 104));
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:9999"), 112));
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:ab99"), 120));
    BC_REQUIRE(is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), 128));
}

BC_AUTO_TEST_CASE(utilities__is_member__ipv6_non_member__false)
{
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("ab99:9999:9999:9999:9999:9999:9999:9999"), 16));
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:9999:9999:9999:9999:9999:9999:9999"), 24));
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:ab99:9999:9999:9999:9999:9999:9999"), 32));
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:9999:9999:9999:9999:9999:9999"), 40));
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:ab99:9999:9999:9999:9999:9999"), 48));
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:9999:9999:9999:9999:9999"), 56));
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:ab99:9999:9999:9999:9999"), 64));
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:9999:9999:9999:9999"), 72));
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:ab99:9999:9999:9999"), 80));
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:abcd:9999:9999:9999"), 88));
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:abcd:ab99:9999:9999"), 96));
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:abcd:abcd:9999:9999"), 104));
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:abcd:abcd:ab99:9999"), 112));
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:9999"), 120));
    BC_REQUIRE(!is_member(make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd"), make_address("abcd:abcd:abcd:abcd:abcd:abcd:abcd:ab99"), 128));
}

// denormalize

BC_AUTO_TEST_CASE(utilities__denormalize__defaults__unchanged)
{
    BC_REQUIRE(denormalize({ system::ip::ipv4{} }).is_v4());
    BC_REQUIRE(denormalize({ system::ip::ipv6{} }).is_v6());
}

BC_AUTO_TEST_CASE(utilities__denormalize__mapped__unmapped)
{
    constexpr system::ip::ipv6::bytes_type mapped
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 1, 2, 3, 4
    };

    const system::ip::address ip{ system::ip::ipv6{ mapped } };
    BC_REQUIRE(denormalize(ip).is_v4());
}

BC_AUTO_TEST_CASE(utilities__denormalize__unmapped__unchanged)
{
    constexpr system::ip::ipv6::bytes_type unmapped
    {
        0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xab, 0xcd, 1, 2, 3, 4
    };

    const system::ip::address ip{ system::ip::ipv6{ unmapped } };
    BC_REQUIRE(denormalize(ip) == ip);
}

// to_host

BC_AUTO_TEST_CASE(utilities__to_host__default__unspecified_v4)
{
    // Default address is ipv4.
    const auto host = to_host(system::ip::address{});
    BC_REQUIRE_EQUAL(host, "0.0.0.0");
}

BC_AUTO_TEST_CASE(utilities__to_host__default_v4__unspecified_v4)
{
    const auto host = to_host(system::ip::ipv4{});
    BC_REQUIRE_EQUAL(host, "0.0.0.0");
}

BC_AUTO_TEST_CASE(utilities__to_host__default_v6__unspecified_v6)
{
    const auto host = to_host(system::ip::ipv6{});
    BC_REQUIRE_EQUAL(host, "::");
}

BC_AUTO_TEST_CASE(utilities__to_host__v4__expected_v4)
{
    const auto host = to_host(make_address("42.42.42.42"));
    BC_REQUIRE_EQUAL(host, "42.42.42.42");
}

BC_AUTO_TEST_CASE(utilities__to_host__v6__expected_v6)
{
    const auto host = to_host(make_address("42:42::42:42"));
    BC_REQUIRE_EQUAL(host, "42:42::42:42");
}

// from_host

BC_AUTO_TEST_CASE(utilities__from_host__empty__throws_invalid_option_value)
{
    BC_REQUIRE_THROW(from_host({}), istream_exception);
}

BC_AUTO_TEST_CASE(utilities__from_host__v4_port__throws_invalid_option_value)
{
    BC_REQUIRE_THROW(from_host("127.0.0.1:42"), istream_exception);
}

BC_AUTO_TEST_CASE(utilities__from_host__v4_cidr__throws_invalid_option_value)
{
    BC_REQUIRE_THROW(from_host("127.0.0.1/24"), istream_exception);
}

BC_AUTO_TEST_CASE(utilities__from_host__v6_port__throws_invalid_option_value)
{
    BC_REQUIRE_THROW(from_host("[42:42::42:42]:42"), istream_exception);
}

BC_AUTO_TEST_CASE(utilities__from_host__v6_cidr__throws_invalid_option_value)
{
    BC_REQUIRE_THROW(from_host("[42:42::42:42]/24"), istream_exception);
}

BC_AUTO_TEST_CASE(utilities__from_host__unbracketed_default_v6__throws_invalid_option_value)
{
    BC_REQUIRE_THROW(from_host("::"), istream_exception);
}

BC_AUTO_TEST_CASE(utilities__from_host__unbracketed_v6__throws_invalid_option_value)
{
    BC_REQUIRE_THROW(from_host("4242::4242"), istream_exception);
}

BC_AUTO_TEST_CASE(utilities__from_host__mapped_v6__throws_invalid_option_value)
{
    BC_REQUIRE_THROW(from_host("[::ffff:127.0.0.1]"), istream_exception);
}

BC_AUTO_TEST_CASE(utilities__from_host__unspecified_v4__expected_v4)
{
    BC_REQUIRE_EQUAL(from_host("0.0.0.0"), system::ip::ipv4{});
}

BC_AUTO_TEST_CASE(utilities__from_host__unspecified_v6__expected_v6)
{
    const system::ip::ipv6 expected{};
    BC_REQUIRE_EQUAL(from_host("[::]"), system::ip::ipv6{});
}

BC_AUTO_TEST_CASE(utilities__from_host__v4__expected_v4)
{
    const system::ip::ipv4 expected{ system::ip::ipv4::bytes_type{ 42, 42, 42, 42 } };
    BC_REQUIRE_EQUAL(from_host("42.42.42.42"), expected);
}

BC_AUTO_TEST_CASE(utilities__from_host__v6__expected_v6)
{
    const system::ip::ipv6 expected
    {
        system::ip::ipv6::bytes_type
        {
            0x42, 0x42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x42, 0x42
        }
    };

    BC_REQUIRE_EQUAL(from_host("[4242::4242]"), expected);
}

////BC_AUTO_TEST_CASE(utilities__from_host__compatible__expected_v4)
////{
////    const system::ip::address expected{ system::ip::ipv4{ system::ip::ipv4::bytes_type{ 127, 0, 1, 1 } } };
////    BC_REQUIRE_EQUAL(make_address("[::127.0.0.1]:8333"), expected);
////}
////
////BC_AUTO_TEST_CASE(utilities__from_host__mapped__expected_v4)
////{
////    const system::ip::address expected{ system::ip::ipv4{ system::ip::ipv4::bytes_type{ 127, 0, 1, 1 } } };
////    BC_REQUIRE_EQUAL(make_address("[::ffff.127.0.0.1]:8333"), expected);
////}

// to_literal

BC_AUTO_TEST_CASE(utilities__to_literal__default__unspecified_v4)
{
    // Default address is ipv4.
    const auto host = to_literal(system::ip::address{});
    BC_REQUIRE_EQUAL(host, "0.0.0.0");
}

BC_AUTO_TEST_CASE(utilities__to_literal__default_v4__unspecified_v4)
{
    const auto host = to_literal(system::ip::ipv4{});
    BC_REQUIRE_EQUAL(host, "0.0.0.0");
}

BC_AUTO_TEST_CASE(utilities__to_literal__default_v6__unspecified_literal_v6)
{
    const auto host = to_literal(system::ip::ipv6{});
    BC_REQUIRE_EQUAL(host, "[::]");
}

BC_AUTO_TEST_CASE(utilities__to_literal__v4__expected_v4)
{
    const auto host = to_literal(make_address("42.42.42.42"));
    BC_REQUIRE_EQUAL(host, "42.42.42.42");
}

BC_AUTO_TEST_CASE(utilities__to_literal__v6__expected_literal_v6)
{
    const auto host = to_literal(make_address("42:42::42:42"));
    BC_REQUIRE_EQUAL(host, "[42:42::42:42]");
}

// to_address

BC_AUTO_TEST_CASE(utilities__to_address__default__non_default)
{
    // default address is v4 and ip_address is v6, to_address is not denormalizing.
    BC_REQUIRE_NE(to_address(system::ip::address{}), messages::peer::ip_address{});
}

BC_AUTO_TEST_CASE(utilities__to_address__default_v4__not_default)
{
    BC_REQUIRE_NE(to_address(system::ip::ipv4{}), messages::peer::ip_address{});
}

BC_AUTO_TEST_CASE(utilities__to_address__default_v6__default)
{
    BC_REQUIRE_EQUAL(to_address(system::ip::ipv6{}), messages::peer::ip_address{});
}

// from_address

BC_AUTO_TEST_CASE(utilities__from_address__default__default_v6)
{
    BC_REQUIRE_EQUAL(from_address(messages::peer::ip_address{}), system::ip::ipv6{});
}

BC_AUTO_TEST_CASE(utilities__from_address__v6_mapped_loopback__loopback_v4)
{
    constexpr messages::peer::ip_address loopback_v4
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 127, 0, 0, 1
    };

    const auto ip = from_address(loopback_v4);
    BC_REQUIRE(ip.is_v6());

    BC_REQUIRE(!ip.to_v6().is_loopback());
    BC_REQUIRE(ip.to_v6().is_v4_mapped());
    BC_REQUIRE(ip.to_v6().to_v4().is_loopback());
}

BC_AUTO_TEST_CASE(utilities__from_address__loopback_v6__loopback_v6)
{
    constexpr messages::peer::ip_address loopback_v6
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
    };

    const auto ip = from_address(loopback_v6);
    BC_REQUIRE(ip.is_v6());
    BC_REQUIRE(ip.is_loopback());
    BC_REQUIRE(!ip.to_v6().is_v4_mapped());
}

// parse_authority

BC_AUTO_TEST_CASE(utilities__parse_authority__empty__false)
{
    system::ip::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BC_REQUIRE(!parse_authority(ip, port, cidr, ""));
}

BC_AUTO_TEST_CASE(utilities__parse_authority__v4__expected)
{
    system::ip::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BC_REQUIRE(parse_authority(ip, port, cidr, "127.0.0.1"));
    BC_REQUIRE(ip.is_v4());
    BC_REQUIRE(ip.is_loopback());
    BC_REQUIRE_EQUAL(port, 0u);
    BC_REQUIRE_EQUAL(cidr, 0u);
}

BC_AUTO_TEST_CASE(utilities__parse_authority__v4_port__expected)
{
    system::ip::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BC_REQUIRE(parse_authority(ip, port, cidr, "127.0.0.1:65535"));
    BC_REQUIRE(ip.is_v4());
    BC_REQUIRE(ip.is_loopback());
    BC_REQUIRE_EQUAL(port, 65535u);
    BC_REQUIRE_EQUAL(cidr, 0u);
}

BC_AUTO_TEST_CASE(utilities__parse_authority__v4_cidr__expected)
{
    system::ip::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BC_REQUIRE(parse_authority(ip, port, cidr, "127.0.0.1/24"));
    BC_REQUIRE(ip.is_v4());
    BC_REQUIRE(ip.is_loopback());
    BC_REQUIRE_EQUAL(port, 0u);
    BC_REQUIRE_EQUAL(cidr, 24u);
}

BC_AUTO_TEST_CASE(utilities__parse_authority__v4_port_cidr__expected)
{
    system::ip::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BC_REQUIRE(parse_authority(ip, port, cidr, "127.0.0.1:42/24"));
    BC_REQUIRE(ip.is_v4());
    BC_REQUIRE(ip.is_loopback());
    BC_REQUIRE_EQUAL(port, 42u);
    BC_REQUIRE_EQUAL(cidr, 24u);
}

BC_AUTO_TEST_CASE(utilities__parse_authority__v4_invalids__false)
{
    system::ip::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BC_REQUIRE(!parse_authority(ip, port, cidr, "[127.0.0.1]"));
    BC_REQUIRE(!parse_authority(ip, port, cidr, "127.0.0.1:65536"));
    BC_REQUIRE(!parse_authority(ip, port, cidr, "127.0.0.1/0"));
    BC_REQUIRE(!parse_authority(ip, port, cidr, "127.0.0.1/33"));
}

BC_AUTO_TEST_CASE(utilities__parse_authority__v6__expected)
{
    system::ip::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BC_REQUIRE(parse_authority(ip, port, cidr, "[::1]"));
    BC_REQUIRE(ip.is_v6());
    BC_REQUIRE(ip.is_loopback());
    BC_REQUIRE_EQUAL(port, 0u);
    BC_REQUIRE_EQUAL(cidr, 0u);
}

BC_AUTO_TEST_CASE(utilities__parse_authority__v6_port__expected)
{
    system::ip::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BC_REQUIRE(parse_authority(ip, port, cidr, "[::1]:65535"));
    BC_REQUIRE(ip.is_v6());
    BC_REQUIRE(ip.is_loopback());
    BC_REQUIRE_EQUAL(port, 65535u);
    BC_REQUIRE_EQUAL(cidr, 0u);
}

BC_AUTO_TEST_CASE(utilities__parse_authority__v6_cidr__expected)
{
    system::ip::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BC_REQUIRE(parse_authority(ip, port, cidr, "[::1]/64"));
    BC_REQUIRE(ip.is_v6());
    BC_REQUIRE(ip.is_loopback());
    BC_REQUIRE_EQUAL(port, 0u);
    BC_REQUIRE_EQUAL(cidr, 64u);
}

BC_AUTO_TEST_CASE(utilities__parse_authority__v6_port_cidr__expected)
{
    system::ip::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BC_REQUIRE(parse_authority(ip, port, cidr, "[::1]:42/64"));
    BC_REQUIRE(ip.is_v6());
    BC_REQUIRE(ip.is_loopback());
    BC_REQUIRE_EQUAL(port, 42u);
    BC_REQUIRE_EQUAL(cidr, 64u);
}

BC_AUTO_TEST_CASE(utilities__parse_authority__v6_invalids__false)
{
    system::ip::address ip{};
    uint16_t port{};
    uint8_t cidr{};
    BC_REQUIRE(!parse_authority(ip, port, cidr, "::"));
    BC_REQUIRE(!parse_authority(ip, port, cidr, "::1"));
    BC_REQUIRE(!parse_authority(ip, port, cidr, "4242::4242"));
    BC_REQUIRE(!parse_authority(ip, port, cidr, "[::1]:65536"));
    BC_REQUIRE(!parse_authority(ip, port, cidr, "[::1]/0"));
    BC_REQUIRE(!parse_authority(ip, port, cidr, "[::1]/129"));
}

// parse_endpoint

BC_AUTO_TEST_CASE(utilities__parse_endpoint__full__true_expected)
{
    std::string host{};
    uint16_t port{};
    BC_REQUIRE(parse_endpoint(host, port, "foo.bar:42"));
    BC_REQUIRE_EQUAL(host, "foo.bar");
    BC_REQUIRE_EQUAL(port, 42u);
}

BC_AUTO_TEST_CASE(utilities__parse_endpoint__host_port__true_expected)
{
    std::string host{};
    uint16_t port{};
    BC_REQUIRE(parse_endpoint(host, port, "foo.bar:65535"));
    BC_REQUIRE_EQUAL(host, "foo.bar");
    BC_REQUIRE_EQUAL(port, 65535u);
}

BC_AUTO_TEST_CASE(utilities__parse_endpoint__invalids__false_expected)
{
    std::string host{};
    uint16_t port{};
    BC_REQUIRE(!parse_endpoint(host, port, "tcp://foo.bar:65536"));
    BC_REQUIRE(!parse_endpoint(host, port, "foobar://foo.bar:42"));
    BC_REQUIRE(!parse_endpoint(host, port, "tcp://:42"));
    BC_REQUIRE(!parse_endpoint(host, port, ":42"));
    BC_REQUIRE(!parse_endpoint(host, port, ""));
}

// to_normal_host

BC_AUTO_TEST_CASE(utilities__to_normal_host__empty_default_zero__empty)
{
    BC_REQUIRE(to_normal_host("", 0).empty());
}

BC_AUTO_TEST_CASE(utilities__to_normal_host__empty_default_nonzero__empty)
{
    BC_REQUIRE(to_normal_host("", 42).empty());
}

BC_AUTO_TEST_CASE(utilities__to_normal_host__nonempty_default_nonzero__nonempty_nonzero)
{
    BC_REQUIRE_EQUAL(to_normal_host("localhost", 42), "localhost:42");
}

BC_AUTO_TEST_CASE(utilities__to_normal_host__numeric_nonzero__numeric_nonzero)
{
    BC_REQUIRE_EQUAL(to_normal_host("127.0.0.1", 42), "127.0.0.1:42");
}

BC_AUTO_TEST_CASE(utilities__to_normal_host__numeric_nonzero_nonzero__numeric_origin_nonzero)
{
    BC_REQUIRE_EQUAL(to_normal_host("127.0.0.1:80", 42), "127.0.0.1:80");
}

// to_normal_origin

BC_AUTO_TEST_CASE(utilities__to_normal_origin__empty__empty)
{
    BC_REQUIRE(to_normal_origin("", 42).empty());
}

BC_AUTO_TEST_CASE(utilities__to_normal_origin__no_scheme__default_port)
{
    BC_REQUIRE_EQUAL(to_normal_origin("Example.com", 42), "example.com:42");
}

BC_AUTO_TEST_CASE(utilities__to_normal_origin__http__default_port)
{
    BC_REQUIRE_EQUAL(to_normal_origin("http://Example.com", 42), "example.com:42");
}

BC_AUTO_TEST_CASE(utilities__to_normal_origin__https__default_port)
{
    BC_REQUIRE_EQUAL(to_normal_origin("https://example.com", 42), "example.com:42");
}

BC_AUTO_TEST_CASE(utilities__to_normal_origin__upper_scheme__empty)
{
    BC_REQUIRE(to_normal_origin("HTTPS://example.com", 42).empty());
}

BC_AUTO_TEST_CASE(utilities__to_normal_origin__tcp_scheme__default_port)
{
    BC_REQUIRE_EQUAL(to_normal_origin("tcp://example.com", 42), "example.com:42");
}

BC_AUTO_TEST_CASE(utilities__to_normal_origin__explicit_port__explicit_port)
{
    BC_REQUIRE_EQUAL(to_normal_origin("http://127.0.0.1:8080", 42), "127.0.0.1:8080");
}

BC_AUTO_TEST_CASE(utilities__to_normal_origin__ipv6__expected)
{
    BC_REQUIRE_EQUAL(to_normal_origin("https://[::1]", 42), "[::1]:42");
}

BC_AUTO_TEST_CASE(utilities__to_normal_origin__scheme_only__empty)
{
    BC_REQUIRE(to_normal_origin("http://", 42).empty());
}

// contains_host

BC_AUTO_TEST_CASE(utilities__contains_host__empty_names__false)
{
    BC_REQUIRE(!contains_host({}, "localhost", 42));
}

BC_AUTO_TEST_CASE(utilities__contains_host__empty_host__false)
{
    BC_REQUIRE(!contains_host({ "localhost" }, "", 42));
}

BC_AUTO_TEST_CASE(utilities__contains_host__portless_name_default_port__true)
{
    BC_REQUIRE(contains_host({ "localhost" }, "LocalHost", 42));
}

BC_AUTO_TEST_CASE(utilities__contains_host__portless_name_any_port__true)
{
    BC_REQUIRE(contains_host({ "localhost" }, "localhost:8780", 42));
}

BC_AUTO_TEST_CASE(utilities__contains_host__portless_name_other_host__false)
{
    BC_REQUIRE(!contains_host({ "localhost" }, "example.com:8780", 42));
}

BC_AUTO_TEST_CASE(utilities__contains_host__ported_name_default_port__true)
{
    BC_REQUIRE(contains_host({ "localhost:42" }, "localhost", 42));
}

BC_AUTO_TEST_CASE(utilities__contains_host__ported_name_same_port__true)
{
    BC_REQUIRE(contains_host({ "localhost:8780" }, "localhost:8780", 42));
}

BC_AUTO_TEST_CASE(utilities__contains_host__ported_name_other_port__false)
{
    BC_REQUIRE(!contains_host({ "localhost:8780" }, "localhost:8781", 42));
}

BC_AUTO_TEST_CASE(utilities__contains_host__ported_name_other_default_port__false)
{
    BC_REQUIRE(!contains_host({ "localhost:8780" }, "localhost", 42));
}

// contains_origin

BC_AUTO_TEST_CASE(utilities__contains_origin__empty_origin__false)
{
    BC_REQUIRE(!contains_origin({ "example.com" }, "", 42));
}

BC_AUTO_TEST_CASE(utilities__contains_origin__invalid_origin__false)
{
    BC_REQUIRE(!contains_origin({ "example.com" }, "HTTPS://example.com", 42));
}

BC_AUTO_TEST_CASE(utilities__contains_origin__portless_name_default_port__true)
{
    BC_REQUIRE(contains_origin({ "example.com" }, "https://Example.com", 42));
}

BC_AUTO_TEST_CASE(utilities__contains_origin__portless_name_any_port__true)
{
    BC_REQUIRE(contains_origin({ "example.com" }, "http://example.com:8780", 42));
}

BC_AUTO_TEST_CASE(utilities__contains_origin__ported_name_default_port__true)
{
    BC_REQUIRE(contains_origin({ "example.com:42" }, "https://example.com", 42));
}

BC_AUTO_TEST_CASE(utilities__contains_origin__ported_name_other_port__false)
{
    BC_REQUIRE(!contains_origin({ "example.com:42" }, "https://example.com:8743", 42));
}

BC_AUTO_TEST_CASE(utilities__contains_origin__portless_ipv6_any_port__true)
{
    BC_REQUIRE(contains_origin({ "[::1]" }, "http://[::1]:8780", 42));
}

BC_AUTO_TEST_SUITE_END()
