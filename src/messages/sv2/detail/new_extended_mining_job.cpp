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
#include <bitcoin/network/messages/sv2/detail/new_extended_mining_job.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/sv2/types.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// static
typename new_extended_mining_job::cptr new_extended_mining_job::deserialize(
    const std::span<const uint8_t>& data) NOEXCEPT
{
    system::istream source{ { data.begin(), data.end() } };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(reader));
    return reader && reader.is_exhausted() ? message : nullptr;
}

// static
new_extended_mining_job new_extended_mining_job::deserialize(
    reader& source) NOEXCEPT
{
    return
    {
        source.read_4_bytes_little_endian(),
        source.read_4_bytes_little_endian(),
        read_option_u32(source),
        source.read_4_bytes_little_endian(),
        read_bool(source),
        read_seq0_255_u256(source),
        read_b0_64k(source),
        read_b0_64k(source)
    };
}

bool new_extended_mining_job::serialize(const data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(writer);
    return writer;
}

void new_extended_mining_job::serialize(writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size();)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_4_bytes_little_endian(channel_id);
    sink.write_4_bytes_little_endian(job_id);
    write_option_u32(sink, ntime_start);
    sink.write_4_bytes_little_endian(version);
    write_bool(sink, version_rolling_allowed);
    write_seq0_255_u256(sink, merkle_path);
    write_b0_64k(sink, coinbase_tx_prefix);
    write_b0_64k(sink, coinbase_tx_suffix);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t new_extended_mining_job::size() const NOEXCEPT
{
    return sizeof(uint32_t) + sizeof(uint32_t) + size_option_u32(ntime_start) +
        sizeof(uint32_t) + sizeof(uint8_t) + size_seq0_255_u256(merkle_path) +
        size_b0_64k(coinbase_tx_prefix) + size_b0_64k(coinbase_tx_suffix);
}

BC_POP_WARNING()

} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin
