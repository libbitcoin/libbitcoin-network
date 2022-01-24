/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_INVENTORY_ITEM_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_INVENTORY_ITEM_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

/// This is also known as a "inventory vector".
struct BCT_API inventory_item
{
    typedef std::shared_ptr<const inventory_item> ptr;

    enum class type_id : uint32_t
    {
        error = 0,
        transaction = 1,
        block = 2,
        filtered_block = 3,
        compact_block = 4,
        witness = system::shift_left<uint32_t>(1, 30),
        witness_transaction = witness | transaction,
        witness_block = witness | block,
        reserved = witness | filtered_block
    };

    static type_id to_type(uint32_t value);
    static uint32_t to_number(type_id type);
    static std::string to_string(type_id type);

    static size_t size(uint32_t version);
    static inventory_item deserialize(uint32_t version, system::reader& source);
    void serialize(uint32_t version, system::writer& sink) const;

    bool is_block_type() const;
    bool is_transaction_type() const;
    bool is_witnessable_type() const;

    /// Convert message types to witness types.
    void to_witness();

    type_id type;
    system::hash_digest hash;
};

typedef std::vector<inventory_item> inventory_items;

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
