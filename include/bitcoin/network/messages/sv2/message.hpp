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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_SV2_MESSAGE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_SV2_MESSAGE_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/sv2/heading.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {

/// Deserialize message payload from wire protocol encoding.
/// Returns nullptr if serialization fails for any reason (expected).
template <typename Message>
inline typename Message::cptr deserialize(
    const std::span<const uint8_t>& body) NOEXCEPT
{
    return Message::deserialize(body);
}

/// Serialize message object to wire protocol encoding,
/// into data sized to heading and payload.
/// Returns false if serialization fails for any reason (unexpected).
template <typename Message>
inline bool serialize(const Message& message,
    const system::data_slab& data) NOEXCEPT
{
    using namespace system;
    if (data.size() < heading::size())
        return false;

    const auto start = std::next(data.begin(), heading::size());
    const data_slab body(start, data.end());
    const auto size = body.size();
    return size <= heading::maximum_payload && message.serialize(body) &&
        heading::factory(Message::identifier, Message::channel, size)
            .serialize(data);
}

/// Serialize message object to wire protocol encoding.
/// Returns nullptr if serialization fails for any reason (unexpected).
template <typename Message>
inline system::chunk_ptr serialize(const Message& message) NOEXCEPT
{
    using namespace system;
    const auto size = ceilinged_add(heading::size(), message.size());
    const auto data = emplace_shared<data_chunk>(size);
    return serialize(message, data_slab{ *data }) ? data : chunk_ptr{};
}

} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
