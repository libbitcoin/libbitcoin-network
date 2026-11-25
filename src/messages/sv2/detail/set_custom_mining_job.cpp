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
#include <bitcoin/network/messages/sv2/detail/set_custom_mining_job.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/sv2/types.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// static
typename set_custom_mining_job::cptr set_custom_mining_job::deserialize(
    const std::span<const uint8_t>& data) NOEXCEPT
{
    system::istream source{ { data.begin(), data.end() } };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(reader));
    return reader && reader.is_exhausted() ? message : nullptr;
}

// static
set_custom_mining_job set_custom_mining_job::deserialize(
    reader& source) NOEXCEPT
{
    return
    {
        source.read_4_bytes_little_endian(),
        source.read_4_bytes_little_endian(),
        read_b0_255(source),
        source.read_4_bytes_little_endian(),
        source.read_hash(),
        source.read_4_bytes_little_endian(),
        source.read_4_bytes_little_endian(),
        source.read_4_bytes_little_endian(),
        read_b0_255(source),
        source.read_4_bytes_little_endian(),
        read_b0_64k(source),
        source.read_4_bytes_little_endian(),
        read_seq0_255_u256(source)
    };
}

bool set_custom_mining_job::serialize(const data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(writer);
    return writer;
}

void set_custom_mining_job::serialize(writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size();)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_4_bytes_little_endian(channel_id);
    sink.write_4_bytes_little_endian(request_id);
    write_b0_255(sink, mining_job_token);
    sink.write_4_bytes_little_endian(version);
    sink.write_bytes(prev_hash);
    sink.write_4_bytes_little_endian(ntime_start);
    sink.write_4_bytes_little_endian(nbits);
    sink.write_4_bytes_little_endian(coinbase_tx_version);
    write_b0_255(sink, coinbase_prefix);
    sink.write_4_bytes_little_endian(coinbase_tx_input_sequence);
    write_b0_64k(sink, coinbase_tx_outputs);
    sink.write_4_bytes_little_endian(coinbase_tx_locktime);
    write_seq0_255_u256(sink, merkle_path);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t set_custom_mining_job::size() const NOEXCEPT
{
    return sizeof(uint32_t) + sizeof(uint32_t) +
        size_b0_255(mining_job_token) + sizeof(uint32_t) + system::hash_size +
        sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) +
        size_b0_255(coinbase_prefix) + sizeof(uint32_t) +
        size_b0_64k(coinbase_tx_outputs) + sizeof(uint32_t) +
        size_seq0_255_u256(merkle_path);
}

BC_POP_WARNING()

} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin
