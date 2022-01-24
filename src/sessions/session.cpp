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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session
#define NAME "session"
    
using namespace bc::system;
using namespace std::placeholders;

session::session(p2p& network, bool notify_on_connect)
  : stopped_(true),
    notify_on_connect_(notify_on_connect),
    network_(network),
    settings_(network.network_settings())
{
}

session::~session()
{
    BC_ASSERT_MSG(stopped(), "The session was not stopped.");
}

// Properties.
// ----------------------------------------------------------------------------
// protected

size_t session::address_count() const
{
    return {}; ////network_.address_count();
}

size_t session::connection_count() const
{
    return {}; ////network_.connection_count();
}

code session::fetch_address(messages::address_item& out_address) const
{
    return {}; ////network_.fetch_address(out_address);
}

bool session::blacklisted(const authority& authority) const
{
    const auto ip_compare = [&](const config::authority& blocked)
    {
        return authority.ip() == blocked.ip();
    };

    const auto& list = settings_.blacklists;
    return std::any_of(list.begin(), list.end(), ip_compare);
}

bool session::stopped() const
{
    return stopped_;
}

// TODO: clean this up.
bool session::stopped(const code& ec) const
{
    return stopped() || ec == error::service_stopped;
}

// Socket creators.
// ----------------------------------------------------------------------------

acceptor::ptr session::create_acceptor()
{
    return std::make_shared<acceptor>(network_.service(), settings_);
}

connector::ptr session::create_connector()
{
    return std::make_shared<connector>(network_.service(), settings_);
}

// Pending connect.
// ----------------------------------------------------------------------------

code session::pend(connector::ptr connector)
{
    return {}; ////network_.pend(connector);
}

void session::unpend(connector::ptr connector)
{
    return; ////network_.unpend(connector);
}

// Pending handshake.
// ----------------------------------------------------------------------------

code session::pend(channel::ptr channel)
{
    return {}; ////return network_.pend(channel);
}

void session::unpend(channel::ptr channel)
{
    return; ////network_.unpend(channel);
}

bool session::pending(uint64_t version_nonce) const
{
    return {}; ////return network_.pending(version_nonce);
}

// Start sequence.
// ----------------------------------------------------------------------------

void session::start(result_handler handler)
{
    if (!stopped())
    {
        handler(error::operation_failed);
        return;
    }

    stopped_ = false;

    // This is the end of the start sequence.
    handler(error::success);
}

// TODO: override to log session stop.
void session::stop(const code&)
{
    stopped_ = true;
}

// Registration sequence.
// ----------------------------------------------------------------------------
// Must not change context in start or stop sequences (use bind).

void session::register_channel(channel::ptr channel,
    result_handler handle_started, result_handler handle_stopped)
{
    if (stopped())
    {
        handle_started(error::service_stopped);
        handle_stopped(error::service_stopped);
        return;
    }

    start_channel(channel,
        BIND4(handle_start, _1, channel, handle_started, handle_stopped));
}

void session::start_channel(channel::ptr channel,
    result_handler handle_started)
{
    channel->set_notify(notify_on_connect_);
    channel->set_nonce(pseudo_random::next<uint64_t>(1u, max_uint64));
    channel->start();

    attach_handshake_protocols(channel,
        BIND3(handle_handshake, _1, channel, handle_started));
}

// THIS IS INVOKED ON THE CHANNEL THREAD.
// Channel communication begins after version protocol start returns.
// handle_handshake is invoked after version negotiation completes.
void session::attach_handshake_protocols(channel::ptr channel,
    result_handler handle_started)
{
    // Reject messages are not handled until bip61 (70002).
    // The negotiated_version is initialized to the configured maximum.
    if (channel->negotiated_version() >= messages::level::bip61)
        attach<protocol_version_70002>(channel, network_)->
            start(handle_started);
    else
        attach<protocol_version_31402>(channel, network_)->
            start(handle_started);
}

// THIS IS INVOKED ON THE CHANNEL THREAD.
void session::handle_handshake(const code& ec, channel::ptr channel,
    result_handler handle_started)
{
    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure in handshake with [" << channel->authority()
            << "] " << ec.message();

        handle_started(ec);
        return;
    }

    handshake_complete(channel, handle_started);
}

// THIS IS INVOKED ON THE CHANNEL THREAD.
void session::handshake_complete(channel::ptr channel,
    result_handler handle_started)
{
    // TODO: subscribe to message broadcaster?
    // This will fail if the IP address or nonce is already connected.
////handle_started(network_.store(channel));
}

// THIS IS INVOKED ON THE CHANNEL THREAD.
void session::handle_start(const code& ec, channel::ptr channel,
    result_handler handle_started, result_handler handle_stopped)
{
    // Must either stop or subscribe the channel for stop before returning.
    // All closures must eventually be invoked as otherwise it is a leak.
    // Therefore upon start failure expect start failure and stop callbacks.
    if (ec)
    {
        channel->stop(ec);
        handle_stopped(ec);
    }
    else
    {
        // NOTE: protocols are also subscribed to channel stop.
        // This is where all channels subscribe to their own stop notification.
        channel->subscribe_stop(
            BIND3(handle_remove, _1, channel, handle_stopped));
    }

    // This is the end of the registration sequence.
    handle_started(ec);
}

// THIS IS INVOKED ON THE CHANNEL THREAD (if the channel stops itself).
void session::handle_remove(const code& , channel::ptr channel,
    result_handler handle_stopped)
{
////network_.remove(channel);
    handle_stopped(error::success);
}

} // namespace network
} // namespace libbitcoin
