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
#include <bitcoin/network/sessions/session_manual.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocol_address_31402.hpp>
#include <bitcoin/network/protocols/protocol_ping_31402.hpp>
#include <bitcoin/network/protocols/protocol_ping_60001.hpp>
#include <bitcoin/network/protocols/protocol_reject_70002.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_manual

using namespace bc::system;
using namespace std::placeholders;

session_manual::session_manual(p2p& network, bool notify_on_connect)
  : session(network, notify_on_connect)
{
}

// Start sequence.
// ----------------------------------------------------------------------------
// Manual connections are always enabled.
// Handshake pend not implemented for manual connections (connect to self ok).

void session_manual::start(result_handler handler)
{
    LOG_INFO(LOG_NETWORK)
        << "Starting manual session.";

    session::start(BIND2(handle_started, _1, handler));
}

void session_manual::handle_started(const code& ec,
    result_handler handler)
{
    if (ec)
    {
        handler(ec);
        return;
    }

    // This is the end of the start sequence.
    handler(error::success);
}

// Connect sequence/cycle.
// ----------------------------------------------------------------------------

void session_manual::connect(const std::string& hostname, uint16_t port)
{
    const auto unhandled = [](code, channel::ptr) {};
    connect(hostname, port, unhandled);
}

void session_manual::connect(const std::string& hostname, uint16_t port,
    channel_handler handler)
{
    start_connect(error::success, hostname, port,
        settings_.manual_attempt_limit, handler);
}

// The first connect is a sequence, which then spawns a cycle.
void session_manual::start_connect(const code&,
    const std::string& hostname, uint16_t port, uint32_t attempts,
    channel_handler handler)
{
    if (stopped())
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Suspended manual connection.";

        handler(error::service_stopped, nullptr);
        return;
    }

    const auto retries = floored_subtract(attempts, 1u);
    const auto connector = create_connector();
    pend(connector);

    // MANUAL CONNECT OUTBOUND
    connector->connect(hostname, port,
        BIND7(handle_connect, _1, _2, hostname, port, retries, connector,
            handler));
}

// THIS IS INVOKED ON THE CHANNEL THREAD.
void session_manual::handle_connect(const code& ec,
    channel::ptr channel, const std::string& hostname, uint16_t port,
    uint32_t remaining, connector::ptr connector, channel_handler handler)
{
    unpend(connector);

    if (ec)
    {
        LOG_WARNING(LOG_NETWORK)
            << "Failure connecting ["
            << config::endpoint(hostname, port)
            << "] manually: " << ec.message();

        // Retry forever if limit is zero.
        remaining = settings_.manual_attempt_limit == 0 ? 1 : remaining;

        if (remaining > 0)
        {
            ////// Retry with conditional delay in case of network error.
            ////dispatch_delayed(cycle_delay(ec),
            ////    BIND5(start_connect, _1, hostname, port, remaining, handler));
            return;
        }

        LOG_WARNING(LOG_NETWORK)
            << "Suspending manual connection to ["
            << config::endpoint(hostname, port) << "] after "
            << settings_.manual_attempt_limit << " failed attempts.";

        // This is a failure end of the connect sequence.
        handler(ec, nullptr);
        return;
    }

    register_channel(channel,
        BIND6(handle_channel_start, _1, hostname, port, remaining, channel, handler),
        BIND5(handle_channel_stop, _1, hostname, port, remaining, handler));
}

// THIS IS INVOKED ON THE CHANNEL THREAD.
void session_manual::handle_channel_start(const code& ec,
    const std::string& hostname, uint16_t port, uint32_t remaining,
    channel::ptr channel, channel_handler handler)
{
    if (ec)
    {
        LOG_INFO(LOG_NETWORK)
            << "Manual channel failed to start [" << channel->authority()
            << "] " << ec.message();

        // Retry forever if limit is zero.
        remaining = settings_.manual_attempt_limit == 0 ? 1 : remaining;

        // This is a failure end of the connect sequence.
        // If stop handler won't retry, invoke handler with error.
        if ((ec == error::address_in_use) || (remaining == 0))
            handler(ec, channel);

        return;
    }

    LOG_INFO(LOG_NETWORK)
        << "Connected manual channel ["
        << config::endpoint(hostname, port)
        << "] as [" << channel->authority() << "] ("
        << connection_count() << ")";

    // This is the success end of the connect sequence.
    handler(error::success, channel);
    attach_protocols(channel);
}

// THIS IS INVOKED ON THE CHANNEL THREAD.
// Communication will begin after this function returns, freeing the thread.
void session_manual::attach_protocols(channel::ptr channel)
{
    const auto version = channel->negotiated_version();
    const auto heartbeat = network_.network_settings().channel_heartbeat();

    if (version >= messages::level::bip31)
        attach<protocol_ping_60001>(channel, heartbeat)->start();
    else
        attach<protocol_ping_31402>(channel, heartbeat)->start();

    if (version >= messages::level::bip61)
        attach<protocol_reject_70002>(channel)->start();

    attach<protocol_address_31402>(channel, network_)->start();
}

// THIS IS INVOKED ON THE CHANNEL THREAD (if the channel stops itself).
void session_manual::handle_channel_stop(const code& ec,
    const std::string& hostname, uint16_t port, uint32_t remaining,
    channel_handler handler)
{
    LOG_DEBUG(LOG_NETWORK)
        << "Manual channel stopped: " << ec.message();

    // Special case for already connected, do not keep trying.
    if (ec == error::address_in_use)
        return;

    // Retry forever if limit is zero.
    remaining = settings_.manual_attempt_limit == 0 ? 1 : remaining;

    if (remaining > 0)
    {
        start_connect(error::success, hostname, port, remaining, handler);
        return;
    }

    LOG_WARNING(LOG_NETWORK)
        << "Not restarting manual connection to ["
        << config::endpoint(hostname, port) << "] after "
        << settings_.manual_attempt_limit << " failed attempts.";
}

} // namespace network
} // namespace libbitcoin
