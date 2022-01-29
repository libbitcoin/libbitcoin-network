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
#include <bitcoin/network/sessions/session.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session
#define NAME "session"

using namespace bc::system;
using namespace std::placeholders;

// protected
session::session(p2p& network)
  : stopped_(true),
    network_(network)
{
}

// protected
session::~session()
{
    BC_ASSERT_MSG(stopped(), "The session was not stopped.");
}

// Properties.
// ----------------------------------------------------------------------------

// protected
bool session::stopped() const
{
    return stopped_.load(std::memory_order_relaxed);
}

// protected
bool session::stopped(const code& ec) const
{
    return stopped() || ec == error::service_stopped;
}

// protected
bool session::blacklisted(const config::authority& authority) const
{
    return contains(network_.network_settings().blacklists, authority);
}

// protected
bool session::inbound() const
{
    // inbound session overrides.
    return false;
}

// protected
bool session::notify() const
{
    // seed session overrides.
    return true;
}

duration session::cycle_delay(const code& ec)
{
    return (ec == error::success ||
        ec == error::channel_timeout ||
        ec == error::service_stopped) ? seconds(0) :
            network_.network_settings().connect_timeout();
}

// Socket creators.
// ----------------------------------------------------------------------------

// protected
acceptor::ptr session::create_acceptor()
{
    return std::make_shared<acceptor>(network_.service(),
        network_.network_settings());
}

// protected
connector::ptr session::create_connector()
{
    return std::make_shared<connector>(network_.service(),
        network_.network_settings());
}

// Start sequence.
// ----------------------------------------------------------------------------
// These provide completion and early termination for seed session.

// public
void session::start(result_handler handler)
{
    if (!stopped())
    {
        handler(error::operation_failed);
        return;
    }

    stopped_.store(false, std::memory_order_relaxed);
    handler(error::success);
}

// public
void session::stop(const code&)
{
    stopped_.store(true, std::memory_order_relaxed);
}

// Registration start/stop sequence.
// ----------------------------------------------------------------------------

// protected
void session::start_channel(channel::ptr channel, result_handler started,
    result_handler stopped)
{
    if (this->stopped())
    {
        started(error::service_stopped);
        stopped(error::service_stopped);
        return;
    }

    channel->start();

    if (!inbound())
        network_.pend(channel->nonce());

    result_handler start = BIND4(handle_start, _1, channel, started, stopped);
    attach_handshake(channel, BIND3(handle_handshake, _1, channel, start));
}

// protected
void session::attach_handshake(channel::ptr channel, result_handler handshake)
{
    if (channel->negotiated_version() >= messages::level::bip61)
        attach<protocol_version_70002>(channel, network_)->start(handshake);
    else
        attach<protocol_version_31402>(channel, network_)->start(handshake);
}

// Privates.
// ----------------------------------------------------------------------------

// private
void session::handle_handshake(const code& ec, channel::ptr channel,
    result_handler start)
{
    if (!inbound())
        network_.unpend(channel->nonce());

    if (ec)
    {
        start(ec);
        return;
    }

    network_.store(channel, notify(), inbound(), start);
}

// private
void session::handle_start(const code& ec, channel::ptr channel,
    result_handler started, result_handler stopped)
{
    if (ec)
    {
        channel->stop(ec);
        stopped(ec);
    }
    else
    {
        channel->subscribe_stop(BIND3(handle_remove, _1, channel, stopped));
    }

    started(ec);
}

// private
void session::handle_remove(const code&, channel::ptr channel,
    result_handler stopped)
{
    network_.unstore(channel);
    stopped(error::success);
}

} // namespace network
} // namespace libbitcoin
