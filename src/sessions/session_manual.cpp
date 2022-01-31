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
#include <bitcoin/network/protocols/protocols.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_manual

using namespace bc::system;
using namespace std::placeholders;

session_manual::session_manual(p2p& network)
  : session(network),
    attempt_limit_(network.network_settings().manual_attempt_limit)
{
}

// Start sequence.
// ----------------------------------------------------------------------------
// Manual connections are always enabled.

void session_manual::start(result_handler handler)
{
    BC_ASSERT_MSG(network_.strand().running_in_this_thread(), "strand");
    session::start(BIND2(handle_started, _1, handler));
}

void session_manual::handle_started(const code& ec,
    result_handler handler)
{
    BC_ASSERT_MSG(network_.strand().running_in_this_thread(), "strand");
    handler(ec);
}

// Connect sequence/cycle.
// ----------------------------------------------------------------------------

void session_manual::connect(const std::string& hostname, uint16_t port)
{
    BC_ASSERT_MSG(network_.strand().running_in_this_thread(), "strand");
    connect(hostname, port, {});
}

void session_manual::connect(const std::string& hostname, uint16_t port,
    channel_handler handler)
{
    BC_ASSERT_MSG(network_.strand().running_in_this_thread(), "strand");
    start_connect(error::success, hostname, port, attempt_limit_, handler);
}

// The first connect is a sequence, which then spawns a cycle.
void session_manual::start_connect(const code&,
    const std::string& hostname, uint16_t port, uint32_t attempts,
    channel_handler handler)
{
    BC_ASSERT_MSG(network_.strand().running_in_this_thread(), "strand");

    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    const auto retries = floored_subtract(attempts, 1u);
    const auto connector = create_connector();

    // CONNECT
    connector->connect(hostname, port,
        BIND7(handle_connect, _1, _2, hostname, port, retries, connector, handler));
}

void session_manual::handle_connect(const code& ec,
    channel::ptr channel, const std::string& hostname, uint16_t port,
    uint32_t remaining, connector::ptr connector, channel_handler handler)
{
    BC_ASSERT_MSG(network_.strand().running_in_this_thread(), "strand");

    if (ec)
    {
        // Retry forever if limit is zero.
        remaining = attempt_limit_ == 0 ? 1 : remaining;

        if (remaining > 0)
        {
            // TODO: use timer to delay start in case of error other than
            // channel_timeout. use network_.network_settings().connect_timeout().

            start_connect(error::success, hostname, port, remaining, handler);
            return;
        }

        // This is a failure end of the connect sequence.
        handler(ec, nullptr);
        return;
    }

    start_channel(channel,
        BIND6(handle_channel_start, _1, hostname, port, remaining, channel, handler),
        BIND5(handle_channel_stop, _1, hostname, port, remaining, handler));
}

void session_manual::handle_channel_start(const code& ec,
    const std::string& hostname, uint16_t port, uint32_t remaining,
    channel::ptr channel, channel_handler handler)
{
    BC_ASSERT_MSG(network_.strand().running_in_this_thread(), "strand");

    if (ec)
    {
        // Retry forever if limit is zero.
        remaining = attempt_limit_ == 0 ? 1 : remaining;

        // This is a failure end of the connect sequence.
        // If stop handler won't retry, invoke handler with error.
        if ((ec == error::address_in_use) || (remaining == 0))
            handler(ec, channel);

        return;
    }

    // Calls attach_protocols on channel strand.
    post_attach_protocols(channel);

    // This is the success end of the connect sequence.
    handler(error::success, channel);
}

// Communication will begin after this function returns, freeing the thread.
void session_manual::attach_protocols1(channel::ptr channel) const
{
    // Channel attach and start both require channel strand.
    BC_ASSERT_MSG(channel->stranded(), "channel: attach, start");

    const auto version = channel->negotiated_version();
    const auto heartbeat = network_.network_settings().channel_heartbeat();

    if (version >= messages::level::bip31)
        channel->do_attach<protocol_ping_60001>(heartbeat)->start();
    else
        channel->do_attach<protocol_ping_31402>(heartbeat)->start();

    if (version >= messages::level::bip61)
        channel->do_attach<protocol_reject_70002>()->start();

    channel->do_attach<protocol_address_31402>(network_)->start();
}

void session_manual::handle_channel_stop(const code& ec,
    const std::string& hostname, uint16_t port, uint32_t remaining,
    channel_handler handler)
{
    // Special case for already connected, do not keep trying.
    if (ec == error::address_in_use)
        return;

    // Retry forever if limit is zero.
    remaining = attempt_limit_ == 0 ? 1 : remaining;

    if (remaining > 0)
    {
        start_connect(error::success, hostname, port, remaining, handler);
        return;
    }

    // hit attempt limit, no restart.
}

} // namespace network
} // namespace libbitcoin
