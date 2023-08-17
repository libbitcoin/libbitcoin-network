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
#include <bitcoin/network/messages/reject.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/block.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/enums/magic_numbers.hpp>
#include <bitcoin/network/messages/message.hpp>
#include <bitcoin/network/messages/transaction.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;
    
const std::string reject::command = "reject";
const identifier reject::id = identifier::reject;
const uint32_t reject::version_minimum = level::bip61;
const uint32_t reject::version_maximum = level::maximum_protocol;

// static
bool reject::is_chain(const std::string& message) NOEXCEPT
{
    return (message == block::command) || (message == transaction::command);
}

// static
uint8_t reject::reason_to_byte(reason_code value) NOEXCEPT
{
    return static_cast<uint8_t>(value);
}

// static
reject::reason_code reject::byte_to_reason(uint8_t byte) NOEXCEPT
{
    switch (byte)
    {
        case 0x01:
            return reason_code::malformed;
        case 0x10:
            return reason_code::invalid;
        case 0x11:
            return reason_code::obsolete;
        case 0x12:
            return reason_code::duplicate;
        case 0x40:
            return reason_code::nonstandard;
        case 0x41:
            return reason_code::dust;
        case 0x42:
            return reason_code::insufficient_fee;
        case 0x43:
            return reason_code::checkpoint;
        default:
            return reason_code::undefined;
    }
}

// static
typename reject::cptr reject::deserialize(uint32_t version,
    const system::data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader));
    return reader ? message : nullptr;
}

// static
reject reject::deserialize(uint32_t, reader& source) NOEXCEPT
{
    auto message = source.read_string(max_reject_message);
    const auto chain = is_chain(message);

    return
    {
        std::move(message),
        byte_to_reason(source.read_byte()),
        source.read_string(max_reject_message),

        // Some nodes do not follow the documented convention of supplying hash
        // for tx and block rejects. Use this to prevent error by ensuring only
        // and all provided bytes are read. to_array will pad/truncate.
        chain ? to_array<hash_size>(source.read_bytes()) : null_hash
    };
}

bool reject::serialize(uint32_t version,
    const system::data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer);
    return writer;
}

void reject::serialize(uint32_t BC_DEBUG_ONLY(version),
    writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_string(message);
    sink.write_byte(reason_to_byte(code));
    sink.write_string(reason);

    if (is_chain(message))
        sink.write_bytes(hash);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t reject::size(uint32_t) const NOEXCEPT
{
    return variable_size(message.length()) + message.length()
        + sizeof(uint8_t)
        + variable_size(reason.length()) + reason.length()
        + (is_chain(message) ? hash_size : zero);
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
