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

bool session::stopped() const
{
    return stopped_;
}

bool session::stopped(const code& ec) const
{
    return stopped() || ec == error::service_stopped;
}

bool session::blacklisted(const authority& authority) const
{
    return contains(settings_.blacklists, authority);
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

// override to log session stop.
void session::stop(const code&)
{
    // This is entirely passive.
    stopped_ = true;
}

// Registration start/stop sequence.
// ----------------------------------------------------------------------------
// Must not change context in start or stop sequences (use bind).

void session::register_channel(channel::ptr channel,
    result_handler handle_started1, result_handler handle_stopped1)
{
    if (stopped())
    {
        handle_started1(error::service_stopped);
        handle_stopped1(error::service_stopped);
        return;
    }

    // must set nonce before start_channel
    channel->set_nonce(pseudo_random::next<uint64_t>(1, max_uint64));

    start_channel(channel,
        BIND4(handle_start, _1, channel, handle_started1, handle_stopped1));
}

void session::start_channel(channel::ptr channel,
    result_handler handle_started2)
{
    channel->start();

    attach_handshake_protocols(channel,
        BIND3(handle_handshake, _1, channel, handle_started2));
}

// Channel communication begins after version protocol start returns.
// handle_handshake is invoked after version negotiation completes.
void session::attach_handshake_protocols(channel::ptr channel,
    result_handler handle_started3)
{
    // Reject messages are not handled until bip61 (70002).
    // The negotiated_version is initialized to the configured maximum.
    if (channel->negotiated_version() >= messages::level::bip61)
        attach<protocol_version_70002>(channel, network_)->
            start(handle_started3);
    else
        attach<protocol_version_31402>(channel, network_)->
            start(handle_started3);
}

void session::handle_handshake(const code& ec, channel::ptr channel,
    result_handler handle_started2)
{
    if (ec)
    {
        handle_started2(ec);
        return;
    }

    handshake_complete(channel, handle_started2);
}

void session::handshake_complete(channel::ptr channel,
    result_handler handle_started2)
{
    // This will fail if the IP address is already connected.
    network_.store(channel, notify_on_connect_, false, handle_started2);
}

void session::handle_start(const code& ec, channel::ptr channel,
    result_handler handle_started1, result_handler handle_stopped1)
{
    if (ec)
    {
        channel->stop(ec);
        handle_stopped1(ec);
    }
    else
    {
        // Started, and eventual stop will cause removal.
        channel->subscribe_stop(
            BIND3(handle_remove, _1, channel, handle_stopped1));
    }

    // This is the end of the registration start sequence.
    handle_started1(ec);
}

void session::handle_remove(const code&, channel::ptr channel,
    result_handler handle_stopped1)
{
    network_.unstore(channel);

    // This is the end of the registration stop sequence.
    handle_stopped1(error::success);
}

} // namespace network
} // namespace libbitcoin
