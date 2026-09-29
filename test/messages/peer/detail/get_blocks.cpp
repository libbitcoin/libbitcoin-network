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

BOOST_AUTO_TEST_SUITE(p2p_get_blocks_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(get_blocks__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(get_blocks::command, "getblocks");
    constexpr auto index = messages::peer::registry::index_of<get_blocks>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), get_blocks::command);
    BOOST_REQUIRE_EQUAL(get_blocks::version_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(get_blocks::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(get_blocks__size__default__expected)
{
    constexpr auto expected = sizeof(uint32_t) +
        system::hash_size +
        variable_size(zero);

    BOOST_REQUIRE_EQUAL(get_blocks{}.size(level::canonical), expected);
}

static const auto genesis_hash = system::base16_hash("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");
static const auto payload = system::base16_chunk(
    "80110100"
    "01"
    "6fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000"
    "0000000000000000000000000000000000000000000000000000000000000000");
static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BOOST_AUTO_TEST_CASE(get_blocks__locator_size__zero__one)
{
    BOOST_REQUIRE_EQUAL(get_blocks::locator_size(0), one);
}

BOOST_AUTO_TEST_CASE(get_blocks__locator_size__ten__eleven)
{
    BOOST_REQUIRE_EQUAL(get_blocks::locator_size(10), 11u);
}

BOOST_AUTO_TEST_CASE(get_blocks__heights__zero__genesis)
{
    const get_blocks::indexes expected{ 0 };
    BOOST_REQUIRE_EQUAL(get_blocks::heights(0), expected);
}

BOOST_AUTO_TEST_CASE(get_blocks__heights__ten__contiguous)
{
    const get_blocks::indexes expected{ 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
    BOOST_REQUIRE_EQUAL(get_blocks::heights(10), expected);
}

BOOST_AUTO_TEST_CASE(get_blocks__heights__one_million__locator_size)
{
    const auto heights = get_blocks::heights(1'000'000);
    BOOST_REQUIRE_EQUAL(heights.size(), get_blocks::locator_size(1'000'000));
    BOOST_REQUIRE_EQUAL(heights.front(), 1'000'000u);
    BOOST_REQUIRE_EQUAL(heights.back(), zero);
}

BOOST_AUTO_TEST_CASE(get_blocks__start_hash__empty__null_hash)
{
    BOOST_REQUIRE_EQUAL(get_blocks{}.start_hash(), system::null_hash);
}

BOOST_AUTO_TEST_CASE(get_blocks__start_hash__genesis__genesis)
{
    const get_blocks message{ { genesis_hash }, system::null_hash };
    BOOST_REQUIRE_EQUAL(message.start_hash(), genesis_hash);
}

BOOST_AUTO_TEST_CASE(get_blocks__size__genesis__expected)
{
    const get_blocks message{ { genesis_hash }, system::null_hash };
    BOOST_REQUIRE_EQUAL(message.size(level::bip155), payload.size());
}

BOOST_AUTO_TEST_CASE(get_blocks__deserialize1__genesis__expected)
{
    const auto message = get_blocks::deserialize(level::bip155, payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->start_hashes.size(), one);
    BOOST_REQUIRE_EQUAL(message->start_hashes.front(), genesis_hash);
    BOOST_REQUIRE_EQUAL(message->stop_hash, system::null_hash);
}

BOOST_AUTO_TEST_CASE(get_blocks__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!get_blocks::deserialize(level::canonical, payload));
}

BOOST_AUTO_TEST_CASE(get_blocks__deserialize1__excess_version__nullptr)
{
    BOOST_REQUIRE(!get_blocks::deserialize(excess_version, payload));
}

BOOST_AUTO_TEST_CASE(get_blocks__deserialize1__underflow__nullptr)
{
    const system::data_chunk data{ payload.begin(), std::prev(payload.end()) };
    BOOST_REQUIRE(!get_blocks::deserialize(level::bip155, data));
}

BOOST_AUTO_TEST_CASE(get_blocks__deserialize1__excess_locator__nullptr)
{
    const auto data = system::base16_chunk("80110100" "fd0004");
    BOOST_REQUIRE(!get_blocks::deserialize(level::bip155, data));
}

BOOST_AUTO_TEST_CASE(get_blocks__deserialize2__genesis__expected)
{
    system::read::bytes::copy source(payload);
    const auto message = get_blocks::deserialize(level::minimum_protocol, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(message.start_hash(), genesis_hash);
}

BOOST_AUTO_TEST_CASE(get_blocks__serialize1__genesis__expected)
{
    const get_blocks message{ { genesis_hash }, system::null_hash };
    system::data_chunk data(message.size(level::bip155));
    BOOST_REQUIRE(message.serialize(level::bip155, data));
    BOOST_REQUIRE_EQUAL(data, payload);
}

BOOST_AUTO_TEST_SUITE_END()
