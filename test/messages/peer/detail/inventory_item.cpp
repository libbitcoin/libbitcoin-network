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

BOOST_AUTO_TEST_SUITE(p2p_inventory_item_tests)

using namespace network::messages::peer;

// to_number
// to_type
// to_string
// is_block_type
// is_transaction_type

BOOST_AUTO_TEST_CASE(inventory_item__size__always__expected)
{
    constexpr auto expected = system::hash_size
        + sizeof(uint32_t);

    BOOST_REQUIRE_EQUAL(inventory_item::size(level::canonical), expected);
}

using type_id = inventory_item::type_id;
using selector = inventory_item::selector;

static const inventory_item error_item{ type_id::error, {} };
static const inventory_item transaction_item{ type_id::transaction, {} };
static const inventory_item block_item{ type_id::block, {} };
static const inventory_item filtered_item{ type_id::filtered, {} };
static const inventory_item compact_item{ type_id::compact, {} };
static const inventory_item wtxid_item{ type_id::wtxid, {} };
static const inventory_item witness_tx_item{ type_id::witness_tx, {} };
static const inventory_item witness_block_item{ type_id::witness_block, {} };
static const inventory_item witness_filtered_item{ type_id::witness_filtered, {} };
static const inventory_item witness_compact_item{ type_id::witness_compact, {} };

static const auto genesis_block_hash = system::base16_hash("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");
static const auto genesis_block_item = system::base16_chunk("020000006fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000");

BOOST_AUTO_TEST_CASE(inventory_item__to_number__all__expected)
{
    BOOST_REQUIRE_EQUAL(inventory_item::to_number(type_id::error), 0u);
    BOOST_REQUIRE_EQUAL(inventory_item::to_number(type_id::transaction), 1u);
    BOOST_REQUIRE_EQUAL(inventory_item::to_number(type_id::block), 2u);
    BOOST_REQUIRE_EQUAL(inventory_item::to_number(type_id::filtered), 3u);
    BOOST_REQUIRE_EQUAL(inventory_item::to_number(type_id::compact), 4u);
    BOOST_REQUIRE_EQUAL(inventory_item::to_number(type_id::wtxid), 5u);
    BOOST_REQUIRE_EQUAL(inventory_item::to_number(type_id::witness), 0x40000000u);
    BOOST_REQUIRE_EQUAL(inventory_item::to_number(type_id::witness_tx), 0x40000001u);
    BOOST_REQUIRE_EQUAL(inventory_item::to_number(type_id::witness_block), 0x40000002u);
    BOOST_REQUIRE_EQUAL(inventory_item::to_number(type_id::witness_filtered), 0x40000003u);
    BOOST_REQUIRE_EQUAL(inventory_item::to_number(type_id::witness_compact), 0x40000004u);
}

BOOST_AUTO_TEST_CASE(inventory_item__to_type__all__expected)
{
    BOOST_REQUIRE(inventory_item::to_type(0u) == type_id::error);
    BOOST_REQUIRE(inventory_item::to_type(1u) == type_id::transaction);
    BOOST_REQUIRE(inventory_item::to_type(2u) == type_id::block);
    BOOST_REQUIRE(inventory_item::to_type(3u) == type_id::filtered);
    BOOST_REQUIRE(inventory_item::to_type(4u) == type_id::compact);
    BOOST_REQUIRE(inventory_item::to_type(5u) == type_id::wtxid);
    BOOST_REQUIRE(inventory_item::to_type(0x40000001u) == type_id::witness_tx);
    BOOST_REQUIRE(inventory_item::to_type(0x40000002u) == type_id::witness_block);
    BOOST_REQUIRE(inventory_item::to_type(0x40000003u) == type_id::witness_filtered);
    BOOST_REQUIRE(inventory_item::to_type(0x40000004u) == type_id::witness_compact);
}

BOOST_AUTO_TEST_CASE(inventory_item__to_string__all__expected)
{
    BOOST_REQUIRE_EQUAL(inventory_item::to_string(type_id::error), "error");
    BOOST_REQUIRE_EQUAL(inventory_item::to_string(type_id::transaction), "transaction");
    BOOST_REQUIRE_EQUAL(inventory_item::to_string(type_id::block), "block");
    BOOST_REQUIRE_EQUAL(inventory_item::to_string(type_id::filtered), "filtered");
    BOOST_REQUIRE_EQUAL(inventory_item::to_string(type_id::compact), "compact");
    BOOST_REQUIRE_EQUAL(inventory_item::to_string(type_id::wtxid), "wtxid");
    BOOST_REQUIRE_EQUAL(inventory_item::to_string(type_id::witness_tx), "witness_tx");
    BOOST_REQUIRE_EQUAL(inventory_item::to_string(type_id::witness_block), "witness_block");
    BOOST_REQUIRE_EQUAL(inventory_item::to_string(type_id::witness_filtered), "witness_filtered");
    BOOST_REQUIRE_EQUAL(inventory_item::to_string(type_id::witness_compact), "witness_compact");
    BOOST_REQUIRE_EQUAL(inventory_item::to_string(type_id::witness), "error");
}

BOOST_AUTO_TEST_CASE(inventory_item__deserialize__genesis_block__expected)
{
    system::read::bytes::copy source(genesis_block_item);
    const auto item = inventory_item::deserialize(level::minimum_protocol, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE(source.is_exhausted());
    BOOST_REQUIRE(item.type == type_id::block);
    BOOST_REQUIRE_EQUAL(item.hash, genesis_block_hash);
}

BOOST_AUTO_TEST_CASE(inventory_item__deserialize__underflow__source_false)
{
    const auto data = system::base16_chunk("020000006fe28c0a");
    system::read::bytes::copy source(data);
    inventory_item::deserialize(level::minimum_protocol, source);
    BOOST_REQUIRE(!source);
}

BOOST_AUTO_TEST_CASE(inventory_item__serialize__genesis_block__expected)
{
    const inventory_item item{ type_id::block, genesis_block_hash };
    system::data_chunk data(inventory_item::size(level::minimum_protocol));
    system::write::bytes::copy sink(data);
    item.serialize(level::minimum_protocol, sink);
    BOOST_REQUIRE(sink);
    BOOST_REQUIRE_EQUAL(data, genesis_block_item);
}

BOOST_AUTO_TEST_CASE(inventory_item__serialize__witness_block__witness_flag)
{
    const inventory_item item{ type_id::witness_block, genesis_block_hash };
    system::data_chunk data(inventory_item::size(level::minimum_protocol));
    system::write::bytes::copy sink(data);
    item.serialize(level::minimum_protocol, sink);
    BOOST_REQUIRE(sink);
    BOOST_REQUIRE_EQUAL(data, system::base16_chunk("020000406fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000"));
}

BOOST_AUTO_TEST_CASE(inventory_item__is_selected__all__expected)
{
    const inventory_item tx{ type_id::transaction, {} };
    const inventory_item wtxid{ type_id::wtxid, {} };
    const inventory_item block{ type_id::witness_block, {} };
    const inventory_item filtered{ type_id::witness_filtered, {} };
    BOOST_REQUIRE(tx.is_selected(selector::txids));
    BOOST_REQUIRE(!tx.is_selected(selector::wtxids));
    BOOST_REQUIRE(wtxid.is_selected(selector::wtxids));
    BOOST_REQUIRE(!wtxid.is_selected(selector::txids));
    BOOST_REQUIRE(block.is_selected(selector::blocks));
    BOOST_REQUIRE(!block.is_selected(selector::filters));
    BOOST_REQUIRE(filtered.is_selected(selector::filters));
    BOOST_REQUIRE(!filtered.is_selected(selector::blocks));
}

BOOST_AUTO_TEST_CASE(inventory_item__is_selected__invalid_selector__false)
{
    const inventory_item tx{ type_id::transaction, {} };
    BOOST_REQUIRE(!tx.is_selected(static_cast<selector>(42)));
}

BOOST_AUTO_TEST_CASE(inventory_item__is_block_type__all__expected)
{
    BOOST_REQUIRE(block_item.is_block_type());
    BOOST_REQUIRE(compact_item.is_block_type());
    BOOST_REQUIRE(filtered_item.is_block_type());
    BOOST_REQUIRE(witness_block_item.is_block_type());
    BOOST_REQUIRE(witness_compact_item.is_block_type());
    BOOST_REQUIRE(witness_filtered_item.is_block_type());
    BOOST_REQUIRE(!transaction_item.is_block_type());
    BOOST_REQUIRE(!wtxid_item.is_block_type());
    BOOST_REQUIRE(!error_item.is_block_type());
}

BOOST_AUTO_TEST_CASE(inventory_item__is_transaction_type__all__expected)
{
    BOOST_REQUIRE(transaction_item.is_transaction_type());
    BOOST_REQUIRE(witness_tx_item.is_transaction_type());
    BOOST_REQUIRE(wtxid_item.is_transaction_type());
    BOOST_REQUIRE(!block_item.is_transaction_type());
    BOOST_REQUIRE(!error_item.is_transaction_type());
}

BOOST_AUTO_TEST_CASE(inventory_item__is_witness_type__all__expected)
{
    BOOST_REQUIRE(witness_tx_item.is_witness_type());
    BOOST_REQUIRE(witness_block_item.is_witness_type());
    BOOST_REQUIRE(witness_compact_item.is_witness_type());
    BOOST_REQUIRE(witness_filtered_item.is_witness_type());
    BOOST_REQUIRE(!wtxid_item.is_witness_type());
    BOOST_REQUIRE(!transaction_item.is_witness_type());
}

BOOST_AUTO_TEST_CASE(inventory_item__equality__same__true)
{
    const inventory_item left{ type_id::block, genesis_block_hash };
    const inventory_item right{ type_id::block, genesis_block_hash };
    BOOST_REQUIRE(left == right);
    BOOST_REQUIRE(!(left != right));
}

BOOST_AUTO_TEST_CASE(inventory_item__equality__distinct_type__false)
{
    const inventory_item left{ type_id::block, genesis_block_hash };
    const inventory_item right{ type_id::witness_block, genesis_block_hash };
    BOOST_REQUIRE(!(left == right));
    BOOST_REQUIRE(left != right);
}

BOOST_AUTO_TEST_CASE(inventory_item__equality__distinct_hash__false)
{
    const inventory_item left{ type_id::block, genesis_block_hash };
    const inventory_item right{ type_id::block, system::null_hash };
    BOOST_REQUIRE(!(left == right));
    BOOST_REQUIRE(left != right);
}

BOOST_AUTO_TEST_SUITE_END()
