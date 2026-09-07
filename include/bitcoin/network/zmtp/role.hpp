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
#ifndef LIBBITCOIN_NETWORK_ZMTP_ROLE_HPP
#define LIBBITCOIN_NETWORK_ZMTP_ROLE_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace zmtp {

/// The ZeroMQ socket type of a server socket. The role fixes the wire grammar
/// of the connection: which peer socket types may connect, which frames the
/// peer may send and how they are read into rpc, and which rpc messages may
/// be written and how they are framed. It is a property of the channel type
/// that owns the socket, not of configuration.
enum class role : uint8_t
{
    /// Not a zmtp socket.
    undefined,

    /// PUB (XPUB): reads subscriptions, writes topic notifications.
    publisher,

    /// PULL: reads notifications, writes nothing.
    puller,

    /// REP: reads a request and writes its response, in strict alternation.
    replier,

    /// ROUTER: reads identified requests, writes identified responses and
    /// notifications (the rpc id is the peer identity).
    router
};

/// The Socket-Type property value advertised by the role.
constexpr std::string_view socket_type(role value) NOEXCEPT
{
    switch (value)
    {
        case role::publisher:
            return "PUB";
        case role::puller:
            return "PULL";
        case role::replier:
            return "REP";
        case role::router:
            return "ROUTER";
        default:
            return "";
    }
}

/// The peer Socket-Type property value may connect to the role (libzmq
/// socket type compatibility).
constexpr bool compatible(role value, std::string_view peer) NOEXCEPT
{
    switch (value)
    {
        case role::publisher:
            return peer == "SUB" || peer == "XSUB";
        case role::puller:
            return peer == "PUSH";
        case role::replier:
            return peer == "REQ" || peer == "DEALER";
        case role::router:
            return peer == "REQ" || peer == "DEALER" || peer == "ROUTER";
        default:
            return false;
    }
}

} // namespace zmtp
} // namespace network
} // namespace libbitcoin

#endif
