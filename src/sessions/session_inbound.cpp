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
#include <bitcoin/network/sessions/session_inbound.hpp>

#include <functional>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_inbound

using namespace system;
using namespace std::placeholders;

// Bind throws (ok).
// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

session_inbound::session_inbound(p2p& network, uint64_t identifier) NOEXCEPT
  : session(network, identifier), tracker<session_inbound>(network.log)
{
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_inbound::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (!settings().inbound_enabled())
    {
        LOGN("Not configured for inbound connections.");
        handler(error::success);
        unsubscribe_close();
        return;
    }

    session::start(BIND(handle_started, _1, std::move(handler)));
}

void session_inbound::handle_started(const code& ec,
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

    LOGN("Accepting " << settings().inbound_connections << " connections on "
        << settings().binds.size() << " bindings.");

    for (const auto& bind: settings().binds)
    {
        const auto acceptor = create_acceptor();

        // Require that all acceptors at least start.
        if (const auto error_code = acceptor->start(bind))
        {
            handler(error_code);
            return;
        }

        LOGN("Bound to endpoint [" << acceptor->local() << "].");

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
void session_inbound::start_accept(const code&,
    const acceptor::ptr& acceptor) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Terminates accept loop (and acceptor is restartable).
    if (stopped())
        return;

    acceptor->accept(BIND(handle_accept, _1, _2, acceptor));
}

void session_inbound::handle_accept(const code& ec,
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
        ////LOGS("Suspended inbound channel start.");
        defer(BIND(start_accept, _1, acceptor));
        return;
    }

    // There was an error accepting the channel, so try again after delay.
    if (ec)
    {
        BC_ASSERT_MSG(!socket || socket->stopped(), "unexpected socket");
        LOGF("Failed to accept inbound connection, " << ec.message());
        defer(BIND(start_accept, _1, acceptor));
        return;
    }

    // There was no error, so listen again without delay.
    start_accept(error::success, acceptor);

    const auto address = socket->authority().to_address_item();

    if (!whitelisted(address))
    {
        ////LOGS("Dropping not whitelisted connection [" << socket->authority() << "].");
        socket->stop();
        return;
    }

    if (blacklisted(address))
    {
        ////LOGS("Dropping blacklisted connection [" << socket->authority() << "].");
        socket->stop();
        return;
    }

    // Could instead stop listening when at limit, though this is simpler.
    if (inbound_channel_count() >= settings().inbound_connections)
    {
        LOGS("Dropping oversubscribed connection [" << socket->authority() << "].");
        socket->stop();
        return;
    }

    const auto channel = create_channel(socket, false);

    LOGS("Accepted inbound connection [" << channel->authority() << "] on binding ["
        << acceptor->local() << "].");

    start_channel(channel,
        BIND(handle_channel_start, _1, channel),
        BIND(handle_channel_stop, _1, channel));
}

bool session_inbound::blacklisted(const config::address& address) const NOEXCEPT
{
    return settings().blacklisted(address);
}

bool session_inbound::whitelisted(const config::address& address) const NOEXCEPT
{
    return settings().whitelisted(address);
}

// Completion sequence.
// ----------------------------------------------------------------------------

void session_inbound::attach_handshake(const channel::ptr& channel,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for attach");

    // Inbound does not require any node services (e.g. bitnodes.io is zero).
    constexpr auto minimum_services = messages::service::node_none;

    const auto self = shared_from_this();
    const auto maximum_version = settings().protocol_maximum;
    const auto maximum_services = settings().services_maximum;
    const auto extended_version = maximum_version >= messages::level::bip37;
    const auto enable_transaction = settings().enable_transaction;
    const auto enable_reject = settings().enable_reject &&
        maximum_version >= messages::level::bip61;

    // Protocol must pause the channel after receiving version and verack.

    // Reject is deprecated.
    if (enable_reject)
        channel->attach<protocol_version_70002>(self, minimum_services,
            maximum_services, enable_transaction)->shake(std::move(handler));

    else if (extended_version)
        channel->attach<protocol_version_70001>(self, minimum_services,
            maximum_services, enable_transaction)->shake(std::move(handler));

    else
        channel->attach<protocol_version_31402>(self, minimum_services,
            maximum_services)->shake(std::move(handler));
}

void session_inbound::handle_channel_start(const code&,
    const channel::ptr&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    ////LOGS("Inbound channel start [" << channel->authority() << "] "
    ////    << ec.message());
}

void session_inbound::attach_protocols(
    const channel::ptr& channel) NOEXCEPT
{
    session::attach_protocols(channel);
}

void session_inbound::handle_channel_stop(const code& LOG_ONLY(ec),
    const channel::ptr& LOG_ONLY(channel)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    LOGS("Inbound channel stop [" << channel->authority() << "] "
        << ec.message());
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
