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

#include <algorithm>
#include <functional>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
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
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

session_outbound::session_outbound(p2p& network) NOEXCEPT
  : session(network), tracker<session_outbound>(network.log())
{
}

bool session_outbound::inbound() const NOEXCEPT
{
    return false;
}

bool session_outbound::notify() const NOEXCEPT
{
    return true;
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_outbound::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (is_zero(settings().outbound_connections) || 
        is_zero(settings().host_pool_capacity) ||
        is_zero(settings().connect_batch_size))
    {
        LOG("Not configured for outbound connections.");
        handler(error::bypassed);
        return;
    }

    if (is_zero(address_count()))
    {
        LOG("Configured for outbound but no addresses.");
        handler(error::address_not_found);
        return;
    }

    session::start(BIND2(handle_started, _1, std::move(handler)));
}

void session_outbound::handle_started(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(!stopped(), "session stopped in start");

    if (ec)
    {
        handler(ec);
        return;
    }

    for (size_t id = 0; id < settings().outbound_connections; ++id)
    {
        // Create a batch of connectors for each outbound connection.
        const auto connectors = create_connectors(settings().connect_batch_size);

        for (const auto& connector: *connectors)
            subscribe_stop([=](const code&) NOEXCEPT
            {
                connector->stop();
            });

        // Start connection attempt with batch of connectors for one peer.
        start_connect(connectors, id);
    }

    LOG("Creating " << settings().outbound_connections << " connections "
        << settings().connect_batch_size << " at a time.");

    // This is the end of the start sequence, does not indicate connect status.
    handler(error::success);
}

// Connnect cycle.
// ----------------------------------------------------------------------------

// Attempt to connect one peer using a batch subset of connectors.
void session_outbound::start_connect(const connectors_ptr& connectors,
    size_t id) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Terminates retry loops (and connector is restartable).
    if (stopped())
        return;

    // Count down the number of connection attempts within the batch.
    const auto counter = std::make_shared<size_t>(connectors->size());

    channel_handler connect =
        BIND4(handle_connect, _1, _2, connectors, id);

    channel_handler one =
        BIND5(handle_one, _1, _2, counter, connectors, std::move(connect));

    // Attempt to connect with a unique address for each connector of batch.
    for (const auto& connector: *connectors)
        take(BIND4(do_one, _1, _2, connector, one));
}

// Attempt to connect the given peer and invoke handle_one.
void session_outbound::do_one(const code& ec, const config::address& peer,
    const connector::ptr& connector, const channel_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // These terminations prevent a tight loop in the empty address pool case.

    if (ec)
    {
        // This can only be address_not_found (empty, no peer).
        //// LOG("verbose " << ec.message());
        handler(ec, nullptr);
        return;
    }

    if (insufficient(peer))
    {
        LOG("Dropping insufficient address [" << peer << "]");
        handler(error::address_insufficient, nullptr);
        return;
    }

    if (unsupported(peer))
    {
        LOG("Dropping unsupported address [" << peer << "]");
        handler(error::address_unsupported, nullptr);
        return;
    }

    if (blacklisted(peer))
    {
        LOG("Dropping blacklisted address [" << peer << "]");
        handler(error::address_blocked, nullptr);
        return;
    }

    // Guard restartable connector (shutdown delay).
    if (stopped())
    {
        // Can't call untake without a channel object, so restore.
        handler(error::service_stopped, nullptr);
        restore(peer.item_ptr(), BIND1(handle_untake, _1));
        return;
    }

    connector->connect(peer, move_copy(handler));
}

// Handle each do_one connection attempt, stopping on first success.
void session_outbound::handle_one(const code& ec, const channel::ptr& channel,
    const count_ptr& count, const connectors_ptr& connectors,
    const channel_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // A successful connection has already occurred, drop this one.
    if (is_zero(*count))
    {
        if (channel)
        {
            // Preceeds handsjake and can untake with success or other code.
            channel->stop(error::channel_dropped);
            untake(ec, channel);
        }

        return;
    }

    // Last indicates that this is the last attempt.
    const auto last = is_zero(--(*count));

    // This connection is successful but there may be others outstanding.
    // Short-circuit subsequent attempts and clear outstanding connectors.
    if (!ec && !last)
    {
        *count = zero;
        for (const auto& connector: *connectors)
            connector->stop();
    }

    // Got a connection.
    if (!ec)
    {
        handler(error::success, channel);
        return;
    }

    // No more connectors remaining and no connections.
    if (ec && last)
    {
        // Disabled due to verbosity, reenable under verbose logging.
        ////LOG("Failed to connect outbound channel, " << ec.message());

        // Reduce the set of errors from the batch to connect_failed.
        handler(error::connect_failed, nullptr);
        return;
    }

    // ec && !last/done, drop this connector attempt.
    // service_stopped on a pending connect causes non-restore of that address,
    // since a channel is never created if there is a connector error code.
    // This is treated as an acceptable loss of a potentially valid address.
    BC_ASSERT_MSG(!channel, "unexpected channel instance");
}

// Handle the singular batch result.
void session_outbound::handle_connect(const code& ec,
    const channel::ptr& channel, const connectors_ptr& connectors,
    size_t id) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Guard restartable timer (shutdown delay).
    if (stopped())
    {
        if (channel)
        {
            channel->stop(error::service_stopped);
            untake(ec, channel);
        }

        return;
    }

    // This is always connect_failed with nullptr, no log.
    // There was an error connecting a channel, so try again after delay.
    if (ec)
    {
        BC_ASSERT_MSG(!channel, "unexpected channel instance");
        const auto timeout = settings().connect_timeout();

        // Provides a timeout delay in case of empty address pool, etc.
        start_timer(BIND2(start_connect, connectors, id), timeout);
        return;
    }

    start_channel(channel,
        BIND3(handle_channel_start, _1, channel, id),
        BIND4(handle_channel_stop, _1, channel, id, connectors));
}

void session_outbound::attach_handshake(const channel::ptr& channel,
    result_handler&& handler) const NOEXCEPT
{
    session::attach_handshake(channel, std::move(handler));
}

void session_outbound::handle_channel_start(const code& LOG_ONLY(ec),
    const channel::ptr& LOG_ONLY(channel), size_t LOG_ONLY(id)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    LOG("Outbound channel start [" << channel->authority() << "] "
        "(" << id << ") " << ec.message());
}

void session_outbound::attach_protocols(
    const channel::ptr& channel) const NOEXCEPT
{
    session::attach_protocols(channel);
}

void session_outbound::handle_channel_stop(const code& ec,
    const channel::ptr& channel, size_t id,
    const connectors_ptr& connectors) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    LOG("Outbound channel stop [" << channel->authority() << "] "
        "(" << id << ") " << ec.message());

    untake(ec, channel);

    // The channel stopped following connection, try again without delay.
    // This is the only opportunity for a tight loop (could use timer).
    start_connect(connectors, id);
}

void session_outbound::untake(const code& ec,
    const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(channel, "channel");

    if (ec && ec != error::service_stopped)
        return;

    const auto foo = channel->updated_address();

    // Handshake may , in which case peer services may remain default (0).
    LOG("Update [" << config::address(foo) << "] " << ec.message());

    // Update timestamp and set peer services before placing back to host pool.
    restore(channel->updated_address(), BIND1(handle_untake, _1));
}

void session_outbound::handle_untake(const code&) const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
