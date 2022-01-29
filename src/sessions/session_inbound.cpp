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
#include <bitcoin/network/sessions/session_inbound.hpp>

#include <cstddef>
#include <functional>
#include <bitcoin/system.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_inbound

using namespace bc::system;
using namespace std::placeholders;

session_inbound::session_inbound(p2p& network)
  : session(network),
    connection_limit_(
        network.network_settings().inbound_connections +
        network.network_settings().outbound_connections +
        network.network_settings().peers.size())
{
}

bool session_inbound::inbound() const
{
    return true;
}

// Start sequence.
// ----------------------------------------------------------------------------

void session_inbound::start(result_handler handler)
{
    if (network_.network_settings().inbound_port == 0 ||
        network_.network_settings().inbound_connections == 0)
    {
        handler(error::success);
        return;
    }

    session::start(BIND2(handle_started, _1, handler));
}

void session_inbound::handle_started(const code& ec,
    result_handler handler)
{
    if (ec)
    {
        handler(ec);
        return;
    }

    acceptor_ = create_acceptor();

    // LISTEN
    const auto start_ec = acceptor_->start(
        network_.network_settings().inbound_port);

    if (start_ec)
    {
        handler(start_ec);
        return;
    }

    start_accept(error::success);

    // This is the end of the start sequence.
    handler(error::success);
}

////void session_inbound::stop(const code& ec)
////{
////    acceptor_->stop(ec);
////}

// Accept sequence.
// ----------------------------------------------------------------------------

void session_inbound::start_accept(const code&)
{
    if (stopped())
        return;

    // ACCEPT
    acceptor_->accept(BIND2(handle_accept, _1, _2));
}

void session_inbound::handle_accept(const code& ec,
    channel::ptr channel)
{
    if (stopped(ec))
        return;

    ////// Start accepting with conditional delay in case of network error.
    ////dispatch_delayed(cycle_delay(ec), BIND1(start_accept, _1));

    if (ec)
        return;

    if (blacklisted(channel->authority()))
        return;

    if (network_.channel_count() >= connection_limit_)
        return;

    start_channel(channel,
        BIND2(handle_channel_start, _1, channel),
        BIND1(handle_channel_stop, _1));
}

void session_inbound::handle_channel_start(const code& ec,
    channel::ptr channel)
{
    if (ec)
        return;

    attach_protocols(channel);
}

void session_inbound::attach_protocols(channel::ptr channel)
{
    const auto version = channel->negotiated_version();
    const auto heartbeat = network_.network_settings().channel_heartbeat();

    if (version >= messages::level::bip31)
        channel->attach<protocol_ping_60001>(heartbeat)->start();
    else
        channel->attach<protocol_ping_31402>(heartbeat)->start();

    if (version >= messages::level::bip61)
        channel->attach<protocol_reject_70002>()->start();

    channel->attach<protocol_address_31402>(network_)->start();
}

void session_inbound::handle_channel_stop(const code&)
{
}

} // namespace network
} // namespace libbitcoin
