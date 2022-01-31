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

#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
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

session::session(p2p& network)
  : stopped_(true),
    network_(network)
{
}

session::~session()
{
    BC_ASSERT_MSG(stopped(), "The session was not stopped.");
}

void session::start(result_handler handler)
{
    BC_ASSERT_MSG(network_.strand().running_in_this_thread(), "strand");

    if (!stopped())
    {
        handler(error::operation_failed);
        return;
    }

    stopped_.store(false, std::memory_order_relaxed);
    handler(error::success);
}

void session::stop()
{
    stopped_.store(true, std::memory_order_relaxed);
}

// Channel sequence.
// ----------------------------------------------------------------------------

void session::start_channel(channel::ptr channel, result_handler started,
    result_handler stopped)
{
    BC_ASSERT_MSG(network_.strand().running_in_this_thread(), "strand");

    if (session::stopped())
    {
        started(error::service_stopped);
        stopped(error::service_stopped);
        return;
    }

    channel->start();

    if (!inbound())
        network_.pend(channel->nonce());

    result_handler start = BIND4(handle_start, _1, channel, started, stopped);
    result_handler shake = BIND3(handle_handshake, _1, channel, std::move(start));

    // TODO: handshake bind to strand.
    boost::asio::post(channel->strand(),
        std::bind(&session::attach_handshake,
            shared_from_this(), channel, std::move(shake)));
}

void session::attach_handshake(channel::ptr channel, result_handler handshake)
{
    // Channel attach and start both require channel strand.
    BC_ASSERT_MSG(channel->stranded(), "channel: attach, start");

    if (network_.network_settings().protocol_maximum >= messages::level::bip61)
        channel->do_attach<protocol_version_70002>(network_)->start(handshake);
    else
        channel->do_attach<protocol_version_31402>(network_)->start(handshake);
}

void session::handle_handshake(const code& ec, channel::ptr channel,
    result_handler start)
{
    BC_ASSERT_MSG(network_.strand().running_in_this_thread(), "strand");

    if (!inbound())
        network_.unpend(channel->nonce());

    if (ec)
    {
        start(ec);
        return;
    }

    start(network_.store(channel, notify(), inbound()));
}

void session::handle_start(const code& ec, channel::ptr channel,
    result_handler started, result_handler stopped)
{
    if (ec)
    {
        channel->stop(ec);
        stopped(ec);
        started(ec);
        return;
    }

    // TODO: BIND3 bind to strand.
    channel->subscribe_stop(BIND3(handle_stop, _1, channel, stopped),
        std::move(started));
}

void session::handle_stop(const code& ec, channel::ptr channel,
    result_handler stopped)
{
    BC_ASSERT_MSG(network_.strand().running_in_this_thread(), "strand");

    network_.unstore(channel);
    stopped(ec);
}

// Properties.
// ----------------------------------------------------------------------------

bool session::stopped() const
{
    return stopped_.load(std::memory_order_relaxed);
}

bool session::stopped(const code& ec) const
{
    return stopped() || ec == error::service_stopped;
}

bool session::blacklisted(const config::authority& authority) const
{
    return contains(network_.network_settings().blacklists, authority);
}

////duration session::cycle_delay(const code& ec)
////{
////    return (ec == error::success ||
////        ec == error::channel_timeout ||
////        ec == error::service_stopped) ? seconds(0) :
////            network_.network_settings().connect_timeout();
////}

acceptor::ptr session::create_acceptor()
{
    return std::make_shared<acceptor>(network_.strand(), network_.service(),
        network_.network_settings());
}

connector::ptr session::create_connector()
{
    return std::make_shared<connector>(network_.strand(), network_.service(),
        network_.network_settings());
}

bool session::inbound() const
{
    return false;
}

bool session::notify() const
{
    return true;
}

} // namespace network
} // namespace libbitcoin
