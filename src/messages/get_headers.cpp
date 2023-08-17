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
#include <bitcoin/network/messages/get_headers.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/enums/magic_numbers.hpp>
#include <bitcoin/network/messages/get_blocks.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;
    
const std::string get_headers::command = "getheaders";
const identifier get_headers::id = identifier::get_headers;
const uint32_t get_headers::version_minimum = level::headers_protocol;
const uint32_t get_headers::version_maximum = level::maximum_protocol;

// static
size_t get_headers::locator_size(size_t top) NOEXCEPT
{
    return get_blocks::locator_size(top);
}

// static
get_headers::indexes get_headers::heights(size_t top) NOEXCEPT
{
    return get_blocks::heights(top);
}

// static
typename get_headers::cptr get_headers::deserialize(uint32_t version,
    const system::data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader));
    return reader ? message : nullptr;
}

// static
get_headers get_headers::deserialize(uint32_t version, reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    // Protocol version is stoopid (and unused).
    source.skip_bytes(sizeof(uint32_t));

    // Count of hashes is redundant with the message size.
    const auto count = source.read_size(max_get_headers);

    get_headers get;
    get.start_hashes.reserve(count);

    for (size_t hash = 0; hash < count; ++hash)
        get.start_hashes.push_back(source.read_hash());

    get.stop_hash = source.read_hash();
    return get;
}

bool get_headers::serialize(uint32_t version,
    const system::data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer);
    return writer;
}

void get_headers::serialize(uint32_t version, writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    // Write version vs. member protocol_version.
    ////sink.write_4_bytes_little_endian(protocol_version);
    sink.write_4_bytes_little_endian(version);

    // Count of hashes is redundant with the message size.
    sink.write_variable(start_hashes.size());

    for (const auto& start_hash : start_hashes)
        sink.write_bytes(start_hash);

    sink.write_bytes(stop_hash);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t get_headers::size(uint32_t) const NOEXCEPT
{
    return sizeof(uint32_t) +
        hash_size +
        variable_size(start_hashes.size()) +
            (hash_size * start_hashes.size());
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
