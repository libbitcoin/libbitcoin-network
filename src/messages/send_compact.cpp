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
#include <bitcoin/network/messages/send_compact.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;
    
const std::string send_compact::command = "sendcmpct";
const identifier send_compact::id = identifier::send_compact;
const uint32_t send_compact::version_minimum = level::bip152;
const uint32_t send_compact::version_maximum = level::maximum_protocol;

// static
size_t send_compact::size(uint32_t) NOEXCEPT
{
    return sizeof(uint8_t)
        + sizeof(uint64_t);
}

// static
typename send_compact::cptr send_compact::deserialize(uint32_t version,
    const system::data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader));
    return reader ? message : nullptr;
}

// static
send_compact send_compact::deserialize(uint32_t version,
    reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    const auto mode = source.read_byte();
    const auto protocol = source.read_8_bytes_little_endian();

    // bip152: high_bandwidth value is boolean and must be zero or one.
    if (mode > one)
        source.invalidate();

    // bip152: protocol version must "currently" be one (bad specification).
    // Peers of negotiated protocol 70015 are setting this to two.
    ////if (!is_one(protocol))
    ////    source.invalidate();

    return { to_bool(mode), protocol };
}

bool send_compact::serialize(uint32_t version,
    const system::data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer);
    return writer;
}

void send_compact::serialize(uint32_t BC_DEBUG_ONLY(version),
    writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_byte(static_cast<uint8_t>(high_bandwidth));
    sink.write_8_bytes_little_endian(compact_version);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
