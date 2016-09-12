/**
 * Copyright (c) 2011-2016 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/network/sessions/session.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/acceptor.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/connector.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/proxy.hpp>
#include <bitcoin/network/protocols/protocol_version_31402.hpp>
#include <bitcoin/network/protocols/protocol_version_70002.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define NAME "session"

// Base class binders.
#define BIND_0(method) \
    base_bind(&session::method)
#define BIND_1(method, p1) \
    base_bind(&session::method, p1)
#define BIND_2(method, p1, p2) \
    base_bind(&session::method, p1, p2)
#define BIND_3(method, p1, p2, p3) \
    base_bind(&session::method, p1, p2, p3)
#define BIND_4(method, p1, p2, p3, p4) \
    base_bind(&session::method, p1, p2, p3, p4)

using namespace std::placeholders;

session::session(p2p& network, bool notify_on_connect)
  : stopped_(true),
    notify_on_connect_(notify_on_connect),
    network_(network),
    settings_(network.network_settings()),
    pool_(network.thread_pool()),
    dispatch_(pool_, NAME)
{
}

session::~session()
{
    BITCOIN_ASSERT_MSG(stopped(), "The session was not stopped.");
}

// Properties.
// ----------------------------------------------------------------------------

// protected:
void session::address_count(count_handler handler)
{
    network_.address_count(handler);
}

// protected:
void session::fetch_address(host_handler handler)
{
    network_.fetch_address(handler);
}

// protected:
void session::connection_count(count_handler handler)
{
    network_.connected_count(handler);
}

// protected:
bool session::blacklisted(const authority& authority) const
{
    const auto& blocked = settings_.blacklists;
    const auto it = std::find(blocked.begin(), blocked.end(), authority);
    return it != blocked.end();
}

// Socket creators.
// ----------------------------------------------------------------------------
// Must not change context in the stop handlers (must use bind).

// protected:
acceptor::ptr session::create_acceptor()
{
    const auto accept = std::make_shared<acceptor>(pool_, settings_);
    subscribe_stop(BIND_2(do_stop_acceptor, _1, accept));
    return accept;
}

void session::do_stop_acceptor(const code&, acceptor::ptr accept)
{
    accept->stop();
}

// protected:
connector::ptr session::create_connector()
{
    const auto connect = std::make_shared<connector>(pool_, settings_);
    subscribe_stop(BIND_2(do_stop_connector, _1, connect));
    return connect;
}

void session::do_stop_connector(const code&, connector::ptr connect)
{
    connect->stop();
}

// Pending connections collection.
// ----------------------------------------------------------------------------

void session::pend(channel::ptr channel, result_handler handler)
{
    network_.pend(channel, handler);
}

void session::unpend(channel::ptr channel, result_handler handler)
{
    network_.unpend(channel, handler);
}

void session::pending(uint64_t version_nonce, truth_handler handler) const
{
    network_.pending(version_nonce, handler);
}

// Start sequence.
// ----------------------------------------------------------------------------
// Must not change context before subscribing.

void session::start(result_handler handler)
{
    if (!stopped())
    {
        handler(error::operation_failed);
        return;
    }

    stopped_ = false;
    subscribe_stop(BIND_1(do_stop_session, _1));

    // This is the end of the start sequence.
    handler(error::success);
}

void session::do_stop_session(const code&)
{
    // This signals the session to stop creating connections, but does not
    // close the session. Channels stop, resulting in session scope loss.
    stopped_ = true;
}

bool session::stopped() const
{
    return stopped_;
}

// Subscribe Stop sequence.
// ----------------------------------------------------------------------------

void session::subscribe_stop(result_handler handler)
{
    network_.subscribe_stop(handler);
}

// Registration sequence.
// ----------------------------------------------------------------------------
// Must not change context in start or stop sequences (use bind).

// protected:
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
        BIND_4(handle_start, _1, channel, handle_started, handle_stopped));
}

// protected:
void session::start_channel(channel::ptr channel,
    result_handler handle_started)
{
    channel->set_notify(notify_on_connect_);
    channel->set_nonce(nonzero_pseudo_random());

    // The channel starts, invokes the handler, then starts the read cycle.
    channel->start(
        BIND_3(handle_starting, _1, channel, handle_started));
}

void session::handle_starting(const code& ec, channel::ptr channel,
    result_handler handle_started)
{
    if (ec)
    {
        log::info(LOG_NETWORK)
            << "Channel failed to start [" << channel->authority() << "] "
            << ec.message();
        handle_started(ec);
        return;
    }

    attach_handshake_protocols(channel,
        BIND_3(handle_handshake, _1, channel, handle_started));
}

// protected:
void session::attach_handshake_protocols(channel::ptr channel,
    result_handler handle_started)
{
    // The negotiated_version is initialized to the configured maximum.
    if (channel->negotiated_version() >= message::version::level::bip61)
        attach<protocol_version_70002>(channel)->start(handle_started);
    else
        attach<protocol_version_31402>(channel)->start(handle_started);
}

void session::handle_handshake(const code& ec, channel::ptr channel,
    result_handler handle_started)
{
    if (ec)
    {
        log::debug(LOG_NETWORK)
            << "Failure in handshake with [" << channel->authority()
            << "] " << ec.message();

        handle_started(ec);
        return;
    }

    // This will fail if the IP address or nonce is already connected.
    network_.store(channel, handle_started);
}

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
        channel->subscribe_stop(
            BIND_3(do_remove, _1, channel, handle_stopped));
    }

    // This is the end of the registration sequence.
    handle_started(ec);
}

void session::do_remove(const code& ec, channel::ptr channel,
    result_handler handle_stopped)
{
    network_.remove(channel, BIND_2(handle_remove, _1, channel));
    handle_stopped(ec);
}

void session::handle_remove(const code& ec, channel::ptr channel)
{
    if (ec)
        log::debug(LOG_NETWORK)
            << "Failed to remove channel [" << channel->authority() << "] "
            << ec.message();
}

} // namespace network
} // namespace libbitcoin
