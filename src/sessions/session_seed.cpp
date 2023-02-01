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

#include <functional>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_seed

using namespace bc::system;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

session_seed::session_seed(p2p& network) NOEXCEPT
  : session(network), tracker<session_seed>(network.log())
{
}

bool session_seed::inbound() const NOEXCEPT
{
    return false;
}

bool session_seed::notify() const NOEXCEPT
{
    return false;
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_seed::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (is_zero(settings().outbound_connections))
    {
        LOG("Bypassed seeding because no outbound connections configured.");
        handler(error::bypassed);
        return;
    }

    if (address_count() >= settings().minimum_address_count())
    {
        LOG("Bypassed seeding because of sufficient address quantity.");
        handler(error::bypassed);
        return;
    }

    if (is_zero(settings().host_pool_capacity) || settings().seeds.empty())
    {
        LOG("Bypassed seeding because of no address pool capacity.");
        handler(error::seeding_unsuccessful);
        return;
    }

    session::start(BIND2(handle_started, _1, std::move(handler)));
}

void session_seed::stop() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Set stopped state.
    session::stop();

    // Stop all seeding channels.
    for (const auto& channel: seeding_)
        channel->stop(error::service_stopped);

    // Free all seeding channels.
    seeding_.clear();
}

void session_seed::handle_started(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Seeding runs entirely in start and is possible to stop.
    ////BC_ASSERT_MSG(!stopped(), "session stopped in start");

    if (ec)
    {
        handler(ec);
        return;
    }

    // Create a connector for each seed connection.
    const auto connectors = create_connectors(settings().seeds.size());
    const auto counter = std::make_shared<size_t>(connectors->size());
    auto it = settings().seeds.begin();

    for (const auto& connector: *connectors)
    {
        const auto& seed = *(it++);

        subscribe_stop([=](const code&) NOEXCEPT
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
    const connector::ptr& connector, const channel_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    LOG("Connecting to seed node [" << seed << "]");

    // Guard restartable connector (shutdown delay).
    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    connector->connect(seed, move_copy(handler));
}

void session_seed::handle_connect(const code& ec, const channel::ptr& channel,
    const config::endpoint& LOG_ONLY(seed), const count_ptr& counter,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // If ec channel will be nullptr in handle_channel_stop.
    LOG("Connect seed [" << seed << "]: " << ec.message());

    if (ec)
    {
        BC_ASSERT_MSG(!channel, "unexpected channel instance");

        // Handle channel result in stop handler (not yet registered).
        handle_channel_stop(ec, counter, channel, handler);
        return;
    }

    start_channel(channel,
        BIND2(handle_channel_start, _1, channel),
        BIND4(handle_channel_stop, _1, counter, channel, handler));
}

void session_seed::attach_handshake(const channel::ptr& channel,
    result_handler&& handler) const NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for attach");

    // Weak reference safe as sessions outlive protocols.
    const auto& self = *this;
    const auto enable_reject = settings().enable_reject;
    const auto maximum_version = settings().protocol_maximum;

    // Seeding does not require or provide any node services or allow relay.
    constexpr auto minimum_services = messages::service::node_none;
    constexpr auto maximum_services = messages::service::node_none;
    constexpr auto enable_relay = false;

    // Reject is supported starting at bip61 (70002) and later deprecated.
    if (enable_reject && maximum_version >= messages::level::bip61)
        channel->attach<protocol_version_70002>(self, minimum_services,
            maximum_services, enable_relay)->shake(std::move(handler));

    // Relay is supported starting at bip37 (70001).
    else if (maximum_version >= messages::level::bip37)
        channel->attach<protocol_version_70001>(self, minimum_services,
            maximum_services, enable_relay)->shake(std::move(handler));

    else
        channel->attach<protocol_version_31402>(self, minimum_services,
            maximum_services)->shake(std::move(handler));
}

void session_seed::handle_channel_start(const code& LOG_ONLY(ec),
    const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    LOG("Seed channel start [" << channel->authority() << "] " << ec.message());

    // Pend seeding channel.
    seeding_.insert(channel);
}

void session_seed::attach_protocols(const channel::ptr& channel) const NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");

    // Weak reference safe as sessions outlive protocols.
    const auto& self = *this;
    const auto enable_alert = settings().enable_alert;
    const auto enable_reject = settings().enable_reject;
    const auto negotiated_version = channel->negotiated_version();

    if (negotiated_version >= messages::level::bip31)
        channel->attach<protocol_ping_60001>(self)->start();
    else
        channel->attach<protocol_ping_31402>(self)->start();

    // Alert is deprecated.
    if (enable_alert)
        channel->attach<protocol_alert_31402>(self)->start();

    // Reject is supported starting at bip61 (70002) and later deprecated.
    if (enable_reject && negotiated_version >= messages::level::bip61)
        channel->attach<protocol_reject_70002>(self)->start();

    // Seeding takes place of address protocol, stops upon completion/timeout.
    channel->attach<protocol_seed_31402>(self)->start();
}

void session_seed::handle_channel_stop(const code& ec, const count_ptr& counter,
    const channel::ptr& channel, const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // handle_channel_stop invoked with null if ec.
    if (channel)
    {
        LOG("Seed channel stop [" << channel->authority() << "]");
    }

    // handle_channel_stop invoked with null if ec.
    if (ec != error::service_stopped && !to_bool(seeding_.erase(channel)))
    {
        LOG("Unpend failed to locate seed channel (ok on stop).");
    }

    // Ignore result if previously handled (early termination).
    if (is_zero(*counter))
        return;

    // Handle service stopped, ignoring possible success/fail result.
    if (stopped())
    {
        *counter = zero;
        handler(error::service_stopped);
        return;
    }

    // Handle with success on first positive address count.
    if (address_count() >= settings().minimum_address_count())
    {
        *counter = zero;
        handler(error::success);
        return;
    }

    // Handle failure now that all seeds are processed.
    if (is_zero(--(*counter)))
    {
        handler(error::seeding_unsuccessful);
        return;
    }
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
