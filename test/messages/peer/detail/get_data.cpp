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

BC_AUTO_TEST_SUITE(p2p_get_data_tests)

using namespace system;
using namespace network::messages::peer;

constexpr auto hash_tx = base16_hash("0000000000000000000000000000000000000000000000000000000000000001");
constexpr auto hash_block = base16_hash("0000000000000000000000000000000000000000000000000000000000000002");
constexpr auto hash_witness_tx = base16_hash("0000000000000000000000000000000000000000000000000000000000000003");
constexpr auto hash_witness_block = base16_hash("0000000000000000000000000000000000000000000000000000000000000004");
constexpr auto hash_filtered = base16_hash("0000000000000000000000000000000000000000000000000000000000000005");
constexpr auto hash_wtxid = base16_hash("0000000000000000000000000000000000000000000000000000000000000006");
constexpr auto hash_compact = base16_hash("0000000000000000000000000000000000000000000000000000000000000007");

static get_data make_mixed_inventory() NOEXCEPT
{
    static get_data value
    {
        inventory_items
        {
            { inventory_item::type_id::transaction,   hash_tx },
            { inventory_item::type_id::block,         hash_block },
            { inventory_item::type_id::witness_tx,    hash_witness_tx },
            { inventory_item::type_id::witness_block, hash_witness_block },
            { inventory_item::type_id::filtered,      hash_filtered },
            { inventory_item::type_id::wtxid,         hash_wtxid },
            { inventory_item::type_id::compact,       hash_compact }
        }
    };

    return value;
}

BC_AUTO_TEST_CASE(get_data__properties__always__expected)
{
    BC_REQUIRE_EQUAL(get_data::command, "getdata");
    constexpr auto index = messages::peer::registry::index_of<get_data>();
    BC_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), get_data::command);
    BC_REQUIRE_EQUAL(get_data::version_minimum, level::minimum_protocol);
    BC_REQUIRE_EQUAL(get_data::version_maximum, level::maximum_protocol);
}

BC_AUTO_TEST_CASE(get_data__size__default__expected)
{
    constexpr auto expected = variable_size(zero);

    // passed to base class.
    BC_REQUIRE_EQUAL(get_data{}.size(level::canonical), expected);
}

BC_AUTO_TEST_CASE(get_data__select__selectors__expected_items)
{
    const auto inv = make_mixed_inventory();

    const auto txs = inv.select(inventory_item::selector::txids);
    BC_REQUIRE_EQUAL(txs.size(), 2u);
    BC_CHECK(txs[0].type == inventory_item::type_id::transaction);
    BC_CHECK(txs[1].type == inventory_item::type_id::witness_tx);

    const auto wtxs = inv.select(inventory_item::selector::wtxids);
    BC_REQUIRE_EQUAL(wtxs.size(), 1u);
    BC_CHECK(wtxs[0].type == inventory_item::type_id::wtxid);

    const auto blocks = inv.select(inventory_item::selector::blocks);
    BC_REQUIRE_EQUAL(blocks.size(), 2u);
    BC_CHECK(blocks[0].type == inventory_item::type_id::block);
    BC_CHECK(blocks[1].type == inventory_item::type_id::witness_block);

    const auto filters = inv.select(inventory_item::selector::filters);
    BC_REQUIRE_EQUAL(filters.size(), 1u);
    BC_CHECK(filters[0].type == inventory_item::type_id::filtered);
}

BC_AUTO_TEST_CASE(get_data__filter__types__expected_subset)
{
    const auto inv = make_mixed_inventory();

    const auto txs = inv.filter(inventory_item::type_id::transaction);
    BC_REQUIRE_EQUAL(txs.size(), 1u);
    BC_CHECK(txs[0].type == inventory_item::type_id::transaction);
    BC_CHECK(txs[0].hash == hash_tx);

    const auto witness_txs = inv.filter(inventory_item::type_id::witness_tx);
    BC_REQUIRE_EQUAL(witness_txs.size(), 1u);
    BC_CHECK(witness_txs[0].type == inventory_item::type_id::witness_tx);
    BC_CHECK(witness_txs[0].hash == hash_witness_tx);

    const auto witness_blocks = inv.filter(inventory_item::type_id::witness_block);
    BC_REQUIRE_EQUAL(witness_blocks.size(), 1u);
    BC_CHECK(witness_blocks[0].hash == hash_witness_block);

    const auto none = inv.filter(inventory_item::type_id::error);
    BC_CHECK(none.empty());
}

BC_AUTO_TEST_CASE(get_data__to_hashes__specific_type__expected_hashes)
{
    const auto inv = make_mixed_inventory();

    const auto tx_hashes = inv.to_hashes(inventory_item::type_id::transaction);
    BC_REQUIRE_EQUAL(tx_hashes.size(), 1u);
    BC_CHECK(tx_hashes[0] == hash_tx);

    const auto block_hashes = inv.to_hashes(inventory_item::type_id::block);
    BC_REQUIRE_EQUAL(block_hashes.size(), 1u);
    BC_CHECK(block_hashes[0] == hash_block);

    const auto error_hashes = inv.to_hashes(inventory_item::type_id::error);
    BC_CHECK(error_hashes.empty());
}

BC_AUTO_TEST_CASE(get_data__count__selector__expected_counts)
{
    const auto inv = make_mixed_inventory();

    BC_CHECK_EQUAL(inv.count(inventory_item::selector::txids), 2u);
    BC_CHECK_EQUAL(inv.count(inventory_item::selector::wtxids), 1u);
    BC_CHECK_EQUAL(inv.count(inventory_item::selector::blocks), 2u);
    BC_CHECK_EQUAL(inv.count(inventory_item::selector::filters), 1u);
}

BC_AUTO_TEST_CASE(get_data__count__type_id__expected_counts)
{
    const auto inv = make_mixed_inventory();

    BC_CHECK_EQUAL(inv.count(inventory_item::type_id::transaction), 1u);
    BC_CHECK_EQUAL(inv.count(inventory_item::type_id::block), 1u);
    BC_CHECK_EQUAL(inv.count(inventory_item::type_id::witness_tx), 1u);
    BC_CHECK_EQUAL(inv.count(inventory_item::type_id::witness_block), 1u);
    BC_CHECK_EQUAL(inv.count(inventory_item::type_id::filtered), 1u);
    BC_CHECK_EQUAL(inv.count(inventory_item::type_id::wtxid), 1u);
    BC_CHECK_EQUAL(inv.count(inventory_item::type_id::compact), 1u);
    BC_CHECK_EQUAL(inv.count(inventory_item::type_id::error), 0u);
}

BC_AUTO_TEST_CASE(get_data__any__type_id__expected_presence)
{
    const auto inv = make_mixed_inventory();

    BC_CHECK(inv.any(inventory_item::type_id::transaction));
    BC_CHECK(inv.any(inventory_item::type_id::block));
    BC_CHECK(inv.any(inventory_item::type_id::witness_tx));
    BC_CHECK(inv.any(inventory_item::type_id::witness_block));
    BC_CHECK(!inv.any(inventory_item::type_id::error));
}

BC_AUTO_TEST_CASE(get_data__any_transaction__mixed_items__expected)
{
    auto inv = make_mixed_inventory();
    BC_CHECK(inv.any_transaction());

    inv.items.clear();
    BC_CHECK(!inv.any_transaction());

    inv.items.emplace_back(inventory_item{ inventory_item::type_id::witness_tx, hash_tx });
    BC_CHECK(inv.any_transaction());
}

BC_AUTO_TEST_CASE(get_data__any_block__mixed_items__expected)
{
    auto inv = make_mixed_inventory();
    BC_CHECK(inv.any_block());

    inv.items.clear();
    BC_CHECK(!inv.any_block());

    inv.items.emplace_back(inventory_item{ inventory_item::type_id::block, hash_block });
    BC_CHECK(inv.any_block());
}

BC_AUTO_TEST_CASE(get_data__any_witness__mixed_items__expected)
{
    auto inv = make_mixed_inventory();
    BC_CHECK(inv.any_witness());

    inv.items.clear();
    BC_CHECK(!inv.any_witness());

    inv.items.emplace_back(inventory_item{ inventory_item::type_id::witness_tx, hash_witness_tx });
    BC_CHECK(inv.any_witness());
}

// std::ranges::distance requires non-const view (due to potential underlying caching).
BC_AUTO_TEST_CASE(get_data__view__specific_type__filtered_view)
{
    const auto inv = make_mixed_inventory();

    auto tx_view = inv.view(inventory_item::type_id::transaction);
    BC_CHECK(is_one(std::ranges::distance(tx_view)));

    auto block_view = inv.view(inventory_item::type_id::block);
    BC_CHECK(is_one(std::ranges::distance(block_view)));

    auto error_view = inv.view(inventory_item::type_id::error);
    BC_CHECK(is_zero(std::ranges::distance(error_view)));
}

static const auto payload = base16_chunk(
    "02"
    "01000000" "0100000000000000000000000000000000000000000000000000000000000000"
    "02000040" "0400000000000000000000000000000000000000000000000000000000000000");
static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BC_AUTO_TEST_CASE(get_data__size__two__expected)
{
    const get_data message{ { { inventory_item::type_id::transaction, hash_tx }, { inventory_item::type_id::witness_block, hash_witness_block } } };
    BC_REQUIRE_EQUAL(message.size(level::minimum_protocol), payload.size());
}

BC_AUTO_TEST_CASE(get_data__deserialize1__two__expected)
{
    const auto message = get_data::deserialize(level::minimum_protocol, payload);
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->items.size(), two);
    BC_REQUIRE(message->items.front().type == inventory_item::type_id::transaction);
    BC_REQUIRE_EQUAL(message->items.front().hash, hash_tx);
    BC_REQUIRE(message->items.back().type == inventory_item::type_id::witness_block);
    BC_REQUIRE_EQUAL(message->items.back().hash, hash_witness_block);
}

BC_AUTO_TEST_CASE(get_data__deserialize1__insufficient_version__nullptr)
{
    BC_REQUIRE(!get_data::deserialize(level::canonical, payload));
}

BC_AUTO_TEST_CASE(get_data__deserialize1__excess_version__nullptr)
{
    BC_REQUIRE(!get_data::deserialize(excess_version, payload));
}

BC_AUTO_TEST_CASE(get_data__deserialize1__underflow__nullptr)
{
    const data_chunk data{ payload.begin(), std::prev(payload.end()) };
    BC_REQUIRE(!get_data::deserialize(level::minimum_protocol, data));
}

BC_AUTO_TEST_CASE(get_data__deserialize1__excess_count__nullptr)
{
    const auto data = base16_chunk("fe51c30000");
    BC_REQUIRE(!get_data::deserialize(level::minimum_protocol, data));
}

BC_AUTO_TEST_CASE(get_data__deserialize2__two__expected)
{
    read::bytes::copy source(payload);
    const auto message = get_data::deserialize(level::maximum_protocol, source);
    BC_REQUIRE(source);
    BC_REQUIRE_EQUAL(message.items.size(), two);
}

BC_AUTO_TEST_CASE(get_data__serialize1__two__expected)
{
    const get_data message{ { { inventory_item::type_id::transaction, hash_tx }, { inventory_item::type_id::witness_block, hash_witness_block } } };
    data_chunk data(message.size(level::minimum_protocol));
    BC_REQUIRE(message.serialize(level::minimum_protocol, data));
    BC_REQUIRE_EQUAL(data, payload);
}

BC_AUTO_TEST_SUITE_END()
