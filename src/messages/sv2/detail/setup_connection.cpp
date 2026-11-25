/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#include <bitcoin/network/messages/sv2/detail/setup_connection.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/sv2/types.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// static
typename setup_connection::cptr setup_connection::deserialize(
    const std::span<const uint8_t>& data) NOEXCEPT
{
    system::istream source{ { data.begin(), data.end() } };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(reader));
    return reader && reader.is_exhausted() ? message : nullptr;
}

// static
setup_connection setup_connection::deserialize(reader& source) NOEXCEPT
{
    return
    {
        source.read_byte(),
        source.read_2_bytes_little_endian(),
        source.read_2_bytes_little_endian(),
        source.read_4_bytes_little_endian(),
        read_str0_255(source),
        source.read_2_bytes_little_endian(),
        read_str0_255(source),
        read_str0_255(source),
        read_str0_255(source),
        read_str0_255(source)
    };
}

bool setup_connection::serialize(const data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(writer);
    return writer;
}

void setup_connection::serialize(writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size();)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_byte(protocol);
    sink.write_2_bytes_little_endian(min_version);
    sink.write_2_bytes_little_endian(max_version);
    sink.write_4_bytes_little_endian(flags);
    write_str0_255(sink, endpoint_host);
    sink.write_2_bytes_little_endian(endpoint_port);
    write_str0_255(sink, vendor);
    write_str0_255(sink, hardware_version);
    write_str0_255(sink, firmware);
    write_str0_255(sink, device_id);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t setup_connection::size() const NOEXCEPT
{
    return sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t) +
        sizeof(uint32_t) + size_str0_255(endpoint_host) + sizeof(uint16_t) +
        size_str0_255(vendor) + size_str0_255(hardware_version) +
        size_str0_255(firmware) + size_str0_255(device_id);
}

BC_POP_WARNING()

} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin
