/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/messages/compact_block_item.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;

compact_block_item compact_block_item::deserialize(uint32_t, reader& source,
    bool witness) NOEXCEPT
{
    return 
    {
        source.read_variable(),
        to_shared<chain::transaction>(source, witness)
    };
}

void compact_block_item::serialize(uint32_t BC_DEBUG_ONLY(version),
    writer& sink, bool witness) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version, witness);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_variable(index);
    if (transaction_ptr)
        transaction_ptr->to_data(sink, witness);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t compact_block_item::size(uint32_t, bool witness) const NOEXCEPT
{
    return sizeof(uint64_t)
        + (transaction_ptr ? transaction_ptr->serialized_size(witness) : zero);
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
