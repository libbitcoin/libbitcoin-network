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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_PEER_MESSAGE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_PEER_MESSAGE_HPP

#include <span>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/peer/heading.hpp>
#include <bitcoin/network/messages/peer/detail/transaction.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

inline uint32_t network_checksum(const system::hash_digest& hash) NOEXCEPT
{
    using namespace system;
    return from_little_endian(array_cast<uint8_t, sizeof(uint32_t)>(hash));
}

/// Deserialize message payload from the wire protocol encoding.
/// Returns nullptr if serialization fails for any reason (expected).
template <typename Message>
inline typename Message::cptr deserialize(const std::span<const uint8_t>& body,
    uint32_t version) NOEXCEPT
{
    return Message::deserialize(version, body);
}

/// Serialize message payload to the wire protocol encoding (no heading).
/// Returns nullptr if serialization fails for any reason (unexpected).
template <typename Message>
inline system::chunk_ptr serialize(const Message& message,
    uint32_t version) NOEXCEPT
{
    using namespace system;
    const auto data = emplace_shared<data_chunk>(message.size(version));
    const data_slab body(data->begin(), data->end());
    return message.serialize(version, body) ? data : chunk_ptr{};
}

/// Serialize message object to the wire protocol encoding, into data sized to
/// the heading and payload.
/// Returns false if serialization fails for any reason (unexpected).
template <typename Message>
inline bool serialize(const Message& message, uint32_t magic,
    uint32_t version, const system::data_slab& data) NOEXCEPT
{
    using namespace system;
    const auto start = std::next(data.begin(), heading::size());
    const data_slab body(start, data.end());
    return message.serialize(version, body) &&
        heading::factory(magic, Message::command, body).serialize(data);
}

// Specialize for tx to eliminate hash computation on non-witness writes.
// Transaction is the only message that can reuse identity hash for heading.
template <>
BCT_API bool serialize<transaction>(const transaction& message,
    uint32_t magic, uint32_t version, const system::data_slab& data) NOEXCEPT;

/// Serialize message object to the wire protocol encoding.
/// Returns nullptr if serialization fails for any reason (unexpected).
template <typename Message>
inline system::chunk_ptr serialize(const Message& message, uint32_t magic,
    uint32_t version) NOEXCEPT
{
    using namespace system;
    const auto size = ceilinged_add(heading::size(), message.size(version));
    const auto data = emplace_shared<data_chunk>(size);
    return serialize(message, magic, version, *data) ? data : chunk_ptr{};
}

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
