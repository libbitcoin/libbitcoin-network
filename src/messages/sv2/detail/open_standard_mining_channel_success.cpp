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
#include <bitcoin/network/messages/sv2/detail/open_standard_mining_channel_success.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/sv2/types.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// static
typename open_standard_mining_channel_success::cptr
open_standard_mining_channel_success::deserialize(
    const std::span<const uint8_t>& data) NOEXCEPT
{
    system::istream source{ { data.begin(), data.end() } };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(reader));
    return reader && reader.is_exhausted() ? message : nullptr;
}

// static
open_standard_mining_channel_success
open_standard_mining_channel_success::deserialize(reader& source) NOEXCEPT
{
    return
    {
        source.read_4_bytes_little_endian(),
        source.read_4_bytes_little_endian(),
        source.read_hash(),
        read_b0_32(source),
        source.read_4_bytes_little_endian()
    };
}

bool
open_standard_mining_channel_success::serialize(
    const data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(writer);
    return writer;
}

void
open_standard_mining_channel_success::serialize(writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size();)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_4_bytes_little_endian(request_id);
    sink.write_4_bytes_little_endian(channel_id);
    sink.write_bytes(target);
    write_b0_32(sink, extranonce_prefix);
    sink.write_4_bytes_little_endian(group_channel_id);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t
open_standard_mining_channel_success::size() const NOEXCEPT
{
    return sizeof(uint32_t) + sizeof(uint32_t) + system::hash_size +
        size_b0_255(extranonce_prefix) + sizeof(uint32_t);
}

BC_POP_WARNING()

} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin
