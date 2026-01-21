/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include <memory>
#include <utility>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/deadline.hpp>
#include <bitcoin/network/net/socket.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {
    
// Bind throws (ok).
// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace system;
using namespace network::config;
using namespace std::placeholders;

// Construct.
// ----------------------------------------------------------------------------
// seed timeout uses same timer for connect, handshake and completion.

connector::connector(const logger& log, asio::strand& strand,
    asio::context& service, std::atomic_bool& suspended,
    const parameters& parameters) NOEXCEPT
  : strand_(strand),
    service_(service),
    suspended_(suspended),
    parameters_(parameters),
    resolver_(strand),
    timer_(emplace_shared<deadline>(log, strand, parameters.timeout)),
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
    BC_ASSERT(stranded());

    if (!racer_.running())
        return;

    // Posts timer handler to strand.
    // resolver and socket.connect are canceled in the timer handler.
    timer_->stop();
}

// Properties.
// ----------------------------------------------------------------------------

// protected/virtual
bool connector::proxied() const NOEXCEPT
{
    return false;
}

// protected
bool connector::stranded() NOEXCEPT
{
    return strand_.running_in_this_thread();
}

// Methods.
// ----------------------------------------------------------------------------

// This used by outbound (address from pool).
void connector::connect(const address& address,
    socket_handler&& handler) NOEXCEPT
{
    // Address is fully set (with metadata), endpoint is numeric.
    start(address.to_host(), address.port(), address, address,
        std::move(handler));
}

// This used by seed, manual, and socks5 (endpoint from config).
void connector::connect(const endpoint& endpoint,
    socket_handler&& handler) NOEXCEPT
{
    // Address is not set (defaulted), endpoint is numeric or qualified name.
    start(endpoint.host(), endpoint.port(), {}, endpoint, std::move(handler));
}

// protected
void connector::start(const std::string& hostname, uint16_t port,
    const config::address& address, const config::endpoint& endpoint,
    socket_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

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

    // Create the outbound socket and shared finish context.
    const auto finish = std::make_shared<bool>(false);
    const auto socket = std::make_shared<network::socket>(log, service_,
        parameters_, address, endpoint, proxied());

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
void connector::handle_resolve(const boost_code& ec,
    const asio::endpoints& range, const finish_ptr& finish,
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT(stranded());

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
    BC_ASSERT(socket->stranded());

    boost::asio::post(strand_,
        std::bind(&connector::handle_connect,
            shared_from_this(), ec, finish, socket));
}

// protected/virtual
void connector::handle_connect(const code& ec, const finish_ptr& finish,
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT(stranded());

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

// protected/virtual
void connector::handle_timer(const code& ec, const finish_ptr& finish,
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (*finish)
    {
        racer_.finish(error::operation_canceled, nullptr);
        return;
    }

    // Either cancellation or timer failure (unknown which has won).
    // Stopped socket returned with failure code for option of host recovery.
    if (ec)
    {
        // Socket stop is thread safe, dispatching to its own strand.
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
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
