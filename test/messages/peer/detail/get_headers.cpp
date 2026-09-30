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

BOOST_AUTO_TEST_SUITE(p2p_get_headers_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(get_headers__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(get_headers::command, "getheaders");
    constexpr auto index = messages::peer::registry::index_of<get_headers>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), get_headers::command);
    BOOST_REQUIRE_EQUAL(get_headers::version_minimum, level::headers_protocol);
    BOOST_REQUIRE_EQUAL(get_headers::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(get_headers__size__default__expected)
{
    constexpr auto expected = sizeof(uint32_t) +
        system::hash_size +
        variable_size(zero);

    // passed to base class.
    BOOST_REQUIRE_EQUAL(get_headers{}.size(level::canonical), expected);
}

static const auto genesis_hash = system::base16_hash("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");
static const auto payload = system::base16_chunk(
    "80110100"
    "01"
    "6fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000"
    "0000000000000000000000000000000000000000000000000000000000000000");
static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BOOST_AUTO_TEST_CASE(get_headers__locator_size__zero__one)
{
    BOOST_REQUIRE_EQUAL(get_headers::locator_size(0), one);
}

BOOST_AUTO_TEST_CASE(get_headers__locator_size__ten__eleven)
{
    BOOST_REQUIRE_EQUAL(get_headers::locator_size(10), 11u);
}

BOOST_AUTO_TEST_CASE(get_headers__heights__zero__genesis)
{
    const get_headers::indexes expected{ 0 };
    BOOST_REQUIRE_EQUAL(get_headers::heights(0), expected);
}

BOOST_AUTO_TEST_CASE(get_headers__heights__ten__contiguous)
{
    const get_headers::indexes expected{ 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
    BOOST_REQUIRE_EQUAL(get_headers::heights(10), expected);
}

BOOST_AUTO_TEST_CASE(get_headers__heights__one_million__locator_size)
{
    const auto heights = get_headers::heights(1'000'000);
    BOOST_REQUIRE_EQUAL(heights.size(), get_headers::locator_size(1'000'000));
    BOOST_REQUIRE_EQUAL(heights.front(), 1'000'000u);
    BOOST_REQUIRE_EQUAL(heights.back(), zero);
}

BOOST_AUTO_TEST_CASE(get_headers__start_hash__empty__null_hash)
{
    BOOST_REQUIRE_EQUAL(get_headers{}.start_hash(), system::null_hash);
}

BOOST_AUTO_TEST_CASE(get_headers__start_hash__genesis__genesis)
{
    const get_headers message{ { genesis_hash }, system::null_hash };
    BOOST_REQUIRE_EQUAL(message.start_hash(), genesis_hash);
}

BOOST_AUTO_TEST_CASE(get_headers__size__genesis__expected)
{
    const get_headers message{ { genesis_hash }, system::null_hash };
    BOOST_REQUIRE_EQUAL(message.size(level::bip155), payload.size());
}

BOOST_AUTO_TEST_CASE(get_headers__deserialize1__genesis__expected)
{
    const auto message = get_headers::deserialize(level::bip155, payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->start_hashes.size(), one);
    BOOST_REQUIRE_EQUAL(message->start_hashes.front(), genesis_hash);
    BOOST_REQUIRE_EQUAL(message->stop_hash, system::null_hash);
}

BOOST_AUTO_TEST_CASE(get_headers__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!get_headers::deserialize(level::canonical, payload));
}

BOOST_AUTO_TEST_CASE(get_headers__deserialize1__excess_version__nullptr)
{
    BOOST_REQUIRE(!get_headers::deserialize(excess_version, payload));
}

BOOST_AUTO_TEST_CASE(get_headers__deserialize1__underflow__nullptr)
{
    const system::data_chunk data{ payload.begin(), std::prev(payload.end()) };
    BOOST_REQUIRE(!get_headers::deserialize(level::bip155, data));
}

BOOST_AUTO_TEST_CASE(get_headers__deserialize1__excess_locator__nullptr)
{
    const auto data = system::base16_chunk("80110100" "fd0004");
    BOOST_REQUIRE(!get_headers::deserialize(level::bip155, data));
}

BOOST_AUTO_TEST_CASE(get_headers__deserialize2__genesis__expected)
{
    system::read::bytes::copy source(payload);
    const auto message = get_headers::deserialize(level::headers_protocol, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(message.start_hash(), genesis_hash);
}

BOOST_AUTO_TEST_CASE(get_headers__serialize1__genesis__expected)
{
    const get_headers message{ { genesis_hash }, system::null_hash };
    system::data_chunk data(message.size(level::bip155));
    BOOST_REQUIRE(message.serialize(level::bip155, data));
    BOOST_REQUIRE_EQUAL(data, payload);
}

BOOST_AUTO_TEST_CASE(get_headers__deserialize1__headers_protocol__expected)
{
    const auto message = get_headers::deserialize(level::headers_protocol, payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->start_hash(), genesis_hash);
}

BOOST_AUTO_TEST_CASE(get_headers__deserialize1__address_timestamp__nullptr)
{
    BOOST_REQUIRE(!get_headers::deserialize(level::address_timestamp, payload));
}

BOOST_AUTO_TEST_SUITE_END()
