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
using namespace config;
using namespace std::placeholders;

session_manual::session_manual(p2p& network) noexcept
  : session(network)
{
}

bool session_manual::inbound() const noexcept
{
    return false;
}

bool session_manual::notify() const noexcept
{
    return true;
}

// Start/stop sequence.
// ----------------------------------------------------------------------------
// Manual connections are always enabled.

void session_manual::start(result_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
    session::start(BIND2(handle_started, _1, handler));
}

void session_manual::handle_started(const code& ec,
    result_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
    handler(ec);
}

// Connect sequence.
// ----------------------------------------------------------------------------

void session_manual::connect(const std::string& hostname,
    uint16_t port) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    const auto self = shared_from_base<session_manual>();
    connect(hostname, port, [=](const code&, channel::ptr)
    {
        // TODO: log discarded code.
        self->nop();
    });
}

void session_manual::connect(const std::string& hostname, uint16_t port,
    channel_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // BUGBUG: config::authority cons throws on invalid IP format, but this public.
    connect({ hostname, port }, handler);
}

void session_manual::connect(const authority& host,
    channel_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Create a connector for each manual connection.
    const auto connector = create_connector();

    stop_subscriber_->subscribe([=](const code&)
    {
        connector->stop();
    });

    start_connect(host, connector, handler);
}

// Connect cycle.
// ----------------------------------------------------------------------------

void session_manual::start_connect(const authority& host,
    connector::ptr connector, channel_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Terminates retry loops (and connector is restartable).
    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    connector->connect(host,
        BIND5(handle_connect, _1, _2, host, connector, handler));
}

void session_manual::handle_connect(const code& ec, channel::ptr channel,
    const authority& host, connector::ptr connector,
    channel_handler handler) noexcept
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
        timer_->start(BIND3(start_connect, host, connector, handler),
            settings().connect_timeout());
        return;
    }

    start_channel(channel,
        BIND4(handle_channel_start, _1, host, channel, handler),
        BIND4(handle_channel_stop, _1, host, connector, handler));
}

void session_manual::attach_handshake(const channel::ptr& channel,
    result_handler handler) const noexcept
{
    session::attach_handshake(channel, handler);
}

void session_manual::handle_channel_start(const code& ec,
    const authority&, channel::ptr channel, channel_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Notify upon each connection attempt.
    handler(ec, channel);
}

// Communication will begin after this function returns, freeing the thread.
void session_manual::attach_protocols(
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

void session_manual::handle_channel_stop(const code&, const authority& host,
    connector::ptr connector, channel_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // The channel stopped following connection, try again without delay.
    // This is the only opportunity for a tight loop (could use timer).
    start_connect(host, connector, handler);
}

} // namespace network
} // namespace libbitcoin
