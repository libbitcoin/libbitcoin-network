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

BOOST_AUTO_TEST_SUITE(config_address_tests)

using namespace network::config;
using namespace boost::program_options;

// tools.ietf.org/html/rfc4291#section-2.2
#define BC_AUTHORITY_IPV4_ADDRESS "1.2.240.1"
#define BC_AUTHORITY_IPV6_UNSPECIFIED_ADDRESS "::"
#define BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "2001:db8::2"
////#define BC_AUTHORITY_IPV6_COMPATIBLE_ADDRESS "::0102:f001"
////#define BC_AUTHORITY_IPV4_BOGUS_ADDRESS "0.0.0.57:256"
////#define BC_AUTHORITY_IPV6_BOGUS_IPV4_ADDRESS "[::ffff:0:39]:256"

// tools.ietf.org/html/rfc4291#section-2.5.2
constexpr messages::ip_address test_unspecified_ip_address =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// tools.ietf.org/html/rfc4291#section-2.5.5.2
constexpr messages::ip_address test_mapped_ip_address =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xff, 0xff, 0x01, 0x02, 0xf0, 0x01
};

////// tools.ietf.org/html/rfc4291#section-2.5.5.1
////constexpr messages::ip_address test_compatible_ip_address =
////{
////    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
////    0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xf0, 0x01
////};

constexpr messages::ip_address test_ipv6_address =
{
    0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
};

constexpr messages::address_item test_unspecified_ip_address_item
{
    10,
    20,
    test_unspecified_ip_address,
    30
};

constexpr messages::address_item test_mapped_ip_address_item
{
    11,
    21,
    test_mapped_ip_address,
    31
};

constexpr messages::address_item test_ipv6_address_item
{
    13,
    23,
    test_ipv6_address,
    33
};

// construct 1/2/3

BOOST_AUTO_TEST_CASE(address__construct__default__false)
{
    const address host{};
    BOOST_REQUIRE(!host);
}

BOOST_AUTO_TEST_CASE(address__construct__bogus_ip__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(address host("bogus"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(address__construct__bogus_port__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(address host("[::]:bogus"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(address__construct__invalid_ipv4__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(address host("999.999.999.999"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(address__construct__invalid_ipv6__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(address host("[:::]"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(address__construct__invalid_port__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(address host("[::]:12345678901"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(address__construct__mapped__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(address host("[::1.2.240.1]"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(address__construct__bogus_timestamp__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(address host("42.42.42.42:4242/test/123"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(address__construct__bogus_services__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(address host("42.42.42.42:4242/123/test"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(address__construct__extra_token__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(address host("42.42.42.42:4242/123/456/"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(address__construct__empty_service__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(address host("42.42.42.42:4242/123/"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(address__construct__empty_timestamp_service__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(address host("42.42.42.42:4242//"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(address__construct__no_timestamp_service__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(address host("42.42.42.42:4242/"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(address__construct__timestamp_service__valid)
{
    BOOST_REQUIRE(address("42.42.42.42:4242/123/456"));
}

BOOST_AUTO_TEST_CASE(address__construct__no_service__valid)
{
    BOOST_REQUIRE(address("42.42.42.42:4242/42"));
}

BOOST_AUTO_TEST_CASE(address__construct__no_slash__valid)
{
    BOOST_REQUIRE(address("42.42.42.42:4242"));
}

BOOST_AUTO_TEST_CASE(address__construct__no_port__false)
{
    // zero port produces false.
    BOOST_REQUIRE(!address("42.42.42.42"));
}

// is_v4

BOOST_AUTO_TEST_CASE(address__is_v4__default__true)
{
    const address item{};
    BOOST_REQUIRE(!item.is_v4());
}

BOOST_AUTO_TEST_CASE(address__is_v4__unspecified_v6__true)
{
    const address item{ messages::unspecified_address_item };
    BOOST_REQUIRE(!item.is_v4());
}

BOOST_AUTO_TEST_CASE(address__is_v4__loopback_v6__true)
{
    const address item{ messages::address_item{ 0, 0, messages::loopback_ip_address, 42 } };
    BOOST_REQUIRE(!item.is_v4());
}

BOOST_AUTO_TEST_CASE(address__is_v4__loopback_v4__false)
{
    const address item{ "127.0.0.1:8333/42/24" };
    BOOST_REQUIRE(item.is_v4());
}

// is_v6

BOOST_AUTO_TEST_CASE(address__is_v6__default__true)
{
    const address item{};
    BOOST_REQUIRE(item.is_v6());
}

BOOST_AUTO_TEST_CASE(address__is_v6__unspecified_v6__true)
{
    const address item{ messages::unspecified_address_item };
    BOOST_REQUIRE(item.is_v6());
}

BOOST_AUTO_TEST_CASE(address__is_v6__loopback_v6__true)
{
    const address item{ messages::address_item{ 0, 0, messages::loopback_ip_address, 42 } };
    BOOST_REQUIRE(item.is_v6());
}

BOOST_AUTO_TEST_CASE(address__is_v6__loopback_v4__false)
{
    const address item{ "127.0.0.1:8333/42/24" };
    BOOST_REQUIRE(!item.is_v6());
}

// cast/ip/port

BOOST_AUTO_TEST_CASE(address__address_item__default__unspecified)
{
    const address host{};
    const messages::address_item& item = host;
    BOOST_REQUIRE_EQUAL(item.ip, host.ip());
    BOOST_REQUIRE_EQUAL(item.port, host.port());
    BOOST_REQUIRE(!messages::is_specified(item));
}

BOOST_AUTO_TEST_CASE(address__address_item__default__secified_expected)
{
    const address host{ messages::address_item{ 0, 0, messages::loopback_ip_address, 42 } };
    const messages::address_item& item = host;
    BOOST_REQUIRE_EQUAL(item.ip, host.ip());
    BOOST_REQUIRE_EQUAL(item.port, host.port());
    BOOST_REQUIRE(messages::is_specified(item));
}

// port

BOOST_AUTO_TEST_CASE(address__port__default__zero_false)
{
    const address host{};
    BOOST_REQUIRE(!host);
    BOOST_REQUIRE_EQUAL(host.port(), 0u);
}

BOOST_AUTO_TEST_CASE(address__port__nullptr__zero_false)
{
    const address host{ messages::address_item::cptr{} };
    BOOST_REQUIRE(!host);
    BOOST_REQUIRE_EQUAL(host.port(), 0u);
}

BOOST_AUTO_TEST_CASE(address__port__copy__expected)
{
    const address other(test_ipv6_address_item);
    const address host(other);
    BOOST_REQUIRE_EQUAL(host.port(), test_ipv6_address_item.port);
}

BOOST_AUTO_TEST_CASE(address__port__move__expected)
{
    address other(move_copy(test_ipv6_address_item));
    const address host(std::move(other));
    BOOST_REQUIRE_EQUAL(host.port(), test_ipv6_address_item.port);
}

BOOST_AUTO_TEST_CASE(address__port__ipv4_address__expected)
{
    constexpr uint16_t expected_port = 42;
    std::stringstream stream{};
    stream << BC_AUTHORITY_IPV4_ADDRESS ":" << expected_port;
    const address host(stream.str());
    BOOST_REQUIRE_EQUAL(host.port(), expected_port);
}

BOOST_AUTO_TEST_CASE(address__port__ipv6_address__expected)
{
    constexpr uint16_t expected_port = 42;
    std::stringstream stream{};
    stream << "[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]:" << expected_port;
    const address host(stream.str());
    BOOST_REQUIRE_EQUAL(host.port(), expected_port);
}

BOOST_AUTO_TEST_CASE(address__port__ip_address__expected)
{
    const address host(test_ipv6_address_item);
    BOOST_REQUIRE_EQUAL(host.port(), test_ipv6_address_item.port);
}

BOOST_AUTO_TEST_CASE(address__port__hostname__zero)
{
    const address host("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]");
    BOOST_REQUIRE_EQUAL(host.port(), 0u);
}

// timestamp()

BOOST_AUTO_TEST_CASE(address__timestamp__default__zero)
{
    const address host(BC_AUTHORITY_IPV4_ADDRESS);
    BOOST_REQUIRE_EQUAL(host.timestamp(), 0u);
}

BOOST_AUTO_TEST_CASE(address__timestamp__value__expected)
{
    const address host(test_ipv6_address_item);
    BOOST_REQUIRE_EQUAL(host.timestamp(), test_ipv6_address_item.timestamp);
}

// services()

BOOST_AUTO_TEST_CASE(address__services__default__zero)
{
    const address host("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]");
    BOOST_REQUIRE_EQUAL(host.services(), 0u);
}

BOOST_AUTO_TEST_CASE(address__services__value__expected)
{
    const address host(test_ipv6_address_item);
    BOOST_REQUIRE_EQUAL(host.services(), test_ipv6_address_item.services);
}

// bool

BOOST_AUTO_TEST_CASE(address__bool__default__false)
{
    const address host{};
    BOOST_REQUIRE(!host);
}

BOOST_AUTO_TEST_CASE(address__bool__unspecified__false)
{
    const address host{ test_unspecified_ip_address_item };
    BOOST_REQUIRE(!host);
}

BOOST_AUTO_TEST_CASE(address__bool__specified__true)
{
    const address host{ test_ipv6_address_item };
    BOOST_REQUIRE(host);
}

// to_string

BOOST_AUTO_TEST_CASE(address__to_string__default__unspecified)
{
    const address host;
    BOOST_REQUIRE_EQUAL(host.to_string(), "[" BC_AUTHORITY_IPV6_UNSPECIFIED_ADDRESS "]/0/0");
}

BOOST_AUTO_TEST_CASE(address__to_string__unspecified__unspecified)
{
    const std::string line = "[" BC_AUTHORITY_IPV6_UNSPECIFIED_ADDRESS "]";
    const address host(line + "/0/0");
    BOOST_REQUIRE_EQUAL(host.to_string(), line + "/0/0");
}

BOOST_AUTO_TEST_CASE(address__to_string__ipv4__expected)
{
    const std::string line = BC_AUTHORITY_IPV4_ADDRESS;
    const address host(line + "/0/0");
    BOOST_REQUIRE_EQUAL(host.to_string(), line + "/0/0");
}

BOOST_AUTO_TEST_CASE(address__to_string__ipv4_port__expected)
{
    const std::string line = BC_AUTHORITY_IPV4_ADDRESS ":42";
    const address host(line + "/0/0");
    BOOST_REQUIRE_EQUAL(host.to_string(), line + "/0/0");
}

BOOST_AUTO_TEST_CASE(address__to_string__ipv6__expected)
{
    const std::string line = "[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]";
    const address host(line + "/0/0");
    BOOST_REQUIRE_EQUAL(host.to_string(), line + "/0/0");
}

BOOST_AUTO_TEST_CASE(address__to_string__ipv6_port__expected)
{
    const std::string line = "[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]:42";
    const address host(line + "/0/0");
    BOOST_REQUIRE_EQUAL(host.to_string(), line + "/0/0");
}

// to_host

BOOST_AUTO_TEST_CASE(address__to_host__default__ipv6_unspecified)
{
    const address host{};
    BOOST_REQUIRE_EQUAL(host.to_host(), BC_AUTHORITY_IPV6_UNSPECIFIED_ADDRESS);
}

BOOST_AUTO_TEST_CASE(address__to_host__ipv4_mapped_ip_address__ipv4)
{
    // A mapped ip address serializes as IPv4.
    const address host(test_mapped_ip_address_item);
    BOOST_REQUIRE_EQUAL(host.to_host(), BC_AUTHORITY_IPV4_ADDRESS);
}

BOOST_AUTO_TEST_CASE(address__to_host__ipv6_address__ipv6_compressed)
{
    // An ipv6 address serializes using compression.
    const address host(test_ipv6_address_item);
    BOOST_REQUIRE_EQUAL(host.to_host(), BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS);
}

// to_ip

BOOST_AUTO_TEST_CASE(address__to_ip__default__ipv6_unspecified)
{
    const address host{};
    BOOST_REQUIRE(host.to_ip().is_unspecified());
}

BOOST_AUTO_TEST_CASE(address__to_ip__value__expected)
{
    const address host{ test_ipv6_address_item };
    BOOST_REQUIRE_EQUAL(host.to_ip(), asio::ipv6(test_ipv6_address));
}

// equality

BOOST_AUTO_TEST_CASE(address__equality__default_default__true)
{
    const address host1{};
    const address host2{};
    BOOST_REQUIRE(host1 == host2);
}

BOOST_AUTO_TEST_CASE(address__equality__default_unspecified_port__true)
{
    const address host1{};
    const address host2("[" BC_AUTHORITY_IPV6_UNSPECIFIED_ADDRESS "]" ":42");
    BOOST_REQUIRE(host1 == host2);
}

BOOST_AUTO_TEST_CASE(address__equality__ipv6_ipv6_distinct_ports__false)
{
    const address host1("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]:24");
    const address host2("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]:42");
    BOOST_REQUIRE(!(host1 == host2));
}

BOOST_AUTO_TEST_CASE(address__equality__ipv4_ipv4__true)
{
    const address host1(BC_AUTHORITY_IPV4_ADDRESS);
    const address host2(BC_AUTHORITY_IPV4_ADDRESS);
    BOOST_REQUIRE(host1 == host2);
}

BOOST_AUTO_TEST_CASE(address__equality__ipv4_ipv4_port__true)
{
    const address host1(BC_AUTHORITY_IPV4_ADDRESS);
    const address host2(BC_AUTHORITY_IPV4_ADDRESS ":42");
    BOOST_REQUIRE(host1 == host2);
}

BOOST_AUTO_TEST_CASE(address__equality__ipv4_ipv6__false)
{
    const address host1(BC_AUTHORITY_IPV4_ADDRESS);
    const address host2("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]");
    BOOST_REQUIRE(!(host1 == host2));
}

BOOST_AUTO_TEST_CASE(address__equality__ipv6_ipv6__true)
{
    const address host1("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]");
    const address host2("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]");
    BOOST_REQUIRE(host1 == host2);
}

BOOST_AUTO_TEST_CASE(address__equality__ipv6_ipv6_port__true)
{
    const address host1("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]");
    const address host2("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]:42");
    BOOST_REQUIRE(host1 == host2);
}

constexpr messages::address_item test_ipv6_address_item_distinct_timestamp
{
    42,
    23,
    test_ipv6_address,
    33
};

constexpr messages::address_item test_ipv6_address_item_distinct_service
{
    13,
    42,
    test_ipv6_address,
    33
};

BOOST_AUTO_TEST_CASE(address__equality__distinct_timestamp__true)
{
    const address host1(test_ipv6_address_item);
    const address host2(test_ipv6_address_item_distinct_timestamp);
    BOOST_REQUIRE(host1 == host2);
}

BOOST_AUTO_TEST_CASE(address__equality__distinct_services__true)
{
    const address host1(test_ipv6_address_item);
    const address host2(test_ipv6_address_item_distinct_service);
    BOOST_REQUIRE(host1 == host2);
}

// equality address_item

BOOST_AUTO_TEST_CASE(address__equality__default_address_item__true)
{
    const address host1{};
    const messages::address_item host2{};
    BOOST_REQUIRE(host1 == host2);
}

BOOST_AUTO_TEST_CASE(address__equality__same_address_item__true)
{
    const address host1(test_ipv6_address_item);
    BOOST_REQUIRE(host1 == test_ipv6_address_item);
}

BOOST_AUTO_TEST_CASE(address__equality__distinct_address_item__false)
{
    const address host1(test_ipv6_address_item);
    BOOST_REQUIRE(!(host1 == test_mapped_ip_address_item));
}

BOOST_AUTO_TEST_CASE(address__equality__distinct_timestamp_address_item__true)
{
    const address host1(test_ipv6_address_item);
    BOOST_REQUIRE(host1 == test_ipv6_address_item_distinct_timestamp);
}

BOOST_AUTO_TEST_CASE(address__equality__distinct_services_address_item__true)
{
    const address host1(test_ipv6_address_item);
    BOOST_REQUIRE(host1 == test_ipv6_address_item_distinct_service);
}

// inequality

BOOST_AUTO_TEST_CASE(address__inequality__default_default__false)
{
    const address host1{};
    const address host2{};
    BOOST_REQUIRE(!(host1 != host2));
}

BOOST_AUTO_TEST_CASE(address__inequality__default_unspecified_port__true)
{
    const address host1("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]:42");
    const address host2("[" BC_AUTHORITY_IPV6_UNSPECIFIED_ADDRESS "]:42");
    BOOST_REQUIRE(host1 != host2);
}

BOOST_AUTO_TEST_CASE(address__inequality__ipv6_ipv6_distinct_ports__true)
{
    const address host1("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]:24");
    const address host2("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]:42");
    BOOST_REQUIRE(host1 != host2);
}

BOOST_AUTO_TEST_CASE(address__inequality__distinct_timestamp__false)
{
    const address host1(test_ipv6_address_item);
    const address host2(test_ipv6_address_item_distinct_timestamp);
    BOOST_REQUIRE(!(host1 != host2));
}

BOOST_AUTO_TEST_CASE(address__inequality__distinct_services__false)
{
    const address host1(test_ipv6_address_item);
    const address host2(test_ipv6_address_item_distinct_service);
    BOOST_REQUIRE(!(host1 != host2));
}

// inequality address_item

BOOST_AUTO_TEST_CASE(address__inequality__default_address_item__true)
{
    const address host1{};
    const messages::address_item host2{};
    BOOST_REQUIRE(!(host1 != host2));
}

BOOST_AUTO_TEST_CASE(address__inequality__same_address_item__true)
{
    const address host1(test_ipv6_address_item);
    BOOST_REQUIRE(!(host1 != test_ipv6_address_item));
}

BOOST_AUTO_TEST_CASE(address__inequality__distinct_address_item__true)
{
    const address host1(test_ipv6_address_item);
    BOOST_REQUIRE(host1 != test_mapped_ip_address_item);
}

BOOST_AUTO_TEST_CASE(address__inequality__distinct_timestamp_address_item_false)
{
    const address host1(test_ipv6_address_item);
    BOOST_REQUIRE(!(host1 != test_ipv6_address_item_distinct_timestamp));
}

BOOST_AUTO_TEST_CASE(address__inequality__distinct_services_address_item__false)
{
    const address host1(test_ipv6_address_item);
    BOOST_REQUIRE(!(host1 != test_ipv6_address_item_distinct_service));
}

BOOST_AUTO_TEST_SUITE_END()
