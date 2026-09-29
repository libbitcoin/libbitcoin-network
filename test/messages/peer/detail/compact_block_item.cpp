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

BOOST_AUTO_TEST_SUITE(p2p_compact_block_item_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(compact_block_item__size__default__expected)
{
    constexpr auto expected = variable_size(uint64_t{});
    BOOST_REQUIRE_EQUAL(compact_block_item{}.size(level::canonical, true), expected);
    BOOST_REQUIRE_EQUAL(compact_block_item{}.size(level::canonical, false), expected);
}

// bitcoin/src/test/data/blockfilters.json (testnet block 2 coinbase)
static const auto coinbase2 = system::base16_chunk("01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0e0432e7494d010e062f503253482fffffffff0100f2052a010000002321038a7f6ef1c8ca0c588aa53fa860128077c9e6c11e6830f4d7ee4e763a56b7718fac00000000");
static const auto coinbase2_hash = system::base16_hash("20222eb90f5895556926c112bb5aa0df4ab5abc3107e21a6950aec3b2e3541e2");

BOOST_AUTO_TEST_CASE(compact_block_item__deserialize__coinbase__expected)
{
    const auto data = system::splice(system::base16_chunk("fd0301"), coinbase2);
    system::read::bytes::copy source(data);
    const auto item = compact_block_item::deserialize(level::bip152, source, true);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(item.index, 0x0103u);
    BOOST_REQUIRE(item.transaction_ptr);
    BOOST_REQUIRE_EQUAL(item.transaction_ptr->hash(false), coinbase2_hash);
}

BOOST_AUTO_TEST_CASE(compact_block_item__serialize__coinbase__round_trip)
{
    const auto data = system::splice(system::base16_chunk("fd0301"), coinbase2);
    system::read::bytes::copy source(data);
    const auto item = compact_block_item::deserialize(level::bip152, source, true);
    BOOST_REQUIRE_EQUAL(item.size(level::bip152, true), data.size());

    system::data_chunk out(data.size());
    system::write::bytes::copy sink(out);
    item.serialize(level::bip152, sink, true);
    BOOST_REQUIRE(sink);
    BOOST_REQUIRE_EQUAL(out, data);
}

BOOST_AUTO_TEST_CASE(compact_block_item__deserialize__truncated__source_false)
{
    const system::data_chunk data{ coinbase2.begin(), std::prev(coinbase2.end()) };
    const auto prefixed = system::splice(system::base16_chunk("00"), data);
    system::read::bytes::copy source(prefixed);
    const auto item = compact_block_item::deserialize(level::bip152, source, false);
    BOOST_REQUIRE(!source);
}

BOOST_AUTO_TEST_SUITE_END()
