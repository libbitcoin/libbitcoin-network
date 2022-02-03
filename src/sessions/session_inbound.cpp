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

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_inbound::start(result_handler handler)
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (is_zero(settings().inbound_port) ||
        is_zero(settings().inbound_connections))
    {
        handler(error::success);
        return;
    }

    session::start(BIND2(handle_started, _1, handler));
}

void session_inbound::handle_started(const code& ec,
    result_handler handler)
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        handler(ec);
        return;
    }

    acceptor_ = create_acceptor();
    const auto code = acceptor_->start(settings().inbound_port);
    handler(code);

    // LISTEN
    start_accept(code);
}

////void session_inbound::stop()
////{
////    acceptor_->stop();
////}

// Accept sequence.
// ----------------------------------------------------------------------------

void session_inbound::start_accept(const code& ec)
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
        return;

    // ACCEPT (wait)
    acceptor_->accept(BIND2(handle_accept, _1, _2));
}

void session_inbound::handle_accept(const code& ec, channel::ptr channel)
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped(ec))
        return;

    // TODO: use timer to delay start in case of error other than
    // channel_timeout. use network_.network_settings().connect_timeout().
    start_accept(error::success);

    // There was an error accepting the channel, so drop it.
    if (ec)
        return;

    if (channel_count() >= connection_limit_)
        return;

    if (blacklisted(channel->authority()))
        return;

    start_channel(channel,
        BIND2(handle_channel_start, _1, channel),
        BIND1(handle_channel_stop, _1));
}

void session_inbound::handle_channel_start(const code& ec,
    channel::ptr channel)
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
        return;

    // Calls attach_protocols on channel strand.
    post_attach_protocols(channel);
}

void session_inbound::attach_protocols(channel::ptr channel,
    result_handler) const
{
    BC_ASSERT_MSG(stranded(), "strand");

    const auto version = channel->negotiated_version();
    const auto heartbeat = settings().channel_heartbeat();

    // TODO: pass session to base protocol construct (derive settings as required).
    if (version >= messages::level::bip31)
        channel->do_attach<protocol_ping_60001>(*this, heartbeat)->start();
    else
        channel->do_attach<protocol_ping_31402>(*this, heartbeat)->start();

    if (version >= messages::level::bip61)
        channel->do_attach<protocol_reject_70002>(*this)->start();

    channel->do_attach<protocol_address_31402>(*this)->start();
}

void session_inbound::handle_channel_stop(const code&)
{
    BC_ASSERT_MSG(stranded(), "strand");
}

} // namespace network
} // namespace libbitcoin
