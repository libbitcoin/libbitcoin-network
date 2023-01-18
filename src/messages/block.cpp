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
#include <bitcoin/network/messages/block.hpp>

#include <cstddef>
#include <cstdint>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace bc::system;
    
const std::string block::command = "block";
const identifier block::id = identifier::block;
const uint32_t block::version_minimum = level::minimum_protocol;
const uint32_t block::version_maximum = level::maximum_protocol;

// static
block block::deserialize(uint32_t version, reader& source,
    bool witness) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    return { to_shared<chain::block>(source, witness) };
}

void block::serialize(uint32_t BC_DEBUG_ONLY(version), writer& sink,
    bool witness) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version, witness);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    if (block_ptr)
        block_ptr->to_data(sink, witness);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t block::size(uint32_t, bool witness) const NOEXCEPT
{
    return block_ptr ? block_ptr->serialized_size(witness) : zero;
}

////static void foo()
////{
////    system::chain::block::cptr alpha;
////    auto beta = std::move(alpha);
////}

////static void bar()
////{
////    block alpha;
////    auto beta = std::move(alpha);
////}

} // namespace messages
} // namespace network
} // namespace libbitcoin
