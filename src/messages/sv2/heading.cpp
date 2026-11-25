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
#include <bitcoin/network/messages/sv2/heading.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/sv2/registry.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {

using namespace system;

// static
heading heading::factory(uint8_t identifier, bool channel,
    size_t payload_size) NOEXCEPT
{
    if (payload_size > maximum_payload)
        return {};

    return
    {
        channel ? channel_bit : uint16_t{},
        identifier,
        possible_narrow_cast<uint32_t>(payload_size)
    };
}

// static
heading::cptr heading::deserialize(
    const std::span<const uint8_t>& data) NOEXCEPT
{
    system::istream source{ { data.begin(), data.end() } };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(reader));
    return reader ? message : nullptr;
}

// static
heading heading::deserialize(reader& source) NOEXCEPT
{
    return
    {
        source.read_2_bytes_little_endian(),
        source.read_byte(),
        source.read_3_bytes_little_endian()
    };
}

bool heading::serialize(const data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(writer);
    return writer;
}

void heading::serialize(writer& sink) const NOEXCEPT
{
    sink.write_2_bytes_little_endian(extension_type);
    sink.write_byte(message_type);
    sink.write_3_bytes_little_endian(message_length);
}

bool heading::channel() const NOEXCEPT
{
    return !is_zero(bit_and(extension_type, channel_bit));
}

uint16_t heading::extension() const NOEXCEPT
{
    return bit_and(extension_type, bit_not(channel_bit));
}

size_t heading::index() const NOEXCEPT
{
    return is_zero(extension()) ? registry::index(message_type) :
        registry::unknown;
}

} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin
