/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/messages/peer/detail/block.hpp>

#include <iterator>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/messages/peer/detail/transaction.hpp>
#include <bitcoin/network/messages/peer/enums/identifier.hpp>
#include <bitcoin/network/messages/peer/enums/level.hpp>
#include <bitcoin/network/messages/peer/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(NO_UNGUARDED_POINTERS)
    
const std::string block::command = "block";
const identifier block::id = identifier::block;
const uint32_t block::version_minimum = level::minimum_protocol;
const uint32_t block::version_maximum = level::maximum_protocol;

// static
typename block::cptr block::deserialize(uint32_t version,
    const data_chunk& data, bool witness) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        return {};

    const auto message = emplace_shared<messages::peer::block>(
        chain::block_view{ move_copy(data), witness });

    return message->block.is_valid() ? message : nullptr;
}

// static
block block::deserialize(uint32_t version, reader& source,
    bool witness) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        return { chain::block_view{ data_chunk{}, witness } };

    return { chain::block_view{ source.read_bytes(), witness } };
}

bool block::serialize(uint32_t version, const data_slab& data,
    bool witness) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer, witness);
    return writer;
}

void block::serialize(uint32_t BC_DEBUG_ONLY(version), writer& sink,
    bool witness) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version, witness);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_bytes(block.to_data(witness));
    BC_ASSERT(sink && (sink.get_write_position() - start) == bytes);
}

size_t block::size(uint32_t, bool witness) const NOEXCEPT
{
    return block.is_valid() ? block.serialized_size(witness) : zero;
}

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin
