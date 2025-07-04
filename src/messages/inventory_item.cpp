/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/messages/inventory_item.hpp>

#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;

// static
uint32_t inventory_item::to_number(type_id inventory_type) NOEXCEPT
{
    return static_cast<uint32_t>(inventory_type);
}

// static
inventory_item::type_id inventory_item::to_type(uint32_t value) NOEXCEPT
{
    return static_cast<type_id>(value);
}

// static
std::string inventory_item::to_string(type_id inventory_type) NOEXCEPT
{
    switch (inventory_type)
    {
        case type_id::transaction:
            return "transaction";
        case type_id::block:
            return "block";
        case type_id::filtered:
            return "filtered";
        case type_id::compact:
            return "compact";
        case type_id::wtxid:
            return "wtxid";
        case type_id::witness_tx:
            return "witness_tx";
        case type_id::witness_block:
            return "witness_block";
        case type_id::witness_filtered:
            return "witness_filtered";
        case type_id::witness_compact:
            return "witness_compact";
        case type_id::error:
        default:
            return "error";
    }
}

// static
size_t inventory_item::size(uint32_t) NOEXCEPT
{
    return hash_size
        + sizeof(uint32_t);
}

inventory_item inventory_item::deserialize(uint32_t, reader& source) NOEXCEPT
{
    return
    {
        to_type(source.read_4_bytes_little_endian()),
        source.read_hash()
    };
}

void inventory_item::serialize(uint32_t BC_DEBUG_ONLY(version),
    writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_4_bytes_little_endian(to_number(type));
    sink.write_bytes(hash);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

bool inventory_item::is_block_type() const NOEXCEPT
{
    // Filtered are bip37, effectively deprecated by bip111.
    return type == type_id::witness_block
        || type == type_id::witness_compact
        || type == type_id::witness_filtered
        || type == type_id::block
        || type == type_id::compact
        || type == type_id::filtered;
}

bool inventory_item::is_transaction_type() const NOEXCEPT
{
    // Only wtxid corresponds to a witness hash.
    return type == type_id::witness_tx
        || type == type_id::transaction
        || type == type_id::wtxid;
}

bool inventory_item::is_witness_type() const NOEXCEPT
{
    // wtxid excluded
    return type == type_id::witness_tx
        || type == type_id::witness_block
        || type == type_id::witness_compact
        || type == type_id::witness_filtered;
}

bool operator==(const inventory_item& left, const inventory_item& right) NOEXCEPT
{
    return left.type == right.type
        && left.hash == right.hash;
}

bool operator!=(const inventory_item& left, const inventory_item& right) NOEXCEPT
{
    return !(left == right);
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
