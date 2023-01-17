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
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_manual

using namespace bc::system;
using namespace config;
using namespace std::placeholders;

session_manual::session_manual(p2p& network) NOEXCEPT
  : session(network), track<session_manual>(network.log())
{
}

bool session_manual::inbound() const NOEXCEPT
{
    return false;
}

bool session_manual::notify() const NOEXCEPT
{
    return true;
}

// Start/stop sequence.
// ----------------------------------------------------------------------------
// Manual connections are always enabled.

void session_manual::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    session::start(BIND2(handle_started, _1, std::move(handler)));
}

void session_manual::handle_started(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    handler(ec);
}

// Connect sequence.
// ----------------------------------------------------------------------------

////void session_manual::connect(const config::authority& peer,
////    channel_handler&& handler) NOEXCEPT
////{
////    BC_ASSERT_MSG(stranded(), "strand");
////
////    connect(endpoint{ peer.to_hostname(), peer.port() }, std::move(handler));
////}

void session_manual::connect(const config::endpoint& peer) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    const auto self = shared_from_base<session_manual>();
    connect(peer, [=](const code&, channel::ptr) NOEXCEPT
    {
        // TODO: log discarded code.
        self->nop();
    });
}

void session_manual::connect(const config::endpoint& peer,
    channel_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Create a connector for each manual connection.
    const auto connector = create_connector();

    subscribe_stop([=](const code&) NOEXCEPT
    {
        connector->stop();
    });

    start_connect(peer, connector, std::move(handler));
}

// Connect cycle.
// ----------------------------------------------------------------------------

void session_manual::start_connect(const endpoint& peer,
    const connector::ptr& connector, const channel_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Terminates retry loops (and connector is restartable).
    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    connector->connect(peer,
        BIND5(handle_connect, _1, _2, peer, connector, handler));
}

void session_manual::handle_connect(const code& ec, const channel::ptr& channel,
    const endpoint& peer, const connector::ptr& connector,
    const channel_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Guard restartable timer (shutdown delay).
    if (stopped())
    {
        if (channel)
            channel->stop(error::service_stopped);

        handler(error::service_stopped, nullptr);
        return;
    }

    // TODO: log discarded code.
    // There was an error connecting the channel, so try again after delay.
    if (ec)
    {
        BC_ASSERT_MSG(!channel, "unexpected channel instance");
        start_timer(BIND3(start_connect, peer, connector, handler),
            settings().connect_timeout());
        return;
    }

    start_channel(channel,
        BIND4(handle_channel_start, _1, peer, channel, handler),
        BIND4(handle_channel_stop, _1, peer, connector, handler));
}

void session_manual::attach_handshake(const channel::ptr& channel,
    result_handler&& handler) const NOEXCEPT
{
    session::attach_handshake(channel, std::move(handler));
}

void session_manual::handle_channel_start(const code& ec,
    const endpoint&, const channel::ptr& channel,
    const channel_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Notify upon each connection attempt.
    handler(ec, channel);
}

// Communication will begin after this function returns, freeing the thread.
void session_manual::attach_protocols(
    const channel::ptr& channel) const NOEXCEPT
{
    session::attach_protocols(channel);
}

void session_manual::handle_channel_stop(const code&, const endpoint& peer,
    const connector::ptr& connector, const channel_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // The channel stopped following connection, try again without delay.
    // This is the only opportunity for a tight loop (could use timer).
    start_connect(peer, connector, move_copy(handler));
}

} // namespace network
} // namespace libbitcoin
