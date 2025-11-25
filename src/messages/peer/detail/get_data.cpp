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
#include <bitcoin/network/messages/peer/detail/get_data.hpp>

#include <algorithm>
#include <bitcoin/network/messages/peer/enums/identifier.hpp>
#include <bitcoin/network/messages/peer/enums/level.hpp>
#include <bitcoin/network/messages/peer/enums/magic_numbers.hpp>
#include <bitcoin/network/messages/peer/detail/inventory_item.hpp>
#include <bitcoin/network/messages/peer/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

using namespace system;

// filtered_block flag allowed by bip37.
const std::string get_data::command = "getdata";
const identifier get_data::id = identifier::get_data;
const uint32_t get_data::version_minimum = level::minimum_protocol;
const uint32_t get_data::version_maximum = level::maximum_protocol;

// static
typename get_data::cptr get_data::deserialize(uint32_t version,
    const system::data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader));
    return reader ? message : nullptr;
}

// static
get_data get_data::deserialize(uint32_t version, reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    const auto size = source.read_size(max_inventory);
    get_data get;
    get.items.reserve(size);

    for (size_t item = 0; item < size; ++item)
        get.items.push_back(inventory_item::deserialize(version, source));

    return get;
}

bool get_data::serialize(uint32_t version,
    const system::data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer);
    return writer;
}

void get_data::serialize(uint32_t version, writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
        BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

        sink.write_variable(items.size());

    for (const auto& item: items)
        item.serialize(version, sink);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t get_data::size(uint32_t version) const NOEXCEPT
{
    return variable_size(items.size()) +
        (items.size() * inventory_item::size(version));
}

inventory_items get_data::filter(type_id type) const NOEXCEPT
{
    inventory_items out;
    out.reserve(count(type));

    for (const auto& item: items)
        if (item.type == type)
            out.push_back(item);

    return out;
}

hashes get_data::to_hashes(type_id type) const NOEXCEPT
{
    hashes out;
    out.reserve(count(type));

    for (const auto& item: items)
        if (item.type == type)
            out.push_back(item.hash);

    return out;
}

size_t get_data::count(type_id type) const NOEXCEPT
{
    const auto is_type = [type](const inventory_item& item)
    {
        return item.type == type;
    };

    return std::count_if(items.begin(), items.end(), is_type);
}

bool get_data::any(type_id type) const NOEXCEPT
{
    const auto is_type = [type](const inventory_item& item)
    {
        return item.type == type;
    };

    return std::any_of(items.begin(), items.end(), is_type);
}

bool get_data::any_transaction() const NOEXCEPT
{
    const auto is_transaction = [](const inventory_item& item)
    {
        return item.is_transaction_type();
    };

    return std::any_of(items.begin(), items.end(), is_transaction);
}

bool get_data::any_block() const NOEXCEPT
{
    const auto is_block = [](const inventory_item& item)
    {
        return item.is_block_type();
    };

    return std::any_of(items.begin(), items.end(), is_block);
}

bool get_data::any_witness() const NOEXCEPT
{
    const auto is_witness = [](const inventory_item& item)
    {
        return item.is_witness_type();
    };

    return std::any_of(items.begin(), items.end(), is_witness);
}

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin
