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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_PING_31402_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_PING_31402_HPP

#include <memory>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_timer.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Ping-pong protocol.
/// Attach this to a channel immediately following handshake completion.
class BCT_API protocol_ping_31402
  : public protocol_timer, track<protocol_ping_31402>
{
public:
    typedef std::shared_ptr<protocol_ping_31402> ptr;

    protocol_ping_31402(const session& session, channel::ptr channel,
        const duration& heartbeat);

    virtual void start() override;

protected:
    // Expose polymorphic start method from base.
    using protocol_timer::start;

    virtual void send_ping(const code& ec);

    virtual void handle_receive_ping(const code& ec,
        messages::ping::ptr message);

    virtual const std::string& name() const override;
};

} // namespace network
} // namespace libbitcoin

#endif
