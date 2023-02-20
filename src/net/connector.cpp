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
    BC_ASSERT_MSG(!gate_.locked(), "connector is not stopped");
    if (gate_.locked()) { LOG("~connector is not stopped."); }
}

void connector::stop() NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (!gate_.locked())
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

    if (gate_.locked())
    {
        handler(error::operation_failed, nullptr);
        return;
    }

    // Capture the handler.
    gate_.lock(std::move(handler));

    // Create a socket.
    const auto sock = std::make_shared<socket>(log(), service_, host);

    // Posts handle_timer to strand.
    timer_->start(
        std::bind(&connector::handle_timer,
            shared_from_this(), _1, sock));

    // Posts handle_resolve to strand (async_resolve copies strings).
    resolver_.async_resolve(hostname, std::to_string(port),
        std::bind(&connector::handle_resolve,
            shared_from_this(), _1, _2, sock));
}

// private
void connector::handle_resolve(const error::boost_code& ec,
    const asio::endpoints& range, const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (socket->stopped())
    {
        gate_.knock(error::operation_canceled, nullptr);
        return;
    }

    if (ec)
    {
        timer_->stop();
        socket->stop();
        gate_.knock(error::asio_to_error_code(ec), nullptr);
        return;
    }

    // Establishes a socket connection by trying each endpoint in sequence.
    socket->connect(range,
        std::bind(&connector::do_handle_connect,
            shared_from_this(), _1, socket));
}

// private
void connector::do_handle_connect(const code& ec, const socket::ptr& socket) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&connector::handle_connect,
            shared_from_this(), ec, socket));
}

// private
void connector::handle_connect(const code& ec, const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (ec)
    {
        socket->stop();
        timer_->stop();
        gate_.knock(ec, nullptr);
        return;
    }

    timer_->stop();
    gate_.knock(error::success, socket);
}

// private
void connector::handle_timer(const code& ec, const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    resolver_.cancel();
    socket->stop();

    // Translate timer success to operation_timeout.
    gate_.knock(ec ? ec : error::operation_timeout, nullptr);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
