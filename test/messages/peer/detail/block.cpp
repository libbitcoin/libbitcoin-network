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

BOOST_AUTO_TEST_SUITE(p2p_block_tests)

using namespace network::messages;

BOOST_AUTO_TEST_CASE(block__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(peer::block::command, "block");
    constexpr auto index = messages::peer::registry::index_of<peer::block>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), peer::block::command);
    BOOST_REQUIRE_EQUAL(peer::block::version_minimum, peer::level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(peer::block::version_maximum, peer::level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(block__size__default__zero)
{
    const peer::block default_block{ { {}, {} } };
    BOOST_REQUIRE_EQUAL(default_block.size(peer::level::canonical, true), zero);
    BOOST_REQUIRE_EQUAL(default_block.size(peer::level::canonical, false), zero);
}

// bitcoin/src/test/data/blockfilters.json (testnet block 2)
static const auto block2 = system::base16_chunk(
    "0100000006128e87be8b1b4dea47a7247d5528d2702c96826c7a648497e773b800000000e241352e3bec0a95a6217e10c3abb54adfa05abb12c126695595580fb92e222032e7494dffff001d00d23534"
    "01"
    "01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0e0432e7494d010e062f503253482fffffffff0100f2052a010000002321038a7f6ef1c8ca0c588aa53fa860128077c9e6c11e6830f4d7ee4e763a56b7718fac00000000");
static const auto block2_hash = system::base16_hash("000000006c02c8ea6e4ff69651f7fcde348fb9d557a06e6957b65552002a7820");
static const uint32_t excess_version = add1<uint32_t>(peer::level::maximum_protocol);

// deserialize1

BOOST_AUTO_TEST_CASE(block__deserialize1__testnet_block2__expected)
{
    const auto message = peer::block::deserialize(peer::level::minimum_protocol, block2, true);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->block.hash(), block2_hash);
    BOOST_REQUIRE_EQUAL(message->block.transactions(), one);
    BOOST_REQUIRE_EQUAL(message->size(peer::level::minimum_protocol, true), block2.size());
    BOOST_REQUIRE_EQUAL(message->size(peer::level::minimum_protocol, false), block2.size());
}

BOOST_AUTO_TEST_CASE(block__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!peer::block::deserialize(peer::level::canonical, block2, true));
}

BOOST_AUTO_TEST_CASE(block__deserialize1__excess_version__nullptr)
{
    BOOST_REQUIRE(!peer::block::deserialize(excess_version, block2, true));
}

BOOST_AUTO_TEST_CASE(block__deserialize1__empty__nullptr)
{
    BOOST_REQUIRE(!peer::block::deserialize(peer::level::minimum_protocol, system::data_chunk{}, true));
}

BOOST_AUTO_TEST_CASE(block__deserialize1__truncated__nullptr)
{
    const system::data_chunk data{ block2.begin(), std::prev(block2.end()) };
    BOOST_REQUIRE(!peer::block::deserialize(peer::level::minimum_protocol, data, false));
}

// deserialize2

BOOST_AUTO_TEST_CASE(block__deserialize2__testnet_block2__expected)
{
    system::read::bytes::copy source(block2);
    const auto message = peer::block::deserialize(peer::level::maximum_protocol, source, false);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(message.block.hash(), block2_hash);
}

BOOST_AUTO_TEST_CASE(block__deserialize2__insufficient_version__source_false)
{
    system::read::bytes::copy source(block2);
    const auto message = peer::block::deserialize(peer::level::canonical, source, true);
    BOOST_REQUIRE(!source);
    BOOST_REQUIRE(!message.block.is_valid());
}

BOOST_AUTO_TEST_CASE(block__deserialize2__truncated__source_false)
{
    const system::data_chunk data{ block2.begin(), std::prev(block2.end()) };
    system::read::bytes::copy source(data);
    const auto message = peer::block::deserialize(peer::level::minimum_protocol, source, true);
    BOOST_REQUIRE(!source);
}

// serialize1

BOOST_AUTO_TEST_CASE(block__serialize1__round_trip__expected)
{
    const auto message = peer::block::deserialize(peer::level::minimum_protocol, block2, true);
    BOOST_REQUIRE(message);

    system::data_chunk data(message->size(peer::level::minimum_protocol, true));
    BOOST_REQUIRE(message->serialize(peer::level::minimum_protocol, data, true));
    BOOST_REQUIRE_EQUAL(data, block2);
}

BOOST_AUTO_TEST_SUITE_END()
