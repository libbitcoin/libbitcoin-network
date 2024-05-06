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
#include <bitcoin/network/sessions/session_manual.hpp>

#include <functional>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_manual

using namespace system;
using namespace config;
using namespace std::placeholders;

// Bind throws (ok).
// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

session_manual::session_manual(p2p& network, uint64_t identifier) NOEXCEPT
  : session(network, identifier), tracker<session_manual>(network.log)
{
}

// Start/stop sequence.
// ----------------------------------------------------------------------------
// Manual connections are always enabled.

void session_manual::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    session::start(BIND(handle_started, _1, std::move(handler)));
}

void session_manual::handle_started(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    handler(ec);
}

// Connect sequence.
// ----------------------------------------------------------------------------

void session_manual::connect(const config::endpoint& peer) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    const auto self = shared_from_base<session_manual>();
    connect(peer, [=](const code&, channel::ptr) NOEXCEPT
    {
        self->nop();
        return true;
    });
}

void session_manual::connect(const config::endpoint& peer,
    channel_notifier&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Create a persistent connector for the manual connection.
    const auto connector = create_connector();

    subscribe_stop([=](const code&) NOEXCEPT
    {
        connector->stop();
        return false;
    });

    LOGN("Maintaining manual connection to [" << peer << "]");
    start_connect(error::success, peer, connector, std::move(handler));
}

// Connect cycle.
// ----------------------------------------------------------------------------

void session_manual::start_connect(const code&, const endpoint& peer,
    const connector::ptr& connector, const channel_notifier& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Terminates retry loops (and connector is restartable).
    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    connector->connect(peer,
        BIND(handle_connect, _1, _2, peer, connector, handler));
}

void session_manual::handle_connect(const code& ec, const socket::ptr& socket,
    const endpoint& peer, const connector::ptr& connector,
    const channel_notifier& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Guard restartable timer (shutdown delay).
    if (stopped())
    {
        if (socket) socket->stop();
        handler(error::service_stopped, nullptr);
        return;
    }

    if (ec == error::service_suspended)
    {
        ////LOGS("Suspended manual channel start [" << peer << "].");
        defer(BIND(start_connect, _1, peer, connector, handler));
        return;
    }

    // There was an error connecting the channel, so try again after delay.
    if (ec)
    {
        BC_ASSERT_MSG(!socket || socket->stopped(), "unexpected socket");
        LOGS("Failed to connect manual address [" << peer << "] " << ec.message());

        // Connect failure notification.
        if (!handler(ec, nullptr))
        {
            // TODO: drop connector subscription.
            LOGS("Manual channel dropped at connect [" << peer << "].");
            return;
        }

        // Avoid tight loop with delay timer.
        defer(BIND(start_connect, _1, peer, connector, handler));
        return;
    }

    const auto channel = create_channel(socket, false);

    // It is possible for start_channel to directly invoke the handlers.
    start_channel(channel,
        BIND(handle_channel_start, _1, channel, peer, handler),
        BIND(handle_channel_stop, _1, channel, peer, connector, handler));
}

void session_manual::attach_handshake(const channel::ptr& channel,
    result_handler&& handler) NOEXCEPT
{
    session::attach_handshake(channel, std::move(handler));
}

void session_manual::handle_channel_start(const code& ec,
    const channel::ptr& channel, const endpoint& LOG_ONLY(peer),
    const channel_notifier& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    LOGS("Manual channel start [" << peer << "] " << ec.message());

    // Connection success notification.
    if (!ec && !handler(ec, channel))
        channel->stop(error::channel_dropped);
}

// Communication will begin after this function returns, freeing the thread.
void session_manual::attach_protocols(
    const channel::ptr& channel) NOEXCEPT
{
    session::attach_protocols(channel);
}

void session_manual::handle_channel_stop(const code& ec,
    const channel::ptr&, const endpoint& peer, const connector::ptr& connector,
    const channel_notifier& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // The channel stopped following connection, try again with delay.
    LOGS("Manual channel stop [" << peer << "] " << ec.message());

    // Handshake failure notification.
    if (ec == error::channel_dropped || !handler(ec, nullptr))
    {
        // TODO: drop connector subscription.
        LOGS("Manual channel dropped [" << peer << "].");
        return;
    }

    // Cannot be tight loop due to handshake.
    start_connect(error::success, peer, connector, handler);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
