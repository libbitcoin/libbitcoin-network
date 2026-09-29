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

BOOST_AUTO_TEST_SUITE(p2p_address_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(address__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(address::command, "addr");
    constexpr auto index = messages::peer::registry::index_of<address>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), address::command);
    BOOST_REQUIRE_EQUAL(address::version_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(address::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(address__size__default__expected)
{
    constexpr auto expected = variable_size(zero);
    BOOST_REQUIRE_EQUAL(address{}.size(level::canonical), expected);
}

// en.bitcoin.it/wiki/Protocol_documentation#addr
static const auto payload = system::base16_chunk("01" "e215104d" "0100000000000000" "00000000000000000000ffff0a000001" "208d");
static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BOOST_AUTO_TEST_CASE(address__size__one_item__expected)
{
    const address message{ { { 0x4d1015e2, 1, {}, 8333 } } };
    BOOST_REQUIRE_EQUAL(message.size(level::minimum_protocol), payload.size());
    BOOST_REQUIRE_EQUAL(message.size(level::canonical), payload.size() - sizeof(uint32_t));
}

// deserialize1

BOOST_AUTO_TEST_CASE(address__deserialize1__bitcoin_wiki_sample__expected)
{
    const auto message = address::deserialize(level::minimum_protocol, payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->addresses.size(), one);

    const auto& item = message->addresses.front();
    BOOST_REQUIRE_EQUAL(item.timestamp, 0x4d1015e2u);
    BOOST_REQUIRE_EQUAL(item.services, 1u);
    BOOST_REQUIRE(is_v4(item.address));
    BOOST_REQUIRE_EQUAL(item.port, 8333u);
}

BOOST_AUTO_TEST_CASE(address__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!address::deserialize(level::canonical, payload));
}

BOOST_AUTO_TEST_CASE(address__deserialize1__excess_version__nullptr)
{
    BOOST_REQUIRE(!address::deserialize(excess_version, payload));
}

BOOST_AUTO_TEST_CASE(address__deserialize1__underflow__nullptr)
{
    const auto data = system::base16_chunk("01e215104d");
    BOOST_REQUIRE(!address::deserialize(level::minimum_protocol, data));
}

BOOST_AUTO_TEST_CASE(address__deserialize1__excess_count__nullptr)
{
    const auto data = system::base16_chunk("fde903");
    BOOST_REQUIRE(!address::deserialize(level::minimum_protocol, data));
}

BOOST_AUTO_TEST_CASE(address__deserialize1__empty__empty)
{
    const auto data = system::base16_chunk("00");
    const auto message = address::deserialize(level::minimum_protocol, data);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE(message->addresses.empty());
}

// deserialize2

BOOST_AUTO_TEST_CASE(address__deserialize2__bitcoin_wiki_sample__expected)
{
    system::read::bytes::copy source(payload);
    const auto message = address::deserialize(level::maximum_protocol, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(message.addresses.size(), one);
    BOOST_REQUIRE_EQUAL(message.addresses.front().port, 8333u);
}

// serialize1

BOOST_AUTO_TEST_CASE(address__serialize1__round_trip__expected)
{
    const auto message = address::deserialize(level::minimum_protocol, payload);
    BOOST_REQUIRE(message);

    system::data_chunk data(message->size(level::minimum_protocol));
    BOOST_REQUIRE(message->serialize(level::minimum_protocol, data));
    BOOST_REQUIRE_EQUAL(data, payload);
}

BOOST_AUTO_TEST_CASE(address__serialize1__default__empty_count)
{
    system::data_chunk data(address{}.size(level::minimum_protocol));
    BOOST_REQUIRE(address{}.serialize(level::minimum_protocol, data));
    BOOST_REQUIRE_EQUAL(data, system::base16_chunk("00"));
}

BOOST_AUTO_TEST_SUITE_END()
