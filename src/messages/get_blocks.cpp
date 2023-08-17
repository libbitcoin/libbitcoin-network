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
#include <bitcoin/network/messages/get_blocks.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/enums/magic_numbers.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;
    
const identifier get_blocks::id = identifier::get_blocks;
const std::string get_blocks::command = "getblocks";
const uint32_t get_blocks::version_minimum = level::minimum_protocol;
const uint32_t get_blocks::version_maximum = level::maximum_protocol;

// static
// Predict the size of heights output.
size_t get_blocks::locator_size(size_t top) NOEXCEPT
{
    auto size = zero, step = one;

    for (auto height = top; height > 0; height = floored_subtract(height, step))
        if (++size > 9u)
            step = shift_left(step, one);

    return ++size;
}

// static
// This algorithm is a p2p best practice, not a consensus or p2p rule.
get_blocks::indexes get_blocks::heights(size_t top) NOEXCEPT
{
    auto step = one;
    indexes heights;
    heights.reserve(locator_size(top));

    // Start at top block and collect block indexes in reverse.
    for (auto height = top; height > 0; height = floored_subtract(height, step))
    {
        heights.push_back(height);

        // Push top 10 indexes then back off exponentially.
        if (heights.size() > 9u)
            step = shift_left(step, one);
    }

    // Push the genesis block index.
    heights.push_back(zero);
    return heights;
}

// static
typename get_blocks::cptr get_blocks::deserialize(uint32_t version,
    const system::data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader));
    return reader ? message : nullptr;
}

// static
get_blocks get_blocks::deserialize(uint32_t version, reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    const auto read_start_hashes = [](reader& source) NOEXCEPT
    {
        // Count of hashes is redundant with the message size.
        const auto count = source.read_size(max_get_blocks);

        hashes start_hashes;
        start_hashes.reserve(count);

        for (size_t hash = 0; hash < count; ++hash)
            start_hashes.push_back(source.read_hash());

        return start_hashes;
    };

    // Protocol version is stoopid (and unused).
    source.skip_bytes(sizeof(uint32_t));

    return
    {
        ////source.read_4_bytes_little_endian(),
        read_start_hashes(source),
        source.read_hash()
    };
}

bool get_blocks::serialize(uint32_t version,
    const system::data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer);
    return writer;
}

void get_blocks::serialize(uint32_t version, writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    // Write version vs. member protocol_version.
    ////sink.write_4_bytes_little_endian(protocol_version);
    sink.write_4_bytes_little_endian(version);

    // Count of hashes is redundant with the message size.
    sink.write_variable(start_hashes.size());

    for (const auto& start_hash: start_hashes)
        sink.write_bytes(start_hash);

    sink.write_bytes(stop_hash);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t get_blocks::size(uint32_t) const NOEXCEPT
{
    return sizeof(uint32_t) +
        hash_size +
        variable_size(start_hashes.size()) +
            (hash_size * start_hashes.size());
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
