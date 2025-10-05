/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/sessions/session_inbound_client.hpp>

#include <utility>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net.hpp>
#include <bitcoin/network/protocols/protocols.hpp>
#include <bitcoin/network/sessions/session_client.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_inbound_client

using namespace system;
using namespace std::placeholders;

// Bind throws (ok).
// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

session_inbound_client::session_inbound_client(net& network,
    uint64_t identifier) NOEXCEPT
  : session_client(network, identifier),
    tracker<session_inbound_client>(network.log)
{
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_inbound_client::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (!settings().admin.enabled())
    {
        LOGN("Not configured for client connections.");
        handler(error::success);
        unsubscribe_close();
        return;
    }

    session_client::start(BIND(handle_started, _1, std::move(handler)));
}

void session_inbound_client::handle_started(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(!stopped(), "session stopped in start");

    if (ec)
    {
        handler(ec);
        unsubscribe_close();
        return;
    }

    LOGN("Accepting " << settings().admin.connections << " clients on "
        << settings().admin.binds.size() << " bindings.");

    for (const auto& bind: settings().admin.binds)
    {
        const auto acceptor = create_acceptor();

        // Require that all acceptors at least start.
        if (const auto error_code = acceptor->start(bind))
        {
            handler(error_code);
            return;
        }

        LOGN("Bound to client endpoint [" << acceptor->local() << "].");

        // Subscribe acceptor to stop desubscriber.
        subscribe_stop([=](const code&) NOEXCEPT
        {
            acceptor->stop();
            return false;
        });

        start_accept(error::success, acceptor);
    }

    handler(error::success);
}

// Accept cycle.
// ----------------------------------------------------------------------------

// Attempt to accept clients on each configured endpoint.
void session_inbound_client::start_accept(const code&,
    const acceptor::ptr& acceptor) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Terminates accept loop (and acceptor is restartable).
    if (stopped())
        return;

    acceptor->accept(BIND(handle_accept, _1, _2, acceptor));
}

void session_inbound_client::handle_accept(const code& ec,
    const socket::ptr& socket, const acceptor::ptr& acceptor) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Guard restartable timer (shutdown delay).
    if (stopped())
    {
        if (socket) socket->stop();
        return;
    }

    if (ec == error::service_suspended)
    {
        ////LOGS("Suspended inbound client channel start.");
        defer(BIND(start_accept, _1, acceptor));
        return;
    }

    // There was an error accepting the channel, so try again after delay.
    if (ec)
    {
        BC_ASSERT_MSG(!socket || socket->stopped(), "unexpected socket");
        LOGF("Failed to accept client connection, " << ec.message());
        defer(BIND(start_accept, _1, acceptor));
        return;
    }

    if (!enabled())
    {
        LOGS("Dropping client connection (disabled).");
        socket->stop();
        return;
    }

    // There was no error, so listen again without delay.
    start_accept(error::success, acceptor);

    // Creates channel_client cast returned as channel::ptr.
    const auto channel = create_channel(socket);

    LOGS("Accepted client connection [" << channel->authority() << "] on binding ["
        << acceptor->local() << "].");

    start_channel(channel,
        BIND(handle_channel_start, _1, channel),
        BIND(handle_channel_stop, _1, channel));
}

bool session_inbound_client::enabled() const NOEXCEPT
{
    return true;
}

// Completion sequence.
// ----------------------------------------------------------------------------

// Handshake bypassed, channel remains paused until after protocol attach.
void session_inbound_client::do_attach_handshake(
    const channel::ptr& BC_DEBUG_ONLY(channel),
    const result_handler& handshake) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for handshake attach");
    handshake(error::success);
}

void session_inbound_client::handle_channel_start(const code& LOG_ONLY(ec),
    const channel::ptr& LOG_ONLY(channel)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    LOGS("Inbound client channel start [" << channel->authority() << "] "
        << ec.message());
}

void session_inbound_client::attach_protocols(
    const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for protocol attach");

    const auto self = shared_from_this();
    channel->attach<protocol_client>(self)->start();
}

// TODO: presently nothing to invoke channel stop when the channel drops.
// TODO: so this will not be called until the node is stopped.
void session_inbound_client::handle_channel_stop(const code& LOG_ONLY(ec),
    const channel::ptr& LOG_ONLY(channel)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    LOGS("Inbound client channel stop [" << channel->authority() << "] "
        << ec.message());
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
