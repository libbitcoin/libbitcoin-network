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
#include <bitcoin/network/protocols/protocol_ping_31402.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_ping_31402

using namespace system;
using namespace messages;
using namespace std::placeholders;

protocol_ping_31402::protocol_ping_31402(const session::ptr& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    timer_(std::make_shared<deadline>(session->log, channel->strand(),
        session->settings().channel_heartbeat())),
    tracker<protocol_ping_31402>(session->log)
{
}

// Also invoked by protocol_ping_31402.
void protocol_ping_31402::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_31402");

    if (started())
        return;

    SUBSCRIBE_CHANNEL(ping, handle_receive_ping, _1, _2);
    send_ping();

    protocol::start();
}

// Also invoked by protocol_ping_31402.
void protocol_ping_31402::stopping(const code&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_31402");

    timer_->stop();
}

// Outgoing (send_ping [on timer] => handle_send).
// ----------------------------------------------------------------------------

void protocol_ping_31402::send_ping() NOEXCEPT
{
    SEND(ping{}, handle_send_ping, _1);
}

// Also invoked by protocol_ping_31402.
void protocol_ping_31402::handle_send_ping(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_31402");

    if (stopped(ec))
        return;

    timer_->start(BIND(handle_timer, _1));
    protocol::handle_send(ec);
}

void protocol_ping_31402::handle_timer(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_31402");

    if (stopped())
        return;

    // error::operation_canceled implies stopped, so this is something else.
    if (ec)
    {
        // TODO: log code.
        stop(ec);
        return;
    }

    // No error code on timeout, time to send another ping.
    send_ping();
}

// Incoming (receive_ping, the incoming traffic resets channel activity timer).
// ----------------------------------------------------------------------------

bool protocol_ping_31402::handle_receive_ping(const code&,
    const ping::cptr&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_31402");
    return !stopped();
}

} // namespace network
} // namespace libbitcoin
