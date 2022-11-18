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
#include <bitcoin/network/messages/merkle_block.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace bc::system;
    
const std::string merkle_block::command = "merkleblock";
const identifier merkle_block::id = identifier::merkle_block;
const uint32_t merkle_block::version_minimum = level::bip37;
const uint32_t merkle_block::version_maximum = level::maximum_protocol;

merkle_block merkle_block::deserialize(uint32_t, reader& source) NOEXCEPT
{
    const auto read_hashes = [](reader& source) NOEXCEPT
    {
        const auto size = source.read_size(chain::max_block_size);
        system::hashes hashes;
        hashes.reserve(size);

        for (size_t hash = 0; hash < size; ++hash)
            hashes.push_back(source.read_hash());

        return hashes;
    };

    return
    {
        to_shared(new chain::header{ source }),
        source.read_4_bytes_little_endian(),
        read_hashes(source),
        source.read_bytes(source.read_size(chain::max_block_size))
    };
}

void merkle_block::serialize(uint32_t BC_DEBUG_ONLY(version),
    writer& sink) const NOEXCEPT
{
    // sink.get_position() removed due to flipper conflict, commenting out debug
    // BC_DEBUG_ONLY(const auto bytes = size(version);)
    // BC_DEBUG_ONLY(const auto start = sink.get_position();)

    if (header)
        header->to_data(sink);

    sink.write_4_bytes_little_endian(transactions);
    sink.write_variable(hashes.size());

    for (const auto& hash: hashes)
        sink.write_bytes(hash);

    sink.write_variable(flags.size());
    sink.write_bytes(flags);

    // sink.get_position() removed due to flipper conflict, commenting out debug
    // BC_ASSERT(sink && sink.get_position() - start == bytes);
}

size_t merkle_block::size(uint32_t) const NOEXCEPT
{
    return header ? header->serialized_size() : zero
        + sizeof(uint32_t)
        + variable_size(hashes.size()) + (hashes.size() * hash_size)
        + variable_size(flags.size()) + flags.size();
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
