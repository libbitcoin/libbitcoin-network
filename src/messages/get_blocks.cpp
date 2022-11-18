/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
    
const identifier get_blocks::id = identifier::get_blocks;
const std::string get_blocks::command = "getblocks";
const uint32_t get_blocks::version_minimum = level::minimum_protocol;
const uint32_t get_blocks::version_maximum = level::maximum_protocol;

// static
// Predict the size of locator_heights output.
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
get_blocks::indexes get_blocks::locator_heights(size_t top) NOEXCEPT
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

get_blocks get_blocks::deserialize(uint32_t version, reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    const auto read_start_hashes = [](reader& source) NOEXCEPT
    {
        const auto size = source.read_size(max_get_blocks);
        hashes start_hashes;
        start_hashes.reserve(size);

        for (size_t hash = 0; hash < size; ++hash)
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

void get_blocks::serialize(uint32_t version, writer& sink) const NOEXCEPT
{
    // sink.get_position() removed due to flipper conflict, commenting out debug
    // BC_DEBUG_ONLY(const auto bytes = size(version);)
    // BC_DEBUG_ONLY(const auto start = sink.get_position();)

    // Write version vs. member protocol_version.
    ////sink.write_4_bytes_little_endian(protocol_version);
    sink.write_4_bytes_little_endian(version);
    sink.write_variable(start_hashes.size());

    for (const auto& start_hash: start_hashes)
        sink.write_bytes(start_hash);

    sink.write_bytes(stop_hash);

    // sink.get_position() removed due to flipper conflict, commenting out debug
    // BC_ASSERT(sink && sink.get_position() - start == bytes);
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
