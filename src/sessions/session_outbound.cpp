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
#include <bitcoin/network/sessions/session_outbound.hpp>

#include <cstddef>
#include <functional>
#include <bitcoin/system.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_outbound

using namespace bc::system;
using namespace std::placeholders;

session_outbound::session_outbound(p2p& network)
  : session_batch(network)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

void session_outbound::start(result_handler handler)
{
    if (network_.network_settings().outbound_connections == 0)
    {
        handler(error::success);
        return;
    }

    session::start(BIND2(handle_started, _1, handler));
}

void session_outbound::handle_started(const code& ec,
    result_handler handler)
{
    if (ec)
    {
        handler(ec);
        return;
    }

    for (size_t peer = 0; peer < 
        network_.network_settings().outbound_connections; ++peer)
        new_connection(error::success);

    // This is the end of the start sequence.
    handler(error::success);
}

// Connnect cycle.
// ----------------------------------------------------------------------------

void session_outbound::new_connection(const code&)
{
    if (stopped())
        return;

    // CONNECT
    session_batch::connect(BIND2(handle_connect, _1, _2));
}

// THIS IS INVOKED ON THE CHANNEL THREAD.
void session_outbound::handle_connect(const code& ec,
    channel::ptr channel)
{
    if (ec)
    {
        ////// Retry with conditional delay in case of network error.
        ////dispatch_delayed(cycle_delay(ec), BIND1(new_connection, _1));
        return;
    }

    start_channel(channel,
        BIND2(handle_channel_start, _1, channel),
        BIND2(handle_channel_stop, _1, channel));
}

// THIS IS INVOKED ON THE CHANNEL THREAD.
void session_outbound::handle_channel_start(const code& ec,
    channel::ptr channel)
{
    // The start failure is also caught by handle_channel_stop.
    if (ec)
        return;

    attach_protocols(channel);
}

// Communication will begin after this function returns, freeing the thread.
void session_outbound::attach_protocols(channel::ptr channel)
{
    const auto version = channel->negotiated_version();
    const auto heartbeat = network_.network_settings().channel_heartbeat();

    if (version >= messages::level::bip31)
        attach<protocol_ping_60001>(channel, heartbeat)->start();
    else
        attach<protocol_ping_31402>(channel, heartbeat)->start();

    if (version >= messages::level::bip61)
        attach<protocol_reject_70002>(channel)->start();

    attach<protocol_address_31402>(channel, network_)->start();
}

void session_outbound::attach_handshake(channel::ptr channel,
    result_handler handle_started)
{
    const auto& settings = network_.network_settings();
    const auto relay = settings.relay_transactions;
    const auto own_version = settings.protocol_maximum;
    const auto own_services = settings.services;
    const auto invalid_services = settings.invalid_services;
    const auto minimum_version = settings.protocol_minimum;

    // Require peer to serve network (and witness if configured on self).
    const auto min_service = (own_services & messages::service::node_witness) |
        messages::service::node_network;

    // Reject messages are not handled until bip61 (70002).
    // The negotiated_version is initialized to the configured maximum.
    if (channel->negotiated_version() >= messages::level::bip61)
        attach<protocol_version_70002>(channel, network_, own_version,
            own_services, invalid_services, minimum_version, min_service, relay)
            ->start(handle_started);
    else
        attach<protocol_version_31402>(channel, network_, own_version,
            own_services, invalid_services, minimum_version, min_service)
            ->start(handle_started);
}

void session_outbound::handle_channel_stop(const code& ec,
    channel::ptr channel)
{
    new_connection(error::success);
}

} // namespace network
} // namespace libbitcoin
