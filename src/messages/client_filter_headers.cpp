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

// Sponsored in part by Digital Contract Design, LLC

#include <bitcoin/network/messages/client_filter_headers.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;
    
const std::string client_filter_headers::command = "cfheaders";
const identifier client_filter_headers::id = identifier::client_filter_headers;
const uint32_t client_filter_headers::version_minimum = level::bip157;
const uint32_t client_filter_headers::version_maximum = level::maximum_protocol;

// static
typename client_filter_headers::cptr client_filter_headers::deserialize(
    uint32_t version, const system::data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader));
    return reader ? message : nullptr;
}

// static
client_filter_headers client_filter_headers::deserialize(uint32_t version,
    reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    const auto read_filter_hashes = [](reader& source) NOEXCEPT
    {
        const auto size = source.read_size(chain::max_block_size);
        hashes filter_hashes;
        filter_hashes.reserve(size);

        for (size_t header = 0; header < size; header++)
            filter_hashes.push_back(source.read_hash());

        return filter_hashes;
    };

    return
    {
        source.read_byte(),
        source.read_hash(),
        source.read_hash(),
        read_filter_hashes(source)
    };
}

bool client_filter_headers::serialize(uint32_t version,
    const system::data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer);
    return writer;
}

void client_filter_headers::serialize(uint32_t BC_DEBUG_ONLY(version),
    writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_byte(filter_type);
    sink.write_bytes(stop_hash);
    sink.write_bytes(previous_filter_header);
    sink.write_variable(filter_hashes.size());

    for (const auto& hash: filter_hashes)
        sink.write_bytes(hash);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t client_filter_headers::size(uint32_t) const NOEXCEPT
{
    return sizeof(uint8_t)
        + hash_size
        + hash_size
        + variable_size(filter_hashes.size()) +
            (filter_hashes.size() * hash_size);
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
