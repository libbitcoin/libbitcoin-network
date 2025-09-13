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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_MESSAGE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_MESSAGE_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/heading.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

/// Deserialize message payload from the client protocol encoding.
/// Returns nullptr if serialization fails for any reason (expected).
template <typename Message>
typename Message::cptr deserialize(const system::data_chunk& body) NOEXCEPT
{
    return Message::deserialize(body);
}

/// Serialize message object to the client protocol encoding.
/// Returns nullptr if serialization fails for any reason (unexpected).
template <typename Message>
system::chunk_ptr serialize(const Message& message) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const auto data = std::make_shared<system::data_chunk>(message.size());
    BC_POP_WARNING()

    if (!message.serialize(*data))
        return {};

    return data;
}

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
