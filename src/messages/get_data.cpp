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
#include <bitcoin/network/messages/get_data.hpp>

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/enums/magic_numbers.hpp>
#include <bitcoin/network/messages/inventory_item.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

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

////// TODO: add inventory factory witness parameter (once node is ready).
////// Requires a non-const instance for this in-place efficiency.
////void get_data::to_witness() NOEXCEPT
////{
////    const auto convert = [](inventory_item& item) NOEXCEPT
////    {
////        item.to_witness();
////    };
////
////    std::for_each(items.begin(), items.end(), convert);
////}

} // namespace messages
} // namespace network
} // namespace libbitcoin
