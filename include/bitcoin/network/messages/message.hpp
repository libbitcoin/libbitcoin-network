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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_MESSAGE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_MESSAGE_HPP

#include <iterator>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/heading.hpp>
#include <bitcoin/network/messages/transaction.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

inline uint32_t network_checksum(const system::hash_digest& hash) NOEXCEPT
{
    using namespace system;
    return from_little_endian(array_cast<uint8_t, sizeof(uint32_t)>(hash));
}

/// Deserialize message payload from the wire protocol encoding.
/// Returns nullptr if serialization fails for any reason (expected).
template <typename Message>
typename Message::cptr deserialize(const system::data_chunk& body,
    uint32_t version) NOEXCEPT
{
    return Message::deserialize(version, body);
}

/// Serialize message object to the wire protocol encoding.
/// Returns nullptr if serialization fails for any reason (unexpected).
template <typename Message>
system::chunk_ptr serialize(const Message& message, uint32_t magic,
    uint32_t version) NOEXCEPT
{
    using namespace system;
    const auto size = heading::size() + message.size(version);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const auto data = std::make_shared<data_chunk>(size);
    BC_POP_WARNING()
    const auto start = std::next(data->begin(), heading::size());
    const data_slab body(start, data->end());
    if (!message.serialize(version, body) ||
        !heading::factory(magic, Message::command, body).serialize(*data))
        return {};

    return data;
}

// Specialize for tx to eliminate hash computation on non-witness writes.
// Transaction is the only message that can reuse identity hash for heading.
template <>
system::chunk_ptr serialize<transaction>(const transaction& message,
    uint32_t magic, uint32_t version) NOEXCEPT;

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
