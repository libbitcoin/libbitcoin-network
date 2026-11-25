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
#include <bitcoin/network/messages/sv2/detail/set_new_prev_hash.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/sv2/types.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// static
typename set_new_prev_hash::cptr set_new_prev_hash::deserialize(
    const std::span<const uint8_t>& data) NOEXCEPT
{
    system::istream source{ { data.begin(), data.end() } };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(reader));
    return reader && reader.is_exhausted() ? message : nullptr;
}

// static
set_new_prev_hash set_new_prev_hash::deserialize(reader& source) NOEXCEPT
{
    return
    {
        source.read_4_bytes_little_endian(),
        source.read_4_bytes_little_endian(),
        source.read_hash(),
        source.read_4_bytes_little_endian(),
        source.read_4_bytes_little_endian()
    };
}

bool set_new_prev_hash::serialize(const data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(writer);
    return writer;
}

void set_new_prev_hash::serialize(writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size();)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_4_bytes_little_endian(channel_id);
    sink.write_4_bytes_little_endian(job_id);
    sink.write_bytes(prev_hash);
    sink.write_4_bytes_little_endian(ntime_start);
    sink.write_4_bytes_little_endian(nbits);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t set_new_prev_hash::size() const NOEXCEPT
{
    return sizeof(uint32_t) + sizeof(uint32_t) + system::hash_size +
        sizeof(uint32_t) + sizeof(uint32_t);
}

BC_POP_WARNING()

} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin
