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

using namespace bc::system;
using namespace std::placeholders;

session_seed::session_seed(p2p& network) noexcept
  : session(network)
{
}

bool session_seed::inbound() const noexcept
{
    return false;
}

bool session_seed::notify() const noexcept
{
    return false;
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_seed::start(result_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (is_zero(settings().host_pool_capacity) || settings().seeds.empty())
    {
        ////LOG_INFO(LOG_NETWORK)
        ////    << "Not configured to populate an address pool." << std::endl;
        handler(error::success);
        return;
    }

    if (!is_zero(address_count()))
    {
        ////LOG_INFO(LOG_NETWORK)
        ////    << "Bypassed seeding due to existing addresses." << std::endl;
        handler(error::success);
        return;
    }

    session::start(BIND2(handle_started, _1, handler));
}

void session_seed::handle_started(const code& ec,
    result_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(!stopped(), "session stopped in start");

    if (ec)
    {
        handler(ec);
        return;
    }

    // Create a connector for each seed connection.
    const auto connectors = create_connectors(settings().seeds.size());
    const auto counter = std::make_shared<size_t>(connectors->size());
    auto it = settings().seeds.begin();

    for (const auto connector: *connectors)
    {
        const auto& seed = *(it++);

        stop_subscriber_->subscribe([=](const code&)
        {
            connector->stop();
        });

        start_seed(seed, connector,
            BIND5(handle_connect, _1, _2, seed, counter, handler));
    }
}

// Seed sequence.
// ----------------------------------------------------------------------------

// Attempt to connect one seed.
void session_seed::start_seed(const config::endpoint& seed,
    connector::ptr connector, channel_handler handler) noexcept
{
    // Guard restartable connector (shutdown delay).
    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    connector->connect(seed, std::move(handler));
}

void session_seed::handle_connect(const code& ec, channel::ptr channel,
    const config::endpoint& seed, count_ptr counter,
    result_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        BC_ASSERT_MSG(!channel, "unexpected channel instance");

        // Handle channel result in stop handler (not yet registered).
        handle_channel_stop(ec, counter, handler);
        return;
    }

    start_channel(channel,
        BIND2(handle_channel_start, _1, channel),
        BIND3(handle_channel_stop, _1, counter, handler));
}

void session_seed::attach_handshake(const channel::ptr& channel,
    result_handler handshake) const noexcept
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
        channel->attach<protocol_version_70002>(*this, own_version,
            own_services, invalid_services, minimum_version, minimum_services,
            relay)->start(handshake);
    else
        channel->attach<protocol_version_31402>(*this, own_version,
            own_services, invalid_services, minimum_version, minimum_services)
            ->start(handshake);
}

void session_seed::handle_channel_start(const code& ec, channel::ptr channel) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
}

void session_seed::attach_protocols(const channel::ptr& channel) const noexcept
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

    // The seed protocol will stop the channel upon completion/timeout.
    channel->attach<protocol_seed_31402>(*this)->start();
}

void session_seed::handle_channel_stop(const code& ec, count_ptr counter,
    result_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(!is_zero(*counter), "unexpected seed count");

    // Unless service is stopped, all channels will conclude here.
    if (is_zero(--(*counter)))
        handler(is_zero(address_count()) ? error::seeding_unsuccessful :
            error::success);
}

} // namespace network
} // namespace libbitcoin
