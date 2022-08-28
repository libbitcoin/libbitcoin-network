/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/messages/heading.hpp>

#include <map>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/address.hpp>
#include <bitcoin/network/messages/alert.hpp>
#include <bitcoin/network/messages/block.hpp>
#include <bitcoin/network/messages/bloom_filter_add.hpp>
#include <bitcoin/network/messages/bloom_filter_clear.hpp>
#include <bitcoin/network/messages/bloom_filter_load.hpp>
#include <bitcoin/network/messages/client_filter.hpp>
#include <bitcoin/network/messages/client_filter_checkpoint.hpp>
#include <bitcoin/network/messages/client_filter_headers.hpp>
#include <bitcoin/network/messages/compact_block.hpp>
#include <bitcoin/network/messages/compact_transactions.hpp>
#include <bitcoin/network/messages/fee_filter.hpp>
#include <bitcoin/network/messages/get_address.hpp>
#include <bitcoin/network/messages/get_blocks.hpp>
#include <bitcoin/network/messages/get_client_filter_checkpoint.hpp>
#include <bitcoin/network/messages/get_client_filter_headers.hpp>
#include <bitcoin/network/messages/get_client_filters.hpp>
#include <bitcoin/network/messages/get_compact_transactions.hpp>
#include <bitcoin/network/messages/get_data.hpp>
#include <bitcoin/network/messages/get_headers.hpp>
#include <bitcoin/network/messages/headers.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/inventory.hpp>
#include <bitcoin/network/messages/message.hpp>
#include <bitcoin/network/messages/memory_pool.hpp>
#include <bitcoin/network/messages/merkle_block.hpp>
#include <bitcoin/network/messages/not_found.hpp>
#include <bitcoin/network/messages/ping.hpp>
#include <bitcoin/network/messages/pong.hpp>
#include <bitcoin/network/messages/reject.hpp>
#include <bitcoin/network/messages/send_compact.hpp>
#include <bitcoin/network/messages/send_headers.hpp>
#include <bitcoin/network/messages/transaction.hpp>
#include <bitcoin/network/messages/version_acknowledge.hpp>
#include <bitcoin/network/messages/version.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace bc::system;

// Pre-Witness:
// A maximal inventory is 50,000 entries, the largest valid message.
// Inventory with 50,000 entries is 3 + 36 * 50,000 bytes (1,800,003).
// The variable integer portion is maximum 3 bytes (with a count of 50,000).
// According to protocol documentation get_blocks is limited only by the
// general maximum payload size of 0x02000000 (33,554,432). But this is an
// absurd limit for a message that is properly [10 + log2(height) + 1]. Since
// protocol limits height to 2^32 this is 43. Even with expansion to 2^64
// this is limited to 75. So limit payloads to the max inventory payload size.
// Post-Witness:
// The maximum block size inclusive of witness is greater than 1,800,003, so
// with witness-enabled block size (4,000,000).
// This calculation should be revisited given any protocol change.
size_t heading::maximum_payload_size(uint32_t, bool witness) noexcept
{
    static constexpr size_t vector = sizeof(uint32_t) + hash_size;
    static constexpr size_t maximum = 3u + vector * max_inventory;
    return witness ? chain::max_block_weight : maximum;
}

// static
heading heading::factory(uint32_t magic, const std::string& command,
    const data_slice& payload) noexcept
{
    const auto size = payload.size();
    const auto payload_size = size > max_uint32 ? zero : size;
    const auto payload_trimmed = is_zero(payload_size) ? data_slice{} : payload;

    return
    {
        magic,
        command,
        possible_narrow_cast<uint32_t>(payload_size),
        network_checksum(payload_trimmed)
    };
}

// static
heading heading::deserialize(reader& source) noexcept
{
    return
    {
        source.read_4_bytes_little_endian(),
        source.read_string_buffer(command_size),
        source.read_4_bytes_little_endian(),
        source.read_4_bytes_little_endian()
    };
}

void heading::serialize(writer& sink) const noexcept
{
    sink.write_4_bytes_little_endian(magic);
    sink.write_string_buffer(command, command_size);
    sink.write_4_bytes_little_endian(payload_size);
    sink.write_4_bytes_little_endian(checksum);
}

#define COMMAND_ID(name) { name::command, name::id }

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
identifier heading::id() const noexcept
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

bool heading::verify_checksum(const data_slice& body) const noexcept
{
    return network_checksum(body) == checksum;
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
