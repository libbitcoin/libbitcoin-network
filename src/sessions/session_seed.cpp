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
#include <bitcoin/network/sessions/session_seed.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <bitcoin/system.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_seed
#define NAME "session_seed"

using namespace bc::system;
using namespace std::placeholders;

session_seed::session_seed(p2p& network)
  : session(network), remaining_(settings().seeds.size())
{
}

bool session_seed::notify() const
{
    return false;
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_seed::start(result_handler handler)
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (is_zero(settings().host_pool_capacity))
    {
        ////LOG_INFO(LOG_NETWORK)
        ////    << "Not configured to populate an address pool." << std::endl;
        handler(error::success);
        return;
    }

    session::start(BIND2(handle_started, _1, handler));
}

void session_seed::handle_started(const code& ec,
    result_handler handler)
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        handler(ec);
        return;
    }

    const auto start_size = address_count();

    if (!is_zero(start_size))
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Seeding is not required because there are "
            << start_size << " cached addresses." << std::endl;
        handler(error::success);
        return;
    }

    if (settings().seeds.empty())
    {
        LOG_ERROR(LOG_NETWORK)
            << "Seeding is required but no seeds are configured." << std::endl;
        handler(error::operation_failed);
        return;
    }

    const auto counter = BIND3(handle_complete, _1, start_size, handler);

    // The handler is invoked once any seeds are generated, though
    // seeding continues until all seeding channels are closed.
    for (const auto& seed: settings().seeds)
        start_seed(seed, counter);
}

// Seed sequence.
// ----------------------------------------------------------------------------

void session_seed::start_seed(const config::endpoint& seed,
    result_handler counter)
{
    if (stopped())
    {
        counter(error::channel_stopped);
        return;
    }

    const auto connector = create_connector();
    store_connector(connector);

    // OUTBOUND CONNECT
    connector->connect(seed,
        BIND5(handle_connect, _1, _2, seed, connector, counter));
}

void session_seed::handle_connect(const code& ec, channel::ptr channel,
    const config::endpoint& seed, connector::ptr connector,
    result_handler counter)
{
    if (ec)
    {
        counter(ec);
        return;
    }

    if (blacklisted(channel->authority()))
    {
        counter(error::address_blocked);
        return;
    }

    start_channel(channel, BIND2(handle_channel_start, _1, channel), counter);
}

void session_seed::handle_channel_start(const code& ec, channel::ptr channel)
{
    if (ec)
        return;

    // Calls attach_protocols on channel strand.
    post_attach_protocols(channel);
}

void session_seed::attach_protocols(channel::ptr channel) const
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

    channel->do_attach<protocol_seed_31402>(*this)->start();
}

////void session_seed::handle_channel_stop(const code& ec, result_handler counter)
////{
////    counter(ec);
////}

void session_seed::handle_complete(const code& ec, size_t start_size,
    result_handler counter)
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Objective previously completed (and handler invoked).
    if (is_zero(remaining_))
        return;

    if (address_count() > start_size)
    {
        // Signal objective completed.
        remaining_ = zero;

        // Singular handler invoke (other seeds will continue).
        counter(error::success);
        return;
    }

    if (is_zero(--remaining_))
    {
        // Singular handler invoke.
        // Not increased and none remaining, so signal unsuccessful.
        counter(error::seeding_unsuccessful);
    }
}

void session_seed::attach_handshake(channel::ptr channel,
    result_handler handshake) const
{
    BC_ASSERT_MSG(channel->stranded(), "strand");

    // Don't use configured services or relay for seeding.
    const auto relay = false;
    const auto own_version = settings().protocol_maximum;
    const auto own_services = messages::service::node_none;
    const auto invalid_services = settings().invalid_services;
    const auto minimum_version = settings().protocol_minimum;
    const auto minimum_services = messages::service::node_none;

    // Reject messages are not handled until bip61 (70002).
    // The negotiated_version is initialized to the configured maximum.
    if (channel->negotiated_version() >= messages::level::bip61)
        channel->do_attach<protocol_version_70002>(*this, own_version,
            own_services, invalid_services, minimum_version, minimum_services,
            relay)->start(handshake);
    else
        channel->do_attach<protocol_version_31402>(*this, own_version,
            own_services, invalid_services, minimum_version, minimum_services)
            ->start(handshake);
}

} // namespace network
} // namespace libbitcoin
