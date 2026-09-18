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
#ifndef LIBBITCOIN_NETWORK_INTERFACES_PEER_BROADCAST_HPP
#define LIBBITCOIN_NETWORK_INTERFACES_PEER_BROADCAST_HPP

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/interfaces/diagnostics.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

struct peer_broadcast
{
    /// Required for use of network::desubscriber.
    using key = uint64_t;

    /// Desubscriber requires bool handlers, injects `code` parameter.
    template <typename... Args>
    using subscriber = network::desubscriber<key, Args...>;

    /// dispatcher.subscribe(std::forward<signature>(handler));
    /// `key` parameter is passed by message issuer to self-identify.
    template <class Message>
    using signature = std::function<bool(const code&,
        const typename Message::cptr&, const key&)>;

    /// Messages relayed from the receiving channel to all other channels,
    /// and channel diagnostics, which are internal (never serialized).
    /// The v2 encoding is applied by the sender, so v1 is the relayed type.
    static constexpr std::tuple methods
    {
        method<"addr", messages::peer::address::cptr, key>{},
        method<"diagnostics", diagnostics::cptr, key>{}
    };
};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
