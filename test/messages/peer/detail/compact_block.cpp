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

BC_AUTO_TEST_SUITE(p2p_compact_block_tests)

using namespace network::messages::peer;

BC_AUTO_TEST_CASE(compact_block__properties__always__expected)
{
    BC_REQUIRE_EQUAL(compact_block::command, "cmpctblock");
    constexpr auto index = messages::peer::registry::index_of<compact_block>();
    BC_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), compact_block::command);
    BC_REQUIRE_EQUAL(compact_block::version_minimum, level::bip152);
    BC_REQUIRE_EQUAL(compact_block::version_maximum, level::maximum_protocol);
}

BC_AUTO_TEST_CASE(compact_block__size__default__expected)
{
    const auto expected = system::chain::header::serialized_size()
        + sizeof(uint64_t)
        + variable_size(zero)
        + variable_size(zero);

    BC_REQUIRE_EQUAL(compact_block{}.size(level::canonical, true), expected);
    BC_REQUIRE_EQUAL(compact_block{}.size(level::canonical, false), expected);
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

BC_AUTO_TEST_CASE(compact_block__size__unfilled__expected)
{
    const compact_block message{ system::to_shared<system::chain::header>(header2), 0x0102030405060708, { short_id1, short_id2 }, {} };
    BC_REQUIRE_EQUAL(message.size(level::bip152, true), unfilled.size());
    BC_REQUIRE_EQUAL(message.size(level::bip152, false), unfilled.size());
}

// deserialize1

BC_AUTO_TEST_CASE(compact_block__deserialize1__prefilled_coinbase__expected)
{
    const auto message = compact_block::deserialize(level::bip152, prefilled, true);
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->header_ptr->hash(), block2_hash);
    BC_REQUIRE_EQUAL(message->nonce, 0x0102030405060708u);
    BC_REQUIRE_EQUAL(message->short_ids.size(), two);
    BC_REQUIRE_EQUAL(message->short_ids.front(), short_id1);
    BC_REQUIRE_EQUAL(message->short_ids.back(), short_id2);
    BC_REQUIRE_EQUAL(message->transactions.size(), one);
    BC_REQUIRE_EQUAL(message->transactions.front().index, zero);
    BC_REQUIRE_EQUAL(message->transactions.front().transaction_ptr->hash(false), coinbase2_hash);
}

BC_AUTO_TEST_CASE(compact_block__deserialize1__unfilled__expected)
{
    const auto message = compact_block::deserialize(level::bip152, unfilled, false);
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->header_ptr->hash(), block2_hash);
    BC_REQUIRE_EQUAL(message->short_ids.size(), two);
    BC_REQUIRE(message->transactions.empty());
}

BC_AUTO_TEST_CASE(compact_block__deserialize1__insufficient_version__nullptr)
{
    BC_REQUIRE(!compact_block::deserialize(level::bip133, unfilled, true));
}

BC_AUTO_TEST_CASE(compact_block__deserialize1__excess_version__nullptr)
{
    BC_REQUIRE(!compact_block::deserialize(excess_version, unfilled, true));
}

BC_AUTO_TEST_CASE(compact_block__deserialize1__truncated_short_id__nullptr)
{
    const auto data = system::splice(header2, system::base16_chunk("0807060504030201" "02" "010203040506" "0a0b0c"));
    BC_REQUIRE(!compact_block::deserialize(level::bip152, data, true));
}

BC_AUTO_TEST_CASE(compact_block__deserialize1__prefilled_count_exceeds_data__nullptr)
{
    const auto data = system::base16_chunk("0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000" "0000000000000000" "00" "fe40420f00" "00" "000000000001fe40420f00");
    BC_REQUIRE(!compact_block::deserialize(level::bip152, data, true));
}

BC_AUTO_TEST_CASE(compact_block__deserialize1__truncated_prefilled__nullptr)
{
    const system::data_chunk data{ prefilled.begin(), std::prev(prefilled.end()) };
    BC_REQUIRE(!compact_block::deserialize(level::bip152, data, true));
}

// deserialize2

BC_AUTO_TEST_CASE(compact_block__deserialize2__unfilled__expected)
{
    system::read::bytes::copy source(unfilled);
    const auto message = compact_block::deserialize(level::maximum_protocol, source, true);
    BC_REQUIRE(source);
    BC_REQUIRE_EQUAL(message.nonce, 0x0102030405060708u);
}

// serialize1

BC_AUTO_TEST_CASE(compact_block__serialize1__unfilled__expected)
{
    const compact_block message{ system::to_shared<system::chain::header>(header2), 0x0102030405060708, { short_id1, short_id2 }, {} };
    system::data_chunk data(message.size(level::bip152, true));
    BC_REQUIRE(message.serialize(level::bip152, data, true));
    BC_REQUIRE_EQUAL(data, unfilled);
}

BC_AUTO_TEST_SUITE_END()
