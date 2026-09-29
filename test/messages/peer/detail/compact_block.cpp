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

BOOST_AUTO_TEST_SUITE(p2p_compact_block_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(compact_block__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(compact_block::command, "cmpctblock");
    constexpr auto index = messages::peer::registry::index_of<compact_block>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), compact_block::command);
    BOOST_REQUIRE_EQUAL(compact_block::version_minimum, level::bip152);
    BOOST_REQUIRE_EQUAL(compact_block::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(compact_block__size__default__expected)
{
    const auto expected = system::chain::header::serialized_size()
        + sizeof(uint64_t)
        + variable_size(zero)
        + variable_size(zero);

    BOOST_REQUIRE_EQUAL(compact_block{}.size(level::canonical, true), expected);
    BOOST_REQUIRE_EQUAL(compact_block{}.size(level::canonical, false), expected);
}

// bitcoin/src/test/data/blockfilters.json (testnet block 2)
static const auto header2 = system::base16_chunk("0100000006128e87be8b1b4dea47a7247d5528d2702c96826c7a648497e773b800000000e241352e3bec0a95a6217e10c3abb54adfa05abb12c126695595580fb92e222032e7494dffff001d00d23534");
static const auto coinbase2 = system::base16_chunk("01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0e0432e7494d010e062f503253482fffffffff0100f2052a010000002321038a7f6ef1c8ca0c588aa53fa860128077c9e6c11e6830f4d7ee4e763a56b7718fac00000000");
static const auto block2_hash = system::base16_hash("000000006c02c8ea6e4ff69651f7fcde348fb9d557a06e6957b65552002a7820");
static const auto coinbase2_hash = system::base16_hash("20222eb90f5895556926c112bb5aa0df4ab5abc3107e21a6950aec3b2e3541e2");
static const auto short_id1 = system::base16_array("010203040506");
static const auto short_id2 = system::base16_array("0a0b0c0d0e0f");
static const auto tail = system::base16_chunk("0807060504030201" "02" "010203040506" "0a0b0c0d0e0f");
static const auto unfilled = system::build_chunk({ header2, tail, system::base16_chunk("00") });
static const auto prefilled = system::build_chunk({ header2, tail, system::base16_chunk("0100"), coinbase2 });
static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BOOST_AUTO_TEST_CASE(compact_block__size__unfilled__expected)
{
    const compact_block message{ system::to_shared<system::chain::header>(header2), 0x0102030405060708, { short_id1, short_id2 }, {} };
    BOOST_REQUIRE_EQUAL(message.size(level::bip152, true), unfilled.size());
    BOOST_REQUIRE_EQUAL(message.size(level::bip152, false), unfilled.size());
}

// deserialize1

BOOST_AUTO_TEST_CASE(compact_block__deserialize1__prefilled_coinbase__expected)
{
    const auto message = compact_block::deserialize(level::bip152, prefilled, true);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->header_ptr->hash(), block2_hash);
    BOOST_REQUIRE_EQUAL(message->nonce, 0x0102030405060708u);
    BOOST_REQUIRE_EQUAL(message->short_ids.size(), two);
    BOOST_REQUIRE_EQUAL(message->short_ids.front(), short_id1);
    BOOST_REQUIRE_EQUAL(message->short_ids.back(), short_id2);
    BOOST_REQUIRE_EQUAL(message->transactions.size(), one);
    BOOST_REQUIRE_EQUAL(message->transactions.front().index, zero);
    BOOST_REQUIRE_EQUAL(message->transactions.front().transaction_ptr->hash(false), coinbase2_hash);
}

BOOST_AUTO_TEST_CASE(compact_block__deserialize1__unfilled__expected)
{
    const auto message = compact_block::deserialize(level::bip152, unfilled, false);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->header_ptr->hash(), block2_hash);
    BOOST_REQUIRE_EQUAL(message->short_ids.size(), two);
    BOOST_REQUIRE(message->transactions.empty());
}

BOOST_AUTO_TEST_CASE(compact_block__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!compact_block::deserialize(level::bip133, unfilled, true));
}

BOOST_AUTO_TEST_CASE(compact_block__deserialize1__excess_version__nullptr)
{
    BOOST_REQUIRE(!compact_block::deserialize(excess_version, unfilled, true));
}

BOOST_AUTO_TEST_CASE(compact_block__deserialize1__truncated_short_id__nullptr)
{
    const auto data = system::splice(header2, system::base16_chunk("0807060504030201" "02" "010203040506" "0a0b0c"));
    BOOST_REQUIRE(!compact_block::deserialize(level::bip152, data, true));
}

BOOST_AUTO_TEST_CASE(compact_block__deserialize1__truncated_prefilled__nullptr)
{
    const system::data_chunk data{ prefilled.begin(), std::prev(prefilled.end()) };
    BOOST_REQUIRE(!compact_block::deserialize(level::bip152, data, true));
}

// deserialize2

BOOST_AUTO_TEST_CASE(compact_block__deserialize2__unfilled__expected)
{
    system::read::bytes::copy source(unfilled);
    const auto message = compact_block::deserialize(level::maximum_protocol, source, true);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(message.nonce, 0x0102030405060708u);
}

// serialize1

BOOST_AUTO_TEST_CASE(compact_block__serialize1__unfilled__expected)
{
    const compact_block message{ system::to_shared<system::chain::header>(header2), 0x0102030405060708, { short_id1, short_id2 }, {} };
    system::data_chunk data(message.size(level::bip152, true));
    BOOST_REQUIRE(message.serialize(level::bip152, data, true));
    BOOST_REQUIRE_EQUAL(data, unfilled);
}

BOOST_AUTO_TEST_SUITE_END()
