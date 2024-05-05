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
#include <bitcoin/network/sessions/session_outbound.hpp>

#include <algorithm>
#include <functional>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>
#include <bitcoin/network/sessions/session.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_outbound

using namespace system;
using namespace config;
using namespace messages;
using namespace std::placeholders;

// Bind throws (ok).
// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

session_outbound::session_outbound(p2p& network, uint64_t identifier) NOEXCEPT
  : session(network, identifier), tracker<session_outbound>(network.log)
{
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_outbound::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (!settings().outbound_enabled())
    {
        LOGN("Not configured for outbound connections.");
        handler(error::success);
        unsubscribe_close();
        return;
    }

    if (is_zero(address_count()))
    {
        LOGN("Configured for outbound but no addresses.");
        handler(error::address_not_found);
        unsubscribe_close();
        return;
    }

    if (!settings().enable_address)
    {
        LOGN("Address protocol disabled, may cause empty address pool.");
    }

    session::start(BIND(handle_started, _1, std::move(handler)));
}

void session_outbound::handle_started(const code& ec,
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

    const auto peers = settings().outbound_connections;

    LOG_ONLY(const auto batch = settings().connect_batch_size;)
    LOGN("Create " << peers << " connections " << batch << " at a time.");

    // There is currently no way to vary the number of connections at runtime.
    for (size_t peer = 0; peer < peers; ++peer)
        start_connect(error::success);

    // This is the end of the start sequence (actually at connector->connect).
    handler(error::success);
}

// Connnect cycle.
// ----------------------------------------------------------------------------

// Attempt to connect one peer using a batch of connectors.
void session_outbound::start_connect(const code&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Terminates retry loops.
    if (stopped())
        return;

    // Create a set of connectors for batched stop.
    const auto connectors = create_connectors(settings().connect_batch_size);

    // Subscribe connector set to stop desubscriber.
    const auto key = subscribe_stop([=](const code&) NOEXCEPT
    {
        for (const auto& connector: *connectors)
            connector->stop();
        return false;
    });

    // Bogus warning, this pointer is copied into std::bind().
    BC_PUSH_WARNING(NO_UNUSED_LOCAL_SMART_PTR)
    const auto racer = std::make_shared<race>(connectors->size());
    BC_POP_WARNING()
            
    // Race to first success or last failure.
    racer->start(BIND(handle_connect, _1, _2, key));

    // Attempt to connect with unique address for each connector of batch.
    for (const auto& connector: *connectors)
        take(BIND(do_one, _1, _2, key, racer, connector));
}

// Attempt to connect the given peer and invoke handle_one.
void session_outbound::do_one(const code& ec, const config::address& peer,
    object_key key, const race::ptr& racer,
    const connector::ptr& connector) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    ////COUNT(events::outbound1, key);

    if (ec)
    {
        ////LOGS("Address pool is empty.");
        racer->finish(ec, nullptr);
        return;
    }

    // Guard restartable connector (shutdown delay).
    if (stopped())
    {
        restore(peer, BIND(handle_reclaim, _1));
        racer->finish(error::service_stopped, nullptr);
        return;
    }

    connector->connect(peer, BIND(handle_one, _1, _2, key, peer, racer));
}

// Handle each do_one connection attempt, stopping on first success.
void session_outbound::handle_one(const code& ec, const socket::ptr& socket,
    object_key key, const config::address& peer,
    const race::ptr& racer) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    ////COUNT(events::outbound2, key);

    if (ec == error::service_suspended)
        restore(peer, BIND(handle_reclaim, _1));

    // Winner in quality race is first to pass success.
    if (racer->finish(ec, socket))
    {
        // Since there is a winner, accelerate connector stop.
        notify(key);
        return;
    }

    // Stop socket and reclaim address if not the winning finisher.
    reclaim(ec, socket);
}

// Handle the singular batch result.
void session_outbound::handle_connect(const code& ec,
    const socket::ptr& socket, object_key key) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    ////COUNT(events::outbound3, key);

    // Unregister connectors, in case there was no winner.
    notify(key);

    // Guard restartable timer (shutdown delay).
    if (stopped())
    {
        reclaim(ec, socket);
        return;
    }

    if (ec == error::address_not_found)
    {
        LOGS("Address pool is empty.");
        defer(settings().connect_timeout(), BIND(start_connect, _1));
        return;
    }

    if (ec == error::service_suspended)
    {
        ////LOGS("Suspended outbound channel start.");
        defer(BIND(start_connect, _1));
        return;
    }

    // There was an error connecting a channel, so try again after delay.
    if (ec)
    {
        // Avoid tight loop with delay timer.
        defer(BIND(start_connect, _1));
        return;
    }

    const auto channel = create_channel(socket, false);

    start_channel(channel,
        BIND(handle_channel_start, _1, channel),
        BIND(handle_channel_stop, _1, channel));
}

void session_outbound::attach_handshake(const channel::ptr& channel,
    result_handler&& handler) NOEXCEPT
{
    session::attach_handshake(channel, std::move(handler));
}

void session_outbound::handle_channel_start(const code&,
    const channel::ptr&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    ////LOGS("Outbound channel start [" << channel->authority() << "] "
    ////    "(" << key << ") " << ec.message());
}

void session_outbound::attach_protocols(
    const channel::ptr& channel) NOEXCEPT
{
    session::attach_protocols(channel);
}

void session_outbound::handle_channel_stop(const code& ec,
    const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    ////LOGS("Outbound channel stop [" << channel->authority() << "] "
    ////    "(" << key << ") " << ec.message());

    reclaim(ec, channel);

    // Cannot be tight loop due to handshake.
    start_connect(ec);
}

// Address reclaim and socket/channel stop.
// ----------------------------------------------------------------------------
// private

inline bool session_outbound::maybe_reclaim(const code& ec) const NOEXCEPT
{
    // Bypass if host pool is full (don't allow these to evict others).
    if (address_count() >= settings().host_pool_capacity)
        return false;

    // Failures that might work later (timeouts can drain pool).
    return ec == error::operation_timeout
        || ec == error::channel_timeout
        || ec == error::peer_disconnect;
}

inline bool session_outbound::always_reclaim(const code& ec) const NOEXCEPT
{
    // Terminations that worked or might have worked.
    return ec == error::success
        || ec == error::operation_canceled
        || ec == error::channel_expired;
}

// Use initial address time and services, since connection not completed.
void session_outbound::reclaim(const code& ec,
    const socket::ptr& socket) NOEXCEPT
{
    if (!socket)
        return;

    // Reclaiming address implies socket must be stopped.
    socket->stop();

    if (stopped() || always_reclaim(ec) || maybe_reclaim(ec))
    {
        restore(socket->address(), BIND(handle_reclaim, _1));
    }
}

// Set address to current time and services from peer version message.
void session_outbound::reclaim(const code& ec,
    const channel::ptr& channel) NOEXCEPT
{
    if (!channel)
        return;

    // Reclaiming address implies channel must be stopped.
    channel->stop(error::operation_canceled);

    if (stopped() || always_reclaim(ec) || maybe_reclaim(ec))
    {
        restore(channel->get_updated_address(), BIND(handle_reclaim, _1));
    }
}

void session_outbound::handle_reclaim(const code&) const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
