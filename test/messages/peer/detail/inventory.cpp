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

BOOST_AUTO_TEST_SUITE(p2p_inventory_tests)

using namespace system;
using namespace network::messages::peer;

constexpr auto hash_tx = base16_hash("0000000000000000000000000000000000000000000000000000000000000001");
constexpr auto hash_block = base16_hash("0000000000000000000000000000000000000000000000000000000000000002");
constexpr auto hash_witness_tx = base16_hash("0000000000000000000000000000000000000000000000000000000000000003");
constexpr auto hash_witness_block = base16_hash("0000000000000000000000000000000000000000000000000000000000000004");
constexpr auto hash_filtered = base16_hash("0000000000000000000000000000000000000000000000000000000000000005");
constexpr auto hash_wtxid = base16_hash("0000000000000000000000000000000000000000000000000000000000000006");
constexpr auto hash_compact = base16_hash("0000000000000000000000000000000000000000000000000000000000000007");

static inventory make_mixed_inventory() NOEXCEPT
{
    static inventory value
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

BOOST_AUTO_TEST_CASE(inventory__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(inventory::command, "inv");
    constexpr auto index = messages::peer::registry::index_of<inventory>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), inventory::command);
    BOOST_REQUIRE_EQUAL(inventory::version_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(inventory::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(inventory__size__default__expected)
{
    constexpr auto expected = variable_size(zero);
    BOOST_REQUIRE_EQUAL(inventory{}.size(level::canonical), expected);
}

BOOST_AUTO_TEST_CASE(inventory__select__selectors__expected_items)
{
    const auto inv = make_mixed_inventory();

    const auto txs = inv.select(inventory_item::selector::txids);
    BOOST_REQUIRE_EQUAL(txs.size(), 2u);
    BOOST_CHECK(txs[0].type == inventory_item::type_id::transaction);
    BOOST_CHECK(txs[1].type == inventory_item::type_id::witness_tx);

    const auto wtxs = inv.select(inventory_item::selector::wtxids);
    BOOST_REQUIRE_EQUAL(wtxs.size(), 1u);
    BOOST_CHECK(wtxs[0].type == inventory_item::type_id::wtxid);

    const auto blocks = inv.select(inventory_item::selector::blocks);
    BOOST_REQUIRE_EQUAL(blocks.size(), 2u);
    BOOST_CHECK(blocks[0].type == inventory_item::type_id::block);
    BOOST_CHECK(blocks[1].type == inventory_item::type_id::witness_block);

    const auto filters = inv.select(inventory_item::selector::filters);
    BOOST_REQUIRE_EQUAL(filters.size(), 1u);
    BOOST_CHECK(filters[0].type == inventory_item::type_id::filtered);
}

BOOST_AUTO_TEST_CASE(inventory__filter__types__expected_subset)
{
    const auto inv = make_mixed_inventory();

    const auto txs = inv.filter(inventory_item::type_id::transaction);
    BOOST_REQUIRE_EQUAL(txs.size(), 1u);
    BOOST_CHECK(txs[0].type == inventory_item::type_id::transaction);
    BOOST_CHECK(txs[0].hash == hash_tx);

    const auto witness_txs = inv.filter(inventory_item::type_id::witness_tx);
    BOOST_REQUIRE_EQUAL(witness_txs.size(), 1u);
    BOOST_CHECK(witness_txs[0].type == inventory_item::type_id::witness_tx);
    BOOST_CHECK(witness_txs[0].hash == hash_witness_tx);

    const auto witness_blocks = inv.filter(inventory_item::type_id::witness_block);
    BOOST_REQUIRE_EQUAL(witness_blocks.size(), 1u);
    BOOST_CHECK(witness_blocks[0].hash == hash_witness_block);

    const auto none = inv.filter(inventory_item::type_id::error);
    BOOST_CHECK(none.empty());
}

BOOST_AUTO_TEST_CASE(inventory__to_hashes__specific_type__expected_hashes)
{
    const auto inv = make_mixed_inventory();

    const auto tx_hashes = inv.to_hashes(inventory_item::type_id::transaction);
    BOOST_REQUIRE_EQUAL(tx_hashes.size(), 1u);
    BOOST_CHECK(tx_hashes[0] == hash_tx);

    const auto block_hashes = inv.to_hashes(inventory_item::type_id::block);
    BOOST_REQUIRE_EQUAL(block_hashes.size(), 1u);
    BOOST_CHECK(block_hashes[0] == hash_block);

    const auto error_hashes = inv.to_hashes(inventory_item::type_id::error);
    BOOST_CHECK(error_hashes.empty());
}

BOOST_AUTO_TEST_CASE(inventory__count__selector__expected_counts)
{
    const auto inv = make_mixed_inventory();

    BOOST_CHECK_EQUAL(inv.count(inventory_item::selector::txids), 2u);
    BOOST_CHECK_EQUAL(inv.count(inventory_item::selector::wtxids), 1u);
    BOOST_CHECK_EQUAL(inv.count(inventory_item::selector::blocks), 2u);
    BOOST_CHECK_EQUAL(inv.count(inventory_item::selector::filters), 1u);
}

BOOST_AUTO_TEST_CASE(inventory__count__type_id__expected_counts)
{
    const auto inv = make_mixed_inventory();

    BOOST_CHECK_EQUAL(inv.count(inventory_item::type_id::transaction), 1u);
    BOOST_CHECK_EQUAL(inv.count(inventory_item::type_id::block), 1u);
    BOOST_CHECK_EQUAL(inv.count(inventory_item::type_id::witness_tx), 1u);
    BOOST_CHECK_EQUAL(inv.count(inventory_item::type_id::witness_block), 1u);
    BOOST_CHECK_EQUAL(inv.count(inventory_item::type_id::filtered), 1u);
    BOOST_CHECK_EQUAL(inv.count(inventory_item::type_id::wtxid), 1u);
    BOOST_CHECK_EQUAL(inv.count(inventory_item::type_id::compact), 1u);
    BOOST_CHECK_EQUAL(inv.count(inventory_item::type_id::error), 0u);
}

BOOST_AUTO_TEST_CASE(inventory__any__type_id__expected_presence)
{
    const auto inv = make_mixed_inventory();

    BOOST_CHECK(inv.any(inventory_item::type_id::transaction));
    BOOST_CHECK(inv.any(inventory_item::type_id::block));
    BOOST_CHECK(inv.any(inventory_item::type_id::witness_tx));
    BOOST_CHECK(inv.any(inventory_item::type_id::witness_block));
    BOOST_CHECK(!inv.any(inventory_item::type_id::error));
}

BOOST_AUTO_TEST_CASE(inventory__any_transaction__mixed_items__expected)
{
    auto inv = make_mixed_inventory();
    BOOST_CHECK(inv.any_transaction());

    inv.items.clear();
    BOOST_CHECK(!inv.any_transaction());

    inv.items.emplace_back(inventory_item{ inventory_item::type_id::witness_tx, hash_tx });
    BOOST_CHECK(inv.any_transaction());
}

BOOST_AUTO_TEST_CASE(inventory__any_block__mixed_items__expected)
{
    auto inv = make_mixed_inventory();
    BOOST_CHECK(inv.any_block());

    inv.items.clear();
    BOOST_CHECK(!inv.any_block());

    inv.items.emplace_back(inventory_item{ inventory_item::type_id::block, hash_block });
    BOOST_CHECK(inv.any_block());
}

BOOST_AUTO_TEST_CASE(inventory__any_witness__mixed_items__expected)
{
    auto inv = make_mixed_inventory();
    BOOST_CHECK(inv.any_witness());

    inv.items.clear();
    BOOST_CHECK(!inv.any_witness());

    inv.items.emplace_back(inventory_item{ inventory_item::type_id::witness_tx, hash_witness_tx });
    BOOST_CHECK(inv.any_witness());
}

// std::ranges::distance requires non-const view (due to potential underlying caching).
BOOST_AUTO_TEST_CASE(inventory__view__specific_type__filtered_view)
{
    const auto inv = make_mixed_inventory();

    auto tx_view = inv.view(inventory_item::type_id::transaction);
    BOOST_CHECK(is_one(std::ranges::distance(tx_view)));

    auto block_view = inv.view(inventory_item::type_id::block);
    BOOST_CHECK(is_one(std::ranges::distance(block_view)));

    auto error_view = inv.view(inventory_item::type_id::error);
    BOOST_CHECK(is_zero(std::ranges::distance(error_view)));
}

static const auto genesis_block_hash = base16_hash("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");
static const auto genesis_tx_hash = base16_hash("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b");
static const auto genesis_payload = base16_chunk("02020000006fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000010000003ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a");

BOOST_AUTO_TEST_CASE(inventory__factory__move_hashes__expected)
{
    auto hashes = system::hashes{ genesis_block_hash, genesis_tx_hash };
    const auto inv = inventory::factory(std::move(hashes), inventory_item::type_id::witness_block);
    BOOST_REQUIRE_EQUAL(inv.items.size(), two);
    BOOST_REQUIRE(inv.items.front().type == inventory_item::type_id::witness_block);
    BOOST_REQUIRE_EQUAL(inv.items.front().hash, genesis_block_hash);
    BOOST_REQUIRE(inv.items.back().type == inventory_item::type_id::witness_block);
    BOOST_REQUIRE_EQUAL(inv.items.back().hash, genesis_tx_hash);
}

BOOST_AUTO_TEST_CASE(inventory__factory__copy_hashes__expected)
{
    const system::hashes hashes{ genesis_block_hash, genesis_tx_hash };
    const auto inv = inventory::factory(hashes, inventory_item::type_id::transaction);
    BOOST_REQUIRE_EQUAL(inv.items.size(), two);
    BOOST_REQUIRE(inv.items.front().type == inventory_item::type_id::transaction);
    BOOST_REQUIRE_EQUAL(inv.items.front().hash, genesis_block_hash);
    BOOST_REQUIRE(inv.items.back().type == inventory_item::type_id::transaction);
    BOOST_REQUIRE_EQUAL(inv.items.back().hash, genesis_tx_hash);
}

BOOST_AUTO_TEST_CASE(inventory__factory__empty__empty)
{
    const auto inv = inventory::factory(system::hashes{}, inventory_item::type_id::block);
    BOOST_REQUIRE(inv.items.empty());
}

BOOST_AUTO_TEST_CASE(inventory__deserialize1__genesis__expected)
{
    const auto message = inventory::deserialize(level::minimum_protocol, genesis_payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->items.size(), two);
    BOOST_REQUIRE(message->items.front().type == inventory_item::type_id::block);
    BOOST_REQUIRE_EQUAL(message->items.front().hash, genesis_block_hash);
    BOOST_REQUIRE(message->items.back().type == inventory_item::type_id::transaction);
    BOOST_REQUIRE_EQUAL(message->items.back().hash, genesis_tx_hash);
}

BOOST_AUTO_TEST_CASE(inventory__deserialize1__empty__expected)
{
    const auto message = inventory::deserialize(level::minimum_protocol, base16_chunk("00"));
    BOOST_REQUIRE(message);
    BOOST_REQUIRE(message->items.empty());
}

BOOST_AUTO_TEST_CASE(inventory__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!inventory::deserialize(level::minimum_protocol - 1u, genesis_payload));
}

BOOST_AUTO_TEST_CASE(inventory__deserialize1__excessive_version__nullptr)
{
    BOOST_REQUIRE(!inventory::deserialize(level::maximum_protocol + 1u, genesis_payload));
}

BOOST_AUTO_TEST_CASE(inventory__deserialize1__underflow__nullptr)
{
    const auto data = base16_chunk("0202000000");
    BOOST_REQUIRE(!inventory::deserialize(level::minimum_protocol, data));
}

BOOST_AUTO_TEST_CASE(inventory__deserialize1__excess_count__nullptr)
{
    const auto data = base16_chunk("fe51c30000");
    BOOST_REQUIRE(!inventory::deserialize(level::minimum_protocol, data));
}

BOOST_AUTO_TEST_CASE(inventory__serialize1__genesis__expected)
{
    const inventory inv{ { { inventory_item::type_id::block, genesis_block_hash }, { inventory_item::type_id::transaction, genesis_tx_hash } } };
    BOOST_REQUIRE_EQUAL(inv.size(level::minimum_protocol), genesis_payload.size());
    data_chunk data(inv.size(level::minimum_protocol));
    BOOST_REQUIRE(inv.serialize(level::minimum_protocol, data));
    BOOST_REQUIRE_EQUAL(data, genesis_payload);
}

BOOST_AUTO_TEST_SUITE_END()
