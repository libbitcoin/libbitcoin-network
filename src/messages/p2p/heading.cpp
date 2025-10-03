/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/messages/p2p/heading.hpp>

#include <map>
#include <bitcoin/network/messages/p2p/address.hpp>
#include <bitcoin/network/messages/p2p/alert.hpp>
#include <bitcoin/network/messages/p2p/block.hpp>
#include <bitcoin/network/messages/p2p/bloom_filter_add.hpp>
#include <bitcoin/network/messages/p2p/bloom_filter_clear.hpp>
#include <bitcoin/network/messages/p2p/bloom_filter_load.hpp>
#include <bitcoin/network/messages/p2p/client_filter.hpp>
#include <bitcoin/network/messages/p2p/client_filter_checkpoint.hpp>
#include <bitcoin/network/messages/p2p/client_filter_headers.hpp>
#include <bitcoin/network/messages/p2p/compact_block.hpp>
#include <bitcoin/network/messages/p2p/compact_transactions.hpp>
#include <bitcoin/network/messages/p2p/fee_filter.hpp>
#include <bitcoin/network/messages/p2p/get_address.hpp>
#include <bitcoin/network/messages/p2p/get_blocks.hpp>
#include <bitcoin/network/messages/p2p/get_client_filter_checkpoint.hpp>
#include <bitcoin/network/messages/p2p/get_client_filter_headers.hpp>
#include <bitcoin/network/messages/p2p/get_client_filters.hpp>
#include <bitcoin/network/messages/p2p/get_compact_transactions.hpp>
#include <bitcoin/network/messages/p2p/get_data.hpp>
#include <bitcoin/network/messages/p2p/get_headers.hpp>
#include <bitcoin/network/messages/p2p/headers.hpp>
#include <bitcoin/network/messages/p2p/enums/identifier.hpp>
#include <bitcoin/network/messages/p2p/inventory.hpp>
#include <bitcoin/network/messages/p2p/message.hpp>
#include <bitcoin/network/messages/p2p/memory_pool.hpp>
#include <bitcoin/network/messages/p2p/merkle_block.hpp>
#include <bitcoin/network/messages/p2p/not_found.hpp>
#include <bitcoin/network/messages/p2p/ping.hpp>
#include <bitcoin/network/messages/p2p/pong.hpp>
#include <bitcoin/network/messages/p2p/reject.hpp>
#include <bitcoin/network/messages/p2p/send_address_v2.hpp>
#include <bitcoin/network/messages/p2p/send_compact.hpp>
#include <bitcoin/network/messages/p2p/send_headers.hpp>
#include <bitcoin/network/messages/p2p/transaction.hpp>
#include <bitcoin/network/messages/p2p/version.hpp>
#include <bitcoin/network/messages/p2p/version_acknowledge.hpp>
#include <bitcoin/network/messages/p2p/witness_tx_id_relay.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace p2p {

using namespace system;

// maximum_payload:
//
// Pre-Witness:
// A maximal inventory is 50,000 entries, the largest valid message.
// Inventory with 50,000 entries is 3 + 36 * 50,000 bytes (1,800,003).
// The variable integer portion is maximum 3 bytes (with a count of 50,000).
// According to protocol documentation get_blocks is limited only by the
// general maximum payload size of 0x02000000 (33,554,432). But this is an
// absurd limit for message that is properly [10 + log2(height) + 1]. Since
// protocol limits height to 2^32 this is 43. Even with expansion to 2^64
// this is limited to 75. So limit payloads to max inventory payload size.
//
// Post-Witness:
// The maximum block size inclusive of witness is greater than 1,800,003, so
// with witness-enabled block size (4,000,000).
// This calculation should be revisited given any protocol change.

// static
// Logging utility only.
std::string heading::get_command(const data_chunk& payload) NOEXCEPT
{
    if (payload.size() < sizeof(uint32_t) + command_size)
        return "<unknown>";

    std::string out{};
    auto at = std::next(payload.begin(), sizeof(uint32_t));
    const auto end = std::next(at, command_size);
    while (at != end && *at != 0x00)
        out.push_back(*at++);

    return out;
}

// static
heading heading::factory(uint32_t magic, const std::string& command,
    const data_slice& payload) NOEXCEPT
{
    const auto size = payload.size();
    return factory(magic, command, size, bitcoin_hash(size, payload.data()));
}

// static
heading heading::factory(uint32_t magic, const std::string& command,
    size_t payload_size, const hash_digest& payload_hash) NOEXCEPT
{
    if (is_limited<uint32_t>(payload_size))
        return {};

    return
    {
        magic,
        command,
        possible_narrow_cast<uint32_t>(payload_size),
        network_checksum(payload_hash)
    };
}

// static
heading::cptr heading::deserialize(const data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(reader));
    return reader ? message : nullptr;
}

// static
heading heading::deserialize(reader& source) NOEXCEPT
{
    return
    {
        source.read_4_bytes_little_endian(),
        source.read_string_buffer(command_size),
        source.read_4_bytes_little_endian(),
        source.read_4_bytes_little_endian()
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
    sink.write_4_bytes_little_endian(magic);
    sink.write_string_buffer(command, command_size);
    sink.write_4_bytes_little_endian(payload_size);
    sink.write_4_bytes_little_endian(checksum);
}

#define COMMAND_ID(name) { name::command, name::id }

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
identifier heading::id() const NOEXCEPT
{
    // Internal to function avoids static initialization race.
    static const std::map<std::string, identifier> identifiers
    {
        COMMAND_ID(address),
        COMMAND_ID(alert),
        COMMAND_ID(block),
        COMMAND_ID(bloom_filter_add),
        COMMAND_ID(bloom_filter_clear),
        COMMAND_ID(bloom_filter_load),
        COMMAND_ID(client_filter),
        COMMAND_ID(client_filter_checkpoint),
        COMMAND_ID(client_filter_headers),
        COMMAND_ID(compact_block),
        COMMAND_ID(compact_transactions),
        COMMAND_ID(fee_filter),
        COMMAND_ID(get_address),
        COMMAND_ID(get_blocks),
        COMMAND_ID(get_client_filter_checkpoint),
        COMMAND_ID(get_client_filter_headers),
        COMMAND_ID(get_client_filters),
        COMMAND_ID(get_compact_transactions),
        COMMAND_ID(get_data),
        COMMAND_ID(get_headers),
        COMMAND_ID(headers),
        COMMAND_ID(inventory),
        COMMAND_ID(memory_pool),
        COMMAND_ID(merkle_block),
        COMMAND_ID(not_found),
        COMMAND_ID(ping),
        COMMAND_ID(pong),
        COMMAND_ID(reject),
        COMMAND_ID(send_address_v2),
        COMMAND_ID(send_compact),
        COMMAND_ID(send_headers),
        COMMAND_ID(transaction),
        COMMAND_ID(version),
        COMMAND_ID(version_acknowledge)
    };

    const auto it = identifiers.find(command);
    return (it == identifiers.end() ? identifier::unknown : it->second);
}
BC_POP_WARNING()

#undef COMMAND_ID

} // namespace p2p
} // namespace messages
} // namespace network
} // namespace libbitcoin
