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
#include <bitcoin/network/sessions/session_seed.hpp>

#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_seed

using namespace system;
using namespace std::placeholders;

// Bind throws (ok).
// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

session_seed::session_seed(p2p& network, uint64_t identifier) NOEXCEPT
  : session(network, identifier), tracker<session_seed>(network.log)
{
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_seed::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Seeding is allowed even with !enable_address configured.

    if (is_zero(settings().outbound_connections) ||
        is_zero(settings().connect_batch_size))
    {
        LOGN("Bypassed seeding because outbound connections disabled.");
        handler(error::success);
        unsubscribe_close();
        return;
    }

    if (address_count() >= settings().minimum_address_count())
    {
        LOGN("Bypassed seeding because of sufficient ("
            << address_count() << " of " << settings().minimum_address_count()
            << ") address quantity.");
        handler(error::success);
        unsubscribe_close();
        return;
    }

    if (is_zero(settings().host_pool_capacity))
    {
        LOGN("Cannot seed because no address pool capacity configured.");
        handler(error::seeding_unsuccessful);
        unsubscribe_close();
        return;
    }

    if (settings().seeds.empty())
    {
        LOGN("Cannot seed because no seeds configured");
        handler(error::seeding_unsuccessful);
        unsubscribe_close();
        return;
    }

    session::start(BIND(handle_started, _1, std::move(handler)));
}

void session_seed::handle_started(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        handler(ec);
        unsubscribe_close();
        return;
    }

    const auto seeds = settings().seeds.size();
    const auto required = settings().minimum_address_count();

    LOGN("Seeding because of insufficient ("
        << address_count() << " of " << required << ") address quantity.");

    // Bogus warning, this pointer is copied into std::bind().
    BC_PUSH_WARNING(NO_UNUSED_LOCAL_SMART_PTR)
    const auto racer = std::make_shared<race>(seeds, required);
    BC_POP_WARNING()

    // Invoke sufficient on count, invoke complete with all seeds stopped.
    racer->start(move_copy(handler), BIND(stop_seed, _1));

    for (const auto& seed: settings().seeds)
    {
        const auto connector = create_connector();
        subscribe_stop([=](const code&) NOEXCEPT
        {
            connector->stop();
            return false;
        });

        start_seed(error::success, seed, connector,
            BIND(handle_connect, _1, _2, seed, racer));
    }
}

// Seed sequence.
// ----------------------------------------------------------------------------

// Attempt to connect one seed.
void session_seed::start_seed(const code&, const config::endpoint& seed,
    const connector::ptr& connector, const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    LOGN("Connecting to seed [" << seed << "]");

    // Guard restartable connector (shutdown delay).
    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    connector->connect(seed, move_copy(handler));
}

void session_seed::handle_connect(const code& ec, const socket::ptr& socket,
    const config::endpoint& LOG_ONLY(seed), const race::ptr& racer) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        BC_ASSERT_MSG(!socket || socket->stopped(), "unexpected socket");
        LOGN("Failed to connect seed address [" << seed << "] " << ec.message());
        racer->finish(address_count());
        return;
    }

    const auto channel = create_channel(socket, true);

    start_channel(channel,
        BIND(handle_channel_start, _1, channel),
        BIND(handle_channel_stop, _1, channel, racer));
}

void session_seed::attach_handshake(const channel::ptr& channel,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for attach");

    // Tx relay is parsed (by version) but always set to false for seeding.
    constexpr auto enable_transaction = false;

    // Seeding does not require or provide any node services.
    // Nodes that require inbound connection services/txs will not accept.
    constexpr auto minimum_services = messages::service::node_none;
    constexpr auto maximum_services = messages::service::node_none;

    const auto self = shared_from_this();
    const auto maximum_version = settings().protocol_maximum;
    const auto extended_version = maximum_version >= messages::level::bip37;
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

void session_seed::handle_channel_start(const code& ec,
    const channel::ptr& LOG_ONLY(channel)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        LOGN("Seed start [" << channel->authority() << "] " << ec.message());
    }
}

void session_seed::attach_protocols(const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");

    const auto self = shared_from_this();
    const auto enable_alert = settings().enable_alert;
    const auto negotiated_version = channel->negotiated_version();
    const auto enable_pong = negotiated_version >= messages::level::bip31;
    const auto enable_reject = settings().enable_reject &&
        negotiated_version >= messages::level::bip61;

    if (enable_pong)
        channel->attach<protocol_ping_60001>(self)->start();
    else
        channel->attach<protocol_ping_31402>(self)->start();

    // Alert is deprecated.
    if (enable_alert)
        channel->attach<protocol_alert_31402>(self)->start();

    // Reject is deprecated.
    if (enable_reject)
        channel->attach<protocol_reject_70002>(self)->start();

    // Seed protocol stops upon completion, causing session removal.
    channel->attach<protocol_seed_31402>(self)->start();
}

void session_seed::handle_channel_stop(const code& LOG_ONLY(ec),
    const channel::ptr& LOG_ONLY(channel), const race::ptr& racer) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    LOGN("Seed stop [" << channel->authority() << "] " << ec.message());
    racer->finish(address_count());
}

void session_seed::stop_seed(const code&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    LOGN("Seed session complete.");
    unsubscribe_close();
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
