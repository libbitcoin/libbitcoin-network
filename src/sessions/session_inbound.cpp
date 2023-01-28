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
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_inbound

using namespace bc::system;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

session_inbound::session_inbound(p2p& network) NOEXCEPT
  : session(network), tracker<session_inbound>(network.log())
{
}

bool session_inbound::inbound() const NOEXCEPT
{
    return true;
}

bool session_inbound::notify() const NOEXCEPT
{
    return true;
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_inbound::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (is_zero(settings().inbound_port) ||
        is_zero(settings().inbound_connections))
    {
        LOG("Not configured for inbound connections.");
        handler(error::bypassed);
        return;
    }

    session::start(BIND2(handle_started, _1, std::move(handler)));
}

void session_inbound::handle_started(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(!stopped(), "session stopped in start");

    if (ec)
    {
        handler(ec);
        return;
    }

    // Create only one acceptor.
    const auto acceptor = create_acceptor();
    const auto error_code = acceptor->start(settings().inbound_port);
    handler(error_code);

    if (!error_code)
    {
        subscribe_stop([=](const network::code&) NOEXCEPT
        {
            acceptor->stop();
        });

        start_accept(error::success, acceptor);
    }
}

// Accept cycle.
// ----------------------------------------------------------------------------

void session_inbound::start_accept(const code& ec,
    const acceptor::ptr& acceptor) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Terminates accept loop (and acceptor is restartable).
    if (stopped())
        return;

    if (ec)
    {
        LOG("Failed to start acceptor, " << ec.message());
        return;
    }

    acceptor->accept(BIND3(handle_accept, _1, _2, acceptor));
}

void session_inbound::handle_accept(const code& ec,
    const channel::ptr& channel, const acceptor::ptr& acceptor) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Guard restartable timer (shutdown delay).
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
        LOG("Failed to accept inbound channel, " << ec.message());

        start_timer(BIND2(start_accept, _1, acceptor),
            settings().connect_timeout());
        return;
    }

    // There was no error, so listen again without delay.
    start_accept(error::success, acceptor);

    // Could instead stop listening when at limit, though this is simpler.
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

// Completion sequence.
// ----------------------------------------------------------------------------

void session_inbound::attach_handshake(const channel::ptr& channel,
    result_handler&& handler) const NOEXCEPT
{
    session::attach_handshake(channel, std::move(handler));
}

void session_inbound::handle_channel_start(const code&,
    const channel::ptr&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
}

void session_inbound::attach_protocols(
    const channel::ptr& channel) const NOEXCEPT
{
    session::attach_protocols(channel);
}

void session_inbound::handle_channel_stop(const code&,
    const channel::ptr&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
