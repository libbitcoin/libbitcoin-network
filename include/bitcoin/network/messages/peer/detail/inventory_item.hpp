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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_PEER_INVENTORY_ITEM_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_PEER_INVENTORY_ITEM_HPP

#include <memory>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

/// This is also known as a "inventory vector".
struct BCT_API inventory_item
{
    typedef std::shared_ptr<const inventory_item> cptr;

    /// Logical type selection.
    enum class selector
    {
        txids,
        wtxids,
        blocks,
        filters
    };

    /// Serializeable type id.
    enum class type_id : uint32_t
    {
        // It's so dumb that these are not flags.
        error       = 0,
        transaction = 1,
        block       = 2,
        filtered    = 3,
        compact     = 4,
        wtxid       = 5,

        // But witness is a flag.
        witness          = system::bit_right<uint32_t>(30),
        witness_tx       = witness | transaction,
        witness_block    = witness | block,
        witness_filtered = witness | filtered,
        witness_compact  = witness | compact
    };

    static type_id to_type(uint32_t value) NOEXCEPT;
    static uint32_t to_number(type_id type) NOEXCEPT;
    static std::string to_string(type_id type) NOEXCEPT;

    static size_t size(uint32_t version) NOEXCEPT;
    static inventory_item deserialize(uint32_t version,
        system::reader& source) NOEXCEPT;
    void serialize(uint32_t version, system::writer& sink) const NOEXCEPT;

    /// These indicate relevance to a specific object identifier.
    bool is_txid() const NOEXCEPT;
    bool is_wtxid() const NOEXCEPT;
    bool is_block() const NOEXCEPT;
    bool is_filter() const NOEXCEPT;
    bool is_selected(selector types) const NOEXCEPT;

    /// These indicate context, not object identifier.
    bool is_block_type() const NOEXCEPT;
    bool is_transaction_type() const NOEXCEPT;
    bool is_witness_type() const NOEXCEPT;
    bool is_type(type_id id) const NOEXCEPT;

    type_id type;
    system::hash_digest hash;
};

bool operator==(const inventory_item& left, const inventory_item& right) NOEXCEPT;
bool operator!=(const inventory_item& left, const inventory_item& right) NOEXCEPT;

typedef std_vector<inventory_item> inventory_items;

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
