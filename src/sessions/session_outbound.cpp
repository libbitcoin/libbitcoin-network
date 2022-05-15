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
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
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
  : session(network)
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

void session_outbound::start(result_handler&& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (is_zero(settings().outbound_connections) || 
        is_zero(settings().host_pool_capacity) ||
        is_zero(settings().connect_batch_size))
    {
        ////LOG_INFO(LOG_NETWORK)
        ////    << "Not configured for outbound connections." << std::endl;
        handler(error::bypassed);
        return;
    }

    if (is_zero(address_count()))
    {
        ////LOG_INFO(LOG_NETWORK)
        ////    << "Configured for outbound but no addresses." << std::endl;
        handler(error::address_not_found);
        return;
    }

    session::start(BIND2(handle_started, _1, std::move(handler)));
}

void session_outbound::handle_started(const code& ec,
    const result_handler& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(!stopped(), "session stopped in start");

    if (ec)
    {
        handler(ec);
        return;
    }

    for (size_t peer = 0; peer < settings().outbound_connections; ++peer)
    {
        // Create a batch of connectors for each outbount connection.
        const auto connectors = create_connectors(settings().connect_batch_size);

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
void session_outbound::start_connect(const connectors_ptr& connectors) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Terminates retry loops (and connector is restartable).
    if (stopped())
        return;

    // Count down the number of connection attempts within the batch.
    const auto counter = std::make_shared<size_t>(connectors->size());

    channel_handler connect =
        BIND3(handle_connect, _1, _2, connectors);

    channel_handler one =
        BIND5(handle_one, _1, _2, counter, connectors, std::move(connect));

    // Attempt to connect with a unique address for each connector of batch.
    for (const auto& connector: *connectors)
        fetch(BIND4(do_one, _1, _2, connector, one));
}

// Attempt to connect the given host and invoke handle_one.
void session_outbound::do_one(const code& ec, const authority& host,
    const connector::ptr& connector, const channel_handler& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // This termination prevents a tight loop in the empty address pool case.
    if (ec)
    {
        handler(ec, nullptr);
        return;
    }

    // This termination prevents a tight loop in the case of a small address pool.
    if (blacklisted(host))
    {
        handler(error::address_blocked, nullptr);
        return;
    }

    connector->connect(host, move_copy(handler));
}

// Handle each do_one connection attempt, stopping on first success.
void session_outbound::handle_one(const code& ec, const channel::ptr& channel,
    const count_ptr& count, const connectors_ptr& connectors,
    const channel_handler& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // A successful connection has already occurred, drop this one.
    if (is_zero(*count))
    {
        if (channel)
            channel->stop(error::channel_dropped);

        return;
    }

    // Finished indicates that this is the last attempt.
    const auto finished = is_zero(--(*count));

    // This connection is successful but there are others outstanding.
    // Short-circuit subsequent attempts and clear outstanding connectors.
    if (!ec && !finished)
    {
        *count = zero;
        for (auto it = connectors->begin(); it != connectors->end(); ++it)
            (*it)->stop();
    }

    // Got a connection.
    if (!ec)
    {
        handler(error::success, channel);
        return;
    }

    // No more connectors remaining and no connections.
    if (ec && finished)
    {
        // TODO: log discarded code.
        // Reduce the set of errors from the batch to connect_failed.
        handler(error::connect_failed, nullptr);
        return;
    }

    BC_ASSERT_MSG(!channel, "unexpected channel instance");
}

// Handle the singular batch result.
void session_outbound::handle_connect(const code& ec,
    const channel::ptr& channel, const connectors_ptr& connectors) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Guard restartable timer (shutdown delay).
    if (stopped())
    {
        if (channel)
            channel->stop(error::service_stopped);

        return;
    }

    // This is always connect_failed, no log (reduced).
    // There was an error connecting a channel, so try again after delay.
    if (ec)
    {
        BC_ASSERT_MSG(!channel, "unexpected channel instance");
        timer_->start(BIND1(start_connect, connectors),
            settings().connect_timeout());
        return;
    }

    start_channel(channel,
        BIND2(handle_channel_start, _1, channel),
        BIND2(handle_channel_stop, _1, connectors));
}

void session_outbound::attach_handshake(const channel::ptr& channel,
    result_handler&& handler) const noexcept
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
        channel->attach<protocol_version_70002>(*this, own_version,
            own_services, invalid_services, minimum_version, min_service, relay)
            ->start(std::move(handler));
    else
        channel->attach<protocol_version_31402>(*this, own_version,
            own_services, invalid_services, minimum_version, min_service)
            ->start(std::move(handler));
}

void session_outbound::handle_channel_start(const code&, const channel::ptr&) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
}

void session_outbound::attach_protocols(
    const channel::ptr& channel) const noexcept
{
    BC_ASSERT_MSG(channel->stranded(), "strand");

    const auto version = channel->negotiated_version();
    const auto heartbeat = settings().channel_heartbeat();

    if (version >= messages::level::bip31)
        channel->attach<protocol_ping_60001>(*this, heartbeat)->start();
    else
        channel->attach<protocol_ping_31402>(*this, heartbeat)->start();

    if (version >= messages::level::bip61)
        channel->attach<protocol_reject_70002>(*this)->start();

    channel->attach<protocol_address_31402>(*this)->start();
}

void session_outbound::handle_channel_stop(const code&,
    const connectors_ptr& connectors) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // The channel stopped following connection, try again without delay.
    // This is the only opportunity for a tight loop (could use timer).
    start_connect(connectors);
}

} // namespace network
} // namespace libbitcoin
