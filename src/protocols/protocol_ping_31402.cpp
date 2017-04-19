/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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

#include <functional>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocol_timer.hpp>

namespace libbitcoin {
namespace network {

#define NAME "ping"
#define CLASS protocol_ping_31402

using namespace bc::message;
using namespace std::placeholders;

protocol_ping_31402::protocol_ping_31402(p2p& network, channel::ptr channel)
  : protocol_timer(network, channel, true, NAME),
    settings_(network.network_settings()),
    CONSTRUCT_TRACK(protocol_ping_31402)
{
}

void protocol_ping_31402::start()
{
    protocol_timer::start(settings_.channel_heartbeat(), BIND1(send_ping, _1));

    SUBSCRIBE2(ping, handle_receive_ping, _1, _2);

    // Send initial ping message by simulating first heartbeat.
    set_event(error::success);
}

// This is fired by the callback (i.e. base timer and stop handler).
void protocol_ping_31402::send_ping(const code& ec)
{
    if (stopped(ec))
        return;

    if (ec && ec != error::channel_timeout)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure in ping timer for [" << authority() << "] "
            << ec.message();
        stop(ec);
        return;
    }

    SEND2(ping{}, handle_send, _1, ping::command);
}

bool protocol_ping_31402::handle_receive_ping(const code& ec,
    ping_const_ptr message)
{
    if (stopped(ec))
        return false;

    // RESUBSCRIBE
    return true;
}

} // namespace network
} // namespace libbitcoin
