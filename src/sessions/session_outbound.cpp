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

session_outbound::session_outbound(p2p& network, size_t key) NOEXCEPT
  : session(network, key), tracker<session_outbound>(network.log())
{
}

bool session_outbound::inbound() const NOEXCEPT
{
    return false;
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_outbound::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (!settings().outbound_enabled())
    {
        LOG("Not configured for outbound connections.");
        handler(error::bypassed);
        unsubscribe_close();
        return;
    }

    if (is_zero(address_count()))
    {
        LOG("Configured for outbound but no addresses.");
        handler(error::address_not_found);
        unsubscribe_close();
        return;
    }

    if (!settings().enable_address)
    {
        LOG("Address protocol disabled, may cause empty address pool.");
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

    const auto batch = settings().connect_batch_size;
    const auto count = settings().outbound_connections;

    for (size_t index = 0; index < count; ++index)
    {
        const auto connectors = create_connectors(batch);

        // TODO: move into start connect and desubscribe.
        start_connect(error::success, connectors, subscribe_stop(
            [=](const code&) NOEXCEPT
            {
                for (const auto& connector: *connectors)
                    connector->stop();

                return false;
            }));
    }

    LOG("Creating " << count << " connections " << batch << " at a time.");

    // This is the end of the start sequence, does not indicate connect status.
    handler(error::success);
}

// Connnect cycle.
// ----------------------------------------------------------------------------

// Attempt to connect one peer using a batch subset of connectors.
void session_outbound::start_connect(const code&,
    const connectors_ptr& connectors, object_key key) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Terminates retry loops (and connector is restartable).
    if (stopped())
        return;

    ////LOG("Starting connection batch (" << key << ").");

    // Count down the number of connection attempts within the batch.
    const auto counter = integer::create(connectors->size());

    ////LOG("Group (" << key << ") start_connect {batch:" << counter->value() << "}");

    socket_handler connect =
        BIND4(handle_connect, _1, _2, connectors, key);

    socket_handler one =
        BIND6(handle_one, _1, _2, counter, connectors, key, std::move(connect));

    // Attempt to connect with a unique address for each connector of batch.
    for (const auto& connector: *connectors)
        take(BIND6(do_one, _1, _2, key, connector, counter, one));
}

// Attempt to connect the given peer and invoke handle_one.
void session_outbound::do_one(const code& ec, const config::address& peer,
    object_key key, const connector::ptr& connector, const count_ptr& counter,
    const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    ////LOG("Group (" << key << ") do_one {batch:" << counter->value() << "}");

    if (ec)
    {
        // This can only be error::address_not_found (empty, no peer).
        ////LOG("Getting address: " << ec.message());
        handler(ec, nullptr);
        return;
    }

    if (disabled(peer))
    {
        // Should not see these unless there is a change to enable_ipv6.
        ////LOG("Dropping disabled protocol address [" << peer << "]");
        handler(error::address_disabled, nullptr);
        return;
    }

    if (unsupported(peer))
    {
        // Should not see these unless there is a change to invalid_services.
        ////LOG("Dropping unsupported address [" << peer << "]");
        handler(error::address_unsupported, nullptr);
        return;
    }

    if (insufficient(peer))
    {
        // Should not see these unless there is a change to services_minimum.
        ////LOG("Dropping insufficient address [" << peer << "]");
        handler(error::address_insufficient, nullptr);
        return;
    }

    if (blacklisted(peer))
    {
        // Should not see these unless there is a change to blacklist config.
        ////LOG("Dropping blacklisted address [" << peer << "]");
        handler(error::address_blocked, nullptr);
        return;
    }

    if (connected(peer))
    {
        // More common with higher connection count relative to hosts count.
        LOG("Dropping connected address [" << peer << "]");
        handler(error::address_in_use, nullptr);
        return;
    }

    // Guard restartable connector (shutdown delay).
    if (stopped())
    {
        // Ensure the peer address is restored.
        handle_connector(error::service_stopped, nullptr, peer, key, counter,
            handler);
        return;
    }

    // Capture peer for restoration if there is no channel.
    connector->connect(peer,
        BIND6(handle_connector, _1, _2, peer, key, counter, handler));
}

// Calling connector->stop() either from handle_started or handle_one results
// in its timer cancelation, which cancels its handler. In either case the
// address has not been validated, so restore to pool here - without update.
void session_outbound::handle_connector(const code& ec,
    const socket::ptr& socket, const config::address& peer, object_key,
    const count_ptr&, const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    ////LOG("Group (" << key << ") handle_connector {batch:" << counter->value() << "}");

    if (stopped() || ec == error::operation_canceled)
    {
        ////LOG("Restore [" << peer << "] (" << key << ") " << ec.message());
        restore(peer, BIND1(handle_untake, _1));
    }
    else if (ec)
    {
        ////LOG("Dropping failed address [" << peer << "] " << ec.message());
    }

    handler(ec, socket);
}

// Handle each do_one connection attempt, stopping on first success.
void session_outbound::handle_one(const code& ec, const socket::ptr& socket,
    const count_ptr& counter, const connectors_ptr& connectors, object_key,
    const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    ////LOG("Group (" << key << ") handle_one {batch:" << counter->value() << "} " << ec.message());

    if (counter->is_handled())
    {
        if (socket)
        {
            socket->stop();
            untake(ec, socket);
        }

        ////LOG("Group (" << key << ") handle_one [" << counter->value() << "] {exit 0}");
        return;
    }

    counter->decrement();

    if (ec)
    {
        if (counter->is_complete())
        {
            counter->set_handled();

            ////LOG("Group (" << key << ") handle_one [" << counter->value() << "] {exit 2}");
            handler(error::connect_failed, socket);
        }

        ////LOG("Group (" << key << ") handle_one [" << counter->value() << "] {exit 3}");
        return;
    }

    ////LOG("Group (" << key << ") handle_one [" << counter->value() << "] {exit 4}");

    counter->set_handled();
    handler(error::success, socket);

    ////LOG("Group (" << key << ") handle_one [" << counter->value() << "] {canceling}");

    // TODO: unsubscribe using object_key, handler invokes stop loop.
    for (const auto& connector: *connectors)
        connector->stop();

    ////LOG("Group (" << key << ") handle_one [" << counter->value() << "] {complete}");
}

// Handle the singular batch result.
void session_outbound::handle_connect(const code& ec,
    const socket::ptr& socket, const connectors_ptr& connectors,
    object_key key) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    ////LOG("Group (" << key << ") handle_connect.");

    // Guard restartable timer (shutdown delay).
    if (stopped())
    {
        if (socket) socket->stop();
        untake(ec, socket);
        return;
    }

    // There was an error connecting a channel, so try again after delay.
    if (ec)
    {
        ////LOG("Failed to connect outbound address, " << ec.message());
        ////LOG("Group (" << key << ") defer.");

        defer(BIND3(start_connect, _1, connectors, key));
        return;
    }

    const auto channel = create_channel(socket, false);

    start_channel(channel,
        BIND3(handle_channel_start, _1, channel, key),
        BIND4(handle_channel_stop, _1, channel, key, connectors));
}

void session_outbound::attach_handshake(const channel::ptr& channel,
    result_handler&& handler) const NOEXCEPT
{
    session::attach_handshake(channel, std::move(handler));
}

void session_outbound::handle_channel_start(const code&, const channel::ptr&,
    object_key) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    ////LOG("Outbound channel start [" << channel->authority() << "] "
    ////    "(" << key << ") " << ec.message());
    ////LOG("Group (" << key << ") started.");
}

void session_outbound::attach_protocols(
    const channel::ptr& channel) const NOEXCEPT
{
    session::attach_protocols(channel);
}

void session_outbound::handle_channel_stop(const code& ec,
    const channel::ptr& channel, object_key key,
    const connectors_ptr& connectors) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    ////LOG("Group (" << key << ") handle_channel_stop.");
    ////LOG("Outbound channel stop [" << channel->authority() << "] "
    ////    "(" << key << ") " << ec.message());

    untake(ec, channel);

    // This is invoked from the channel::proxy stop subscriber, and is then
    // reposted to the network strand by session, so there is no recursion.
    ////boost::asio::post(strand(),
    ////    BIND3(start_connect, error::success, connectors, key));

    // The channel stopped following connection, try again without delay.
    // Potentially a tight loop, but a new address is selected for retry.
    ////start_connect(error::success, connectors, key);
    defer(BIND3(start_connect, _1, connectors, key));
}

void session_outbound::untake(const code& ec,
    const socket::ptr& socket) NOEXCEPT
{
    // Use initial address, since connection not completed.
    if ((!ec || stopped()) && socket)
        restore(socket->address(), BIND1(handle_untake, _1));
}

void session_outbound::untake(const code& ec,
    const channel::ptr& channel) NOEXCEPT
{
    // Set address to current time and services from peer version message.
    if ((!ec || stopped()) && channel)
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
