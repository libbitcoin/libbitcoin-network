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
#include <bitcoin/network/sessions/session_client.hpp>

#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net.hpp>
#include <bitcoin/network/sessions/session.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_client

using namespace system;
using namespace std::placeholders;

// Bind throws (ok).
// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

// TODO: parameterize settings().[type] struct
session_client::session_client(net& network, uint64_t identifier,
    const config::endpoints& bindings, size_t connections,
    const std::string& name) NOEXCEPT
  : session(network, identifier),
    network_(network),
    bindings_(bindings),
    connections_(connections),
    name_(name)
{
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_client::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Path is also currently required for admin and explore.
    if (bindings_.empty() || is_zero(connections_))
    {
        LOGN("Not configured for " << name_ << " connections.");
        handler(error::success);
        unsubscribe_close();
        return;
    }

    session::start(BIND(handle_started, _1, std::move(handler)));
}

void session_client::handle_started(const code& ec,
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

    LOGN("Accepting " << connections_ << " " << name_ << " clients on "
        << bindings_.size() << " bindings.");

    for (const auto& bind: bindings_)
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

// Attempt to accept peers on each configured endpoint.
void session_client::start_accept(const code&,
    const acceptor::ptr& acceptor) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Terminates accept loop (and acceptor is restartable).
    if (stopped())
        return;

    acceptor->accept(BIND(handle_accepted, _1, _2, acceptor));
}

void session_client::handle_accepted(const code& ec,
    const socket::ptr& socket, const acceptor::ptr& acceptor) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Guard restartable timer (shutdown delay).
    if (stopped())
    {
        if (socket) socket->stop();
        return;
    }

    // Client services are allowed to connect and get busy response.
    ////if (ec == error::service_suspended)
    ////{
    ////    ////LOGS("Suspended " << name_ << " channel start.");
    ////    defer(BIND(start_accept, _1, acceptor));
    ////    return;
    ////}

    // There was an error accepting the channel, so try again after delay.
    if (ec)
    {
        BC_ASSERT_MSG(!socket || socket->stopped(), "unexpected socket");
        LOGF("Failed to accept " << name_ << " connection, " << ec.message());
        defer(BIND(start_accept, _1, acceptor));
        return;
    }

    // Client services are allowed to connect and get busy response.
    ////if (!enabled())
    ////{
    ////    LOGS("Dropping " << name_ << " connection (disabled).");
    ////    socket->stop();
    ////    return;
    ////}

    // We have to drop without responding, otherwise count will overflow.
    // Could instead stop listening when at limit, though this is simpler.
    if (channel_count_ >= connections_)
    {
        LOGS("Dropping oversubscribed " << name_ << " connection ["
            << socket->authority() << "].");
        socket->stop();
        return;
    }

    // TODO: black/white listing.
    // Client services are allowed to connect and get unauthorized response.
    ////const auto address = socket->authority().to_address_item();

    // TODO: pass session info to channel (busy, unauthorized).
    // Creates channel_client cast returned as channel::ptr.
    const auto channel = create_channel(socket);

    LOGS("Accepted " << name_ << " connection [" << channel->authority()
        << "] on binding [" << acceptor->local() << "].");

    // There was no error, so listen again without delay.
    start_accept(error::success, acceptor);

    start_channel(channel,
        BIND(handle_channel_start, _1, channel),
        BIND(handle_channel_stop, _1, channel));
}

// Channel sequence.
// ----------------------------------------------------------------------------

// Some client types do not utilize a handshake, so default is bypassed.
void session_client::attach_handshake(const channel::ptr&,
    result_handler&&) NOEXCEPT
{
    BC_ASSERT(false);
}

// Handshake bypassed, channel remains paused until after protocol attach.
void session_client::do_attach_handshake(
    const channel::ptr& BC_DEBUG_ONLY(channel),
    const result_handler& handshake) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for handshake attach");
    handshake(error::success);
}

// Completion sequence.
// ----------------------------------------------------------------------------

void session_client::handle_channel_start(const code& LOG_ONLY(ec),
    const channel::ptr& LOG_ONLY(channel)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    LOGS("Inbound " << name_ << " channel start [" << channel->authority()
        << "] " << ec.message());

    BC_ASSERT(!is_zero(add1(channel_count_)));
    channel_count_ = ceilinged_add(channel_count_, one);
}

void session_client::handle_channel_stop(const code& LOG_ONLY(ec),
    const channel::ptr& LOG_ONLY(channel)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    LOGS("Inbound " << name_ << " channel stop [" << channel->authority()
        << "] " << ec.message());

    BC_ASSERT(!is_zero(channel_count_));
    channel_count_ = floored_subtract(channel_count_, one);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
