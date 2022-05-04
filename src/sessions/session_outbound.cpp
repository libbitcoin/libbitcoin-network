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

#include <cstddef>
#include <functional>
#include <bitcoin/system.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>
#include <bitcoin/network/sessions/session.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_outbound

using namespace bc::system;
using namespace config;
using namespace std::placeholders;

session_outbound::session_outbound(p2p& network) noexcept
  : session(network),
    batch_(std::max(network.network_settings().connect_batch_size, 1u))
{
}

bool session_outbound::inbound() const noexcept
{
    return false;
}

bool session_outbound::notify() const noexcept
{
    return true;
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_outbound::start(result_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (is_zero(settings().outbound_connections) || 
        is_zero(settings().host_pool_capacity))
    {
        handler(error::success);
        return;
    }

    if (is_zero(address_count()))
    {
        handler(error::address_not_found);
        return;
    }

    session::start(BIND2(handle_started, _1, handler));
}

void session_outbound::handle_started(const code& ec,
    result_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        handler(ec);
        return;
    }

    for (size_t peer = 0; peer < settings().outbound_connections; ++peer)
    {
        // Create batch connectors for each outbound connection.
        // Connectors operate on the network strand but connect asynchronously.
        // Resolution is asynchronous and connection occurs on socket strand.
        // So actual connection attempts run in parallel, apart from setup and
        // response handling within the connector.
        const auto connectors = create_connectors(batch_);

        // Stop all connectors upon session stop.
        for (const auto& connector: *connectors)
            stop_subscriber_->subscribe([=](const code&)
            {
                connector->stop();
            });

        // Start connection attempt with batch of connectors for one peer.
        start_connect(connectors);
    }

    // This is the end of the start sequence, does not indicate connect status.
    handler(error::success);
}

// Connnect cycle.
// ----------------------------------------------------------------------------

// Attempt to connect one peer using a batch subset of connectors.
void session_outbound::start_connect(connectors_ptr connectors) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
        return;

    batch(connectors, BIND3(handle_connect, _1, _2, connectors));
}

// Attempt to connect one peer using a batch subset of connectors.
void session_outbound::batch(connectors_ptr connectors, 
    channel_handler handle_connect) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Count the number of connection attempts within the batch.
    auto counter = std::make_shared<size_t>(zero);

    channel_handler handle_batch =
        BIND5(handle_batch, _1, _2, counter, connectors, std::move(handle_connect));

    // Attempt to connect with a unique address for each connector of batch.
    for (const auto& connector: *connectors)
        fetch(BIND4(do_one, _1, _2, connector, handle_batch));
}

// Attempt to connect the given host and invoke handle_connect.
void session_outbound::do_one(const code& ec, const authority& host,
    connector::ptr connector, channel_handler handle_batch) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        handle_batch(error::service_stopped, nullptr);
        return;
    }

    // This termination prevents a tight loop in the empty address pool case.
    if (ec)
    {
        handle_batch(ec, nullptr);
        return;
    }

    // This creates a tight loop in the case of a small address pool.
    if (blacklisted(host))
    {
        handle_batch(error::address_blocked, nullptr);
        return;
    }

    connector->connect(host, std::move(handle_batch));
}

// Handle each do_one connection attempt, stopping on first success.
void session_outbound::handle_batch(const code& ec, channel::ptr channel,
    count_ptr count, connectors_ptr connectors,
    channel_handler handle_connect) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // A successful connection has already occurred.
    if (*count == batch_)
        return;

    // Finished indicates that this is the last attempt.
    const auto finished = (++(*count) == batch_);

    // This connection is successful but there are others outstanding.
    // Short-circuit subsequent attempts and clear outstanding connectors.
    if (!ec && !finished)
    {
        *count = batch_;
        for (auto it = connectors->begin(); it != connectors->end(); ++it)
            (*it)->stop();
    }

    // Got a connection.
    if (!ec)
    {
        handle_connect(error::success, channel);
        return;
    }

    // No more connectors remaining and no connections.
    if (ec && finished)
    {
        handle_connect(error::connect_failed, nullptr);
        return;
    }

    BC_ASSERT_MSG(!channel, "unexpected channel instance");
}

// Handle each socket connection within the batch of connectors.
void session_outbound::handle_connect(const code& ec, channel::ptr channel,
    connectors_ptr connectors) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec == error::service_stopped)
    {
        BC_ASSERT_MSG(!channel, "unexpected channel instance");
        return;
    }

    // There was an error connecting a channel, so try again after delay.
    if (ec)
    {
        timer_->start(BIND1(start_connect, connectors),
            settings().connect_timeout());
        return;
    }

    if (stopped())
    {
        channel->stop(error::service_stopped);
        return;
    }

    start_channel(channel,
        BIND2(handle_channel_start, _1, channel),
        BIND2(handle_channel_stop, _1, connectors));
}

void session_outbound::attach_handshake(const channel::ptr& channel,
    result_handler handler) const noexcept
{
    BC_ASSERT_MSG(channel->stranded(), "strand");

    const auto relay = settings().relay_transactions;
    const auto own_version = settings().protocol_maximum;
    const auto own_services = settings().services;
    const auto invalid_services = settings().invalid_services;
    const auto minimum_version = settings().protocol_minimum;

    // Require peer to serve network (and witness if configured on self).
    const auto min_service = (own_services & messages::service::node_witness) |
        messages::service::node_network;

    // Reject messages are not handled until bip61 (70002).
    // The negotiated_version is initialized to the configured maximum.
    if (channel->negotiated_version() >= messages::level::bip61)
        channel->do_attach<protocol_version_70002>(*this, own_version,
            own_services, invalid_services, minimum_version, min_service, relay)
            ->start(handler);
    else
        channel->do_attach<protocol_version_31402>(*this, own_version,
            own_services, invalid_services, minimum_version, min_service)
            ->start(handler);
}

void session_outbound::handle_channel_start(const code& ec,
    channel::ptr) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // A handshake failure is caught by session::handle_channel_stopped,
    // which stops the channel, so do not stop the channel here.
    // handle_channel_stop has a copy of the connectors for retry.
}

void session_outbound::attach_protocols(
    const channel::ptr& channel) const noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

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

void session_outbound::handle_channel_stop(const code&,
    connectors_ptr connectors) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // When a connected channel drops, connect another.
    start_connect(connectors);
}

} // namespace network
} // namespace libbitcoin
