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

#include <bitcoin/network/messages/get_client_filter_headers.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;
    
const std::string get_client_filter_headers::command = "getcfheaders";
const identifier get_client_filter_headers::id = identifier::get_client_filter_headers;
const uint32_t get_client_filter_headers::version_minimum = level::bip157;
const uint32_t get_client_filter_headers::version_maximum = level::maximum_protocol;

// static
size_t get_client_filter_headers::size(uint32_t) NOEXCEPT
{
    return sizeof(uint8_t)
        + sizeof(uint32_t)
        + hash_size;
}

// static
typename get_client_filter_headers::cptr
get_client_filter_headers::deserialize(uint32_t version,
    const system::data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader));
    return reader ? message : nullptr;
}

// static
get_client_filter_headers get_client_filter_headers::deserialize(
    uint32_t version, reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    return
    {
        source.read_byte(),
        source.read_4_bytes_little_endian(),
        source.read_hash()
    };
}

bool get_client_filter_headers::serialize(uint32_t version,
    const system::data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer);
    return writer;
}

void get_client_filter_headers::serialize(uint32_t BC_DEBUG_ONLY(version),
    writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_byte(filter_type);
    sink.write_4_bytes_little_endian(start_height);
    sink.write_bytes(stop_hash);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
