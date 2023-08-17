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

BOOST_AUTO_TEST_SUITE(version_tests)

using namespace system;
using namespace bc::network::messages;

BOOST_AUTO_TEST_CASE(version__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(version::command, "version");
    BOOST_REQUIRE(version::id == identifier::version);
    BOOST_REQUIRE_EQUAL(version::version_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(version::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(version__size__default_minimum_version__expected)
{
    const auto expected = sizeof(uint32_t)
        + sizeof(uint64_t)
        + sizeof(uint64_t)
        + address_item::size(level::canonical, false)
        + address_item::size(level::canonical, false)
        + sizeof(uint64_t)
        + variable_size(zero)
        + sizeof(uint32_t)
        + zero;

    BOOST_REQUIRE_EQUAL(version{}.size(level::minimum_protocol), expected);
}

BOOST_AUTO_TEST_CASE(version__size__default_bip37_version__expected)
{
    const auto expected = sizeof(uint32_t)
        + sizeof(uint64_t)
        + sizeof(uint64_t)
        + address_item::size(level::canonical, false)
        + address_item::size(level::canonical, false)
        + sizeof(uint64_t)
        + variable_size(zero)
        + sizeof(uint32_t)
        + sizeof(uint8_t);

    BOOST_REQUIRE_EQUAL(version{}.size(level::bip37), expected);
    BOOST_REQUIRE_EQUAL(version{}.size(level::maximum_protocol), expected);
}

// wire examples

// "/Satoshi:1.1.1/" (70006) no relay
// anarchistprime: bitcointalk.org/index.php?topic=1001407
// This node is identifiable by a different genesis block.
static const auto no_relay_anarchist_prime_1 = base16_chunk("761101000100000000000000ae1b9c58000000000100000000000000260106009000d69ee9a999156d2e27fed77d01000000000000002a0104f80160144600000000000000022b2aaf9b8ea1eb14614b0f2f5361746f7368693a312e312e312f64450200");
static const auto no_relay_anarchist_prime_2 = base16_chunk("7611010001000000000000005b429c5800000000010000000000000000000000000000000000ffff1813e52e939b010000000000000000000000000000000000ffffd59fd7db200ac7f00f6ee45f1ab30f2f5361746f7368693a312e312e312f66450200");

// "/Cornell-Falcon-Network:0.1.0/" (70014) no relay
static const auto no_relay_falcon_1 = base16_chunk("7e11010001000000000000005f429c5800000000010000000000000000000000000000000000ffff000000000000010000000000000000000000000000000000ffff22c06db5208d6241eabdf2d6753c1e2f436f726e656c6c2d46616c636f6e2d4e6574776f726b3a302e312e302f97e60600");
static const auto no_relay_falcon_2 = base16_chunk("7e1101000100000000000000ae429c5800000000010000000000000000000000000000000000ffff000000000000010000000000000000000000000000000000ffff23a25ec4208d9ed337a66b411a441e2f436f726e656c6c2d46616c636f6e2d4e6574776f726b3a302e312e302f97e60600");

// "/Satoshi:0.13.0/" (70014) no relay
static const auto no_relay_satoshi = base16_chunk("7e1101000900000000000000ec429c5800000000090000000000000000000000000000000000ffff1813e52e208d090000000000000000000000000000000000ffff97ec0b6d208d7c8c30307127a822102f5361746f7368693a302e31332e302f97e60600");

// "/therealbitcoin.org:0.9.99.99/" (99999) no relay
static const auto no_relay_the_real_bitcoin = base16_chunk("9f86010001000000000000002336a15800000000010000000000000000000000000000000000ffff1813e52ebb81010000000000000000000000000000000000ffff6f6f6f6f208db1f33b262e6acb331e2f7468657265616c626974636f696e2e6f72673a302e392e39392e39392fb9e80600");

BOOST_AUTO_TEST_CASE(version__factory__no_relay_anarchist_prime_1__valid)
{
    read::bytes::copy source(no_relay_anarchist_prime_1);
    const auto instance = version::deserialize(version::version_minimum, source);
    BOOST_REQUIRE(source);
}

BOOST_AUTO_TEST_CASE(version__factory__no_relay_anarchist_prime_2__valid)
{
    read::bytes::copy source(no_relay_anarchist_prime_2);
    const auto instance = version::deserialize(version::version_minimum, source);
    BOOST_REQUIRE(source);
}

BOOST_AUTO_TEST_CASE(version__factory__no_relay_falcon_1__valid)
{
    read::bytes::copy source(no_relay_falcon_1);
    const auto instance = version::deserialize(version::version_minimum, source);
    BOOST_REQUIRE(source);
}

BOOST_AUTO_TEST_CASE(version__factory__no_relay_falcon_2__valid)
{
    read::bytes::copy source(no_relay_falcon_2);
    const auto instance = version::deserialize(version::version_minimum, source);
    BOOST_REQUIRE(source);
}

BOOST_AUTO_TEST_CASE(version__factory__no_relay_satoshi__valid)
{
    read::bytes::copy source(no_relay_satoshi);
    const auto instance = version::deserialize(version::version_minimum, source);
    BOOST_REQUIRE(source);
}

BOOST_AUTO_TEST_CASE(version__factory__therealbitcoin__valid)
{
    read::bytes::copy source(no_relay_the_real_bitcoin);
    const auto instance = version::deserialize(version::version_minimum, source);
    BOOST_REQUIRE(source);
}

BOOST_AUTO_TEST_SUITE_END()
