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
#include <bitcoin/network/messages/block.hpp>

#include <iterator>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/memory.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>
#include <bitcoin/network/messages/transaction.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;

////BC_PUSH_WARNING(NO_MALLOC_OR_FREE)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(NO_UNGUARDED_POINTERS)

// Measured through block 840,000 - assumes consistent platform sizing.
constexpr auto maximal_block = 30'000'000_size;
    
const std::string block::command = "block";
const identifier block::id = identifier::block;
const uint32_t block::version_minimum = level::minimum_protocol;
const uint32_t block::version_maximum = level::maximum_protocol;

// static
void block::set_hashes(const chain::block& block, const data_chunk& data) NOEXCEPT
{
    constexpr auto header_size = chain::header::serialized_size();

    // Cache header hash.
    block.header().set_hash(bitcoin_hash(header_size, data.data()));

    // Skip transaction count, guarded by preceding successful block construct.
    auto start = std::next(data.data(), header_size);
    std::advance(start, size_variable(*start));

    // Cache transaction hashes.
    auto coinbase = true;
    for (const auto& tx: *block.transactions_ptr())
    {
        const auto witness_size = tx->serialized_size(true);

        // If !witness then wire txs cannot have been segregated.
        if (tx->is_segregated())
        {
            const auto nominal_size = tx->serialized_size(false);

            tx->set_nominal_hash(transaction::desegregated_hash(
                witness_size, nominal_size, start));

            if (!coinbase)
                tx->set_witness_hash(bitcoin_hash(witness_size, start));
        }
        else
        {
            tx->set_nominal_hash(bitcoin_hash(witness_size, start));
        }

        coinbase = false;
        std::advance(start, witness_size);
    }
}

// static
typename block::cptr block::deserialize(uint32_t version,
    const system::data_chunk& data, bool witness) NOEXCEPT
{
    static default_memory memory{};
    return deserialize(*memory.get_arena(), version, data, witness);
}

// static
// WARNING: linear arena must allocate a maximal block and all contents.
// WARNING: chain::block::cptr destruct frees all contained shared_ptr objects.
typename block::cptr block::deserialize(arena& arena, uint32_t version,
    const data_chunk& data, bool witness) NOEXCEPT
{
    ////if (version < version_minimum || version > version_maximum)
    ////    nullptr;

    // TODO: arena::initialize().
    // Returns current arena address (next allocation point).
    // This implies a linear allocator, otherwise nullptr returned.
    const auto begin = pointer_cast<uint8_t>(arena.initialize());
    if (is_null(begin))
        return nullptr;

    istream source{ data };
    byte_reader reader{ source, &arena };

    // block and all of its parts are allocated at the start of arena.
    ////auto allocator = reader.get_allocator();
    ////const auto raw = allocator.new_object<chain::block>(reader, witness);
    ////if (is_null(raw) || !reader)
    ////    return nullptr;
    const auto message = to_shared(deserialize(version, reader, witness));
    if (!reader)
        return nullptr;

    // Copy header and tx hashes into preallocated optionals.
    set_hashes(*message->block_ptr, data);

    // Compute allocated size of raw and its block.
    const auto end = pointer_cast<uint8_t>(arena.allocate(zero));
    const auto allocation = std::distance(begin, end);
    const auto size = possible_narrow_sign_cast<size_t>(allocation);
    message->block_ptr->set_allocation(size);

    // Invokes destruct and deallocate (which may not need to free memory).
    ////return std::make_shared<block>(std::shared_ptr<chain::block>(raw,
    ////    allocator.deleter<chain::block>()));
    return message;

    // Allocate memory for copy of block_ptr and its block, and define deleter.
    ////const auto copy = pointer_cast<chain::block>(std::malloc(size));
    ////if (is_null(copy))
    ////    return nullptr;
    ////
    // Copy the contiguous byte buffer that represents the block.
    ////std::memcpy(copy, block_ptr, size);
    ////
    // Do not invoke block destructor, no free required for linear allocator.
    ////allocator.deleter<chain::block>()(block_ptr);
    ////std::free(copy);
    ////
    ////const auto thread = std::this_thread::get_id();
    ////const auto wiper = [thread](void* memory) NOEXCEPT
    ////{
    ////    if (std::this_thread::get_id() != thread)
    ////    {
    ////        std::free(memory);
    ////    }
    ////};
    ////
    // This leaks copy if destruct on same thread.
    ////return std::make_shared<block>(std::shared_ptr<chain::block>(copy, wiper));
    ////
    // Create an owning block pointer with deallocator and assign to message.
    ////const auto wiper = [](void* address) NOEXCEPT { std::free(address); };
    ////return std::make_shared<block>(std::shared_ptr<chain::block>(copy, wiper));
}

// static
block block::deserialize(uint32_t version, reader& source,
    bool witness) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    return { to_shared<chain::block>(source, witness) };
}

bool block::serialize(uint32_t version,
    const system::data_slab& data, bool witness) const NOEXCEPT
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

    if (block_ptr)
        block_ptr->to_data(sink, witness);

    BC_ASSERT(sink && (sink.get_write_position() - start) == bytes);
}

size_t block::size(uint32_t, bool witness) const NOEXCEPT
{
    return block_ptr ? block_ptr->serialized_size(witness) : zero;
}

BC_POP_WARNING()
BC_POP_WARNING()
////BC_POP_WARNING()

} // namespace messages
} // namespace network
} // namespace libbitcoin
