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
#include <bitcoin/network/net/connector.hpp>

#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/net/socket.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace system;
using namespace network::config;
using namespace std::placeholders;

// Construct.
// ----------------------------------------------------------------------------

connector::connector(const logger& log, asio::strand& strand,
    asio::io_context& service, const settings& settings) NOEXCEPT
  : settings_(settings),
    service_(service),
    strand_(strand),
    resolver_(strand),
    timer_(std::make_shared<deadline>(log, strand, settings.connect_timeout())),
    reporter(log),
    tracker<connector>(log)
{
}

connector::~connector() NOEXCEPT
{
    BC_ASSERT_MSG(stopped_, "connector is not stopped");
    if (!stopped_) { LOG("~connector is not stopped."); }
}

void connector::stop() NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return;

    // Posts timer handler to strand.
    // resolver and socket.connect are canceled in the timer handler.
    timer_->stop();
}

// Methods.
// ----------------------------------------------------------------------------

void connector::connect(const address& host,
    socket_handler&& handler) NOEXCEPT
{
    start(host.to_host(), host.port(), host, std::move(handler));
}

void connector::connect(const authority& host,
    socket_handler&& handler) NOEXCEPT
{
    start(host.to_host(), host.port(), host.to_address_item(),
        std::move(handler));
}

void connector::connect(const endpoint& host,
    socket_handler&& handler) NOEXCEPT
{
    start(host.host(), host.port(), host.to_address(), std::move(handler));
}

// protected
void connector::start(const std::string& hostname, uint16_t port,
    const config::address& host, socket_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (!stopped_)
    {
        handler(error::operation_failed, nullptr);
        return;
    }

    // This allows connect after stop (restartable).
    stopped_ = false;

    // Create the socket.
    const auto sock = std::make_shared<socket>(log(), service_, host);

    // Posts handle_timer to strand (handler copied).
    timer_->start(
        std::bind(&connector::handle_timer,
            shared_from_this(), _1, sock, handler));

    // Posts handle_resolve to strand (async_resolve copies strings).
    resolver_.async_resolve(hostname, std::to_string(port),
        std::bind(&connector::handle_resolve,
            shared_from_this(), _1, _2, sock, std::move(handler)));
}

// private
void connector::handle_resolve(const error::boost_code& ec,
    const asio::endpoints& range, socket::ptr socket,
    const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
    {
        socket->stop();
        return;
    }

    if (ec)
    {
        stopped_ = true;
        timer_->stop();
        socket->stop();
        handler(error::asio_to_error_code(ec), nullptr);
        return;
    }

    // Establishes a socket connection by trying each endpoint in sequence.
    socket->connect(range,
        std::bind(&connector::handle_connect,
            shared_from_this(), _1, socket, handler));
}

// private
void connector::handle_connect(const code& ec, socket::ptr socket,
    const socket_handler& handler) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&connector::do_handle_connect,
            shared_from_this(), ec, socket, handler));
}

// private
void connector::do_handle_connect(const code& ec, socket::ptr socket,
    const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return;

    stopped_ = true;
    timer_->stop();

    if (ec)
    {
        socket->stop();
        handler(ec, nullptr);
        return;
    }

    // Successful connect.
    handler(error::success, socket);
}

// private
void connector::handle_timer(const code& ec, const socket::ptr& socket,
    const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return;

    stopped_ = true;
    socket->stop();
    resolver_.cancel();

    if (ec)
    {
        // Stop result code (error::operation_canceled) return here.
        handler(ec, nullptr);
        return;
    }

    // Timeout result code (error::success) translated to channel_timeout here.
    handler(error::channel_timeout, nullptr);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
