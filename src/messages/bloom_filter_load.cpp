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
#include <bitcoin/network/messages/bloom_filter_load.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/enums/magic_numbers.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;
    
const std::string bloom_filter_load::command = "filterload";
const identifier bloom_filter_load::id = identifier::bloom_filter_load;
const uint32_t bloom_filter_load::version_minimum = level::bip37;
const uint32_t bloom_filter_load::version_maximum = level::maximum_protocol;

// static
typename bloom_filter_load::cptr bloom_filter_load::deserialize(
    uint32_t version, const system::data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader));
    return reader ? message : nullptr;
}

// static
bloom_filter_load bloom_filter_load::deserialize(uint32_t version,
    reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    const auto read_hash_functions = [](reader& source) NOEXCEPT
    {
        const auto hash_functions = source.read_4_bytes_little_endian();

        if (hash_functions > max_bloom_filter_functions)
            source.invalidate();

        return hash_functions;
    };

    return
    {
        source.read_bytes(source.read_size(max_bloom_filter_load)),
        read_hash_functions(source),
        source.read_4_bytes_little_endian(),
        source.read_byte()
    };
}

bool bloom_filter_load::serialize(uint32_t version,
    const system::data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer);
    return writer;
}

void bloom_filter_load::serialize(uint32_t BC_DEBUG_ONLY(version),
    writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_variable(filter.size());
    sink.write_bytes(filter);
    sink.write_4_bytes_little_endian(hash_functions);
    sink.write_4_bytes_little_endian(tweak);
    sink.write_byte(flags);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t bloom_filter_load::size(uint32_t) const NOEXCEPT
{
    return variable_size(filter.size()) + filter.size() +
        + sizeof(uint32_t)
        + sizeof(uint32_t)
        + sizeof(uint8_t);
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
