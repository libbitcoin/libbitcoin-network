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
#include <bitcoin/network/protocols/protocol_ping_31402.hpp>

#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_ping_31402
static const std::string protocol_name = "ping";

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

protocol_ping_31402::protocol_ping_31402(const session& session,
    const channel::ptr& channel)
  : protocol(session, channel),
    timer_(std::make_shared<deadline>(channel->strand(),
        session.settings().channel_heartbeat()))
{
}

const std::string& protocol_ping_31402::name() const
{
    return protocol_name;
}

// Also invoked by protocol_ping_31402.
void protocol_ping_31402::start()
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_31402");

    if (started())
        return;

    SUBSCRIBE2(ping, handle_receive_ping, _1, _2);
    send_ping();

    protocol::start();
}

// Also invoked by protocol_ping_31402.
void protocol_ping_31402::stopping(const code&)
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_31402");

    timer_->stop();
}

// Outgoing (send_ping [on timer] => handle_send).
// ----------------------------------------------------------------------------

void protocol_ping_31402::send_ping()
{
    SEND1(ping{}, handle_send_ping, _1);
}

// Also invoked by protocol_ping_31402.
void protocol_ping_31402::handle_send_ping(const code& ec)
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_31402");

    if (stopped(ec))
        return;

    timer_->start(BIND1(handle_timer, _1));
    protocol::handle_send(ec);
}

void protocol_ping_31402::handle_timer(const code& ec)
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

void protocol_ping_31402::handle_receive_ping(const code& ec, const ping::ptr&)
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_31402");
}

} // namespace network
} // namespace libbitcoin
