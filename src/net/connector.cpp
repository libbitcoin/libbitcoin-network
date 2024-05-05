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
#include <bitcoin/network/net/connector.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/deadline.hpp>
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
    asio::io_context& service, const settings& settings,
    std::atomic_bool& suspended) NOEXCEPT
  : settings_(settings),
    service_(service),
    strand_(strand),
    suspended_(suspended),
    resolver_(strand),
    timer_(std::make_shared<deadline>(log, strand, settings.connect_timeout())),
    reporter(log),
    tracker<connector>(log)
{
}

connector::~connector() NOEXCEPT
{
    BC_ASSERT_MSG(!racer_.running(), "connector is not stopped");
    if (racer_.running()) { LOGF("~connector is not stopped."); }
}

void connector::stop() NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (!racer_.running())
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

    if (racer_.running())
    {
        handler(error::operation_failed, nullptr);
        return;
    }

    if (suspended_.load())
    {
        handler(error::service_suspended, nullptr);
        return;
    }

    // Capture the handler.
    racer_.start(std::move(handler));

    // Create a socket and shared finish context.
    const auto finish = std::make_shared<bool>(false);
    const auto socket = std::make_shared<network::socket>(log, service_,
        host);

    // Posts handle_timer to strand.
    timer_->start(
        std::bind(&connector::handle_timer,
            shared_from_this(), _1, finish, socket));

    // Posts handle_resolve to strand (async_resolve copies strings).
    resolver_.async_resolve(hostname, std::to_string(port),
        std::bind(&connector::handle_resolve,
            shared_from_this(), _1, _2, finish, socket));
}

// private
void connector::handle_resolve(const error::boost_code& ec,
    const asio::endpoints& range, const finish_ptr& finish,
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // Timer stopped the socket, it wins (with timeout/failure).
    if (socket->stopped())
    {
        racer_.finish(error::operation_canceled, nullptr);
        return;
    }

    if (suspended_.load())
    {
        timer_->stop();
        socket->stop();
        racer_.finish(error::service_suspended, nullptr);
        return;
    }

    // Failure in resolve, it wins (with resolve failure).
    if (ec)
    {
        timer_->stop();
        socket->stop();
        racer_.finish(error::asio_to_error_code(ec), nullptr);
        return;
    }

    // Posts do_handle_connect to the socket's strand.
    // Establishes a socket connection by trying each endpoint in sequence.
    socket->connect(range,
        std::bind(&connector::do_handle_connect,
            shared_from_this(), _1, finish, socket));
}

// private
void connector::do_handle_connect(const code& ec, const finish_ptr& finish,
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT_MSG(socket->stranded(), "strand");

    boost::asio::post(strand_,
        std::bind(&connector::handle_connect,
            shared_from_this(), ec, finish, socket));
}

// private
void connector::handle_connect(const code& ec, const finish_ptr& finish,
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // Timer stopped the socket, it wins (with timeout/failure).
    if (socket->stopped())
    {
        racer_.finish(error::operation_canceled, nullptr);
        return;
    }

    if (suspended_.load())
    {
        socket->stop();
        timer_->stop();
        racer_.finish(error::service_suspended, nullptr);
        return;
    }

    // Failure in connect, connector wins (with connect failure).
    if (ec)
    {
        socket->stop();
        timer_->stop();
        racer_.finish(ec, nullptr);
        return;
    }

    // Successful connect (error::success), inform and cancel timer.
    *finish = true;
    timer_->stop();
    racer_.finish(error::success, socket);
}

// private
void connector::handle_timer(const code& ec, const finish_ptr& finish,
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // Successful connect, connector wins (error::success).
    if (*finish)
    {
        racer_.finish(error::operation_canceled, nullptr);
        return;
    }

    // Either cancellation or timer failure (unknown which has won).
    // Stopped socket returned with failure code for option of host recovery.
    if (ec)
    {
        socket->stop();
        resolver_.cancel();
        racer_.finish(ec, socket);
        return;
    }

    // Timeout before connect, timer wins, cancel resolver/connector.
    // Timer fires with error::success, change to error::operation_timeout.
    // Stopped socket returned with failure code for option of host recovery.
    socket->stop();
    resolver_.cancel();
    racer_.finish(error::operation_timeout, socket);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
