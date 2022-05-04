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

session_inbound::session_inbound(p2p& network) noexcept
  : session(network),
    connection_limit_(network.network_settings().inbound_connections)
{
}

bool session_inbound::inbound() const noexcept
{
    return true;
}

bool session_inbound::notify() const noexcept
{
    return true;
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_inbound::start(result_handler handler) noexcept
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
    result_handler handler) noexcept
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

    start_accept(code);
}

void session_inbound::stop() noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (acceptor_)
    {
        acceptor_->stop();
        acceptor_.reset();
    }

    session::stop();
}

// Accept sequence.
// ----------------------------------------------------------------------------

void session_inbound::start_accept(const code& ec) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
        return;

    // May be acceptor start failure or cancelation from timer, terminal.
    if (ec)
        return;

    // ACCEPT (wait)
    acceptor_->accept(BIND2(handle_accept, _1, _2));
}

void session_inbound::handle_accept(const code& ec,
    channel::ptr channel) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Timer may start up again after service stop, so check first.
    if (stopped())
    {
        if (channel)
            channel->stop(error::service_stopped);

        return;
    }

    // There was an error accepting the channel, so try again after delay.
    if (ec)
    {
        BC_ASSERT_MSG(!channel, "unexpected channel instance");
        timer_->start(BIND1(start_accept, _1), settings().connect_timeout());
        return;
    }

    // There was no error, so listen again.
    start_accept(error::success);

    // Could instead stop listening when at limit, though that requires event.
    if (inbound_channel_count() >= settings().inbound_connections)
    {
        channel->stop(error::oversubscribed);
        return;
    }

    if (blacklisted(channel->authority()))
    {
        channel->stop(error::address_blocked);
        return;
    }

    start_channel(channel,
        BIND2(handle_channel_start, _1, channel),
        BIND2(handle_channel_stop, _1, channel));
}

void session_inbound::attach_handshake(const channel::ptr& channel,
    result_handler handler) const noexcept
{
    session::attach_handshake(channel, handler);
}

void session_inbound::handle_channel_start(const code&, channel::ptr) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
}

void session_inbound::attach_protocols(
    const channel::ptr& channel) const noexcept
{
    BC_ASSERT_MSG(channel->stranded(), "strand");

    const auto version = channel->negotiated_version();
    const auto heartbeat = settings().channel_heartbeat();

    if (version >= messages::level::bip31)
        channel->do_attach<protocol_ping_60001>(*this, heartbeat)->start();
    else
        channel->do_attach<protocol_ping_31402>(*this, heartbeat)->start();

    if (version >= messages::level::bip61)
        channel->do_attach<protocol_reject_70002>(*this)->start();

    channel->do_attach<protocol_address_31402>(*this)->start();
}

void session_inbound::handle_channel_stop(const code&, channel::ptr) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
}

} // namespace network
} // namespace libbitcoin
