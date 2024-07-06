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
#include <bitcoin/network/messages/get_compact_transactions.hpp>

#include <numeric>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;
    
const std::string get_compact_transactions::command = "getblocktxn";
const identifier get_compact_transactions::id = identifier::get_compact_transactions;
const uint32_t get_compact_transactions::version_minimum = level::bip152;
const uint32_t get_compact_transactions::version_maximum = level::maximum_protocol;

// static
typename get_compact_transactions::cptr get_compact_transactions::deserialize(
    uint32_t version, const system::data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader));
    return reader ? message : nullptr;
}

// static
get_compact_transactions get_compact_transactions::deserialize(uint32_t version,
    reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    const auto read_indexes = [](reader& source) NOEXCEPT
    {
        const auto size = source.read_size(chain::max_block_size);
        std_vector<uint64_t> indexes;
        indexes.reserve(size);

        for (size_t index = 0; index < size; ++index)
            indexes.push_back(source.read_size());

        return indexes;
    };

    return
    {
        source.read_hash(),
        read_indexes(source)
    };
}

bool get_compact_transactions::serialize(uint32_t version,
    const system::data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer);
    return writer;
}

void get_compact_transactions::serialize(uint32_t BC_DEBUG_ONLY(version),
    writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_bytes(block_hash);
    sink.write_variable(indexes.size());

    for (const auto& index: indexes)
        sink.write_variable(index);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t get_compact_transactions::size(uint32_t) const NOEXCEPT
{
    const auto values = [](size_t total, uint64_t output) NOEXCEPT
    {
        return total + variable_size(output);
    };

    return hash_size +
        variable_size(indexes.size()) +
            std::accumulate(indexes.begin(), indexes.end(), zero, values);
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
