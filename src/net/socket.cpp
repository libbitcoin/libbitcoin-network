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
#include <bitcoin/network/net/socket.hpp>

#include <utility>
#include <variant>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/deadline.hpp>

// Boost: "The execution context provides the I/O executor that the socket will
// use, by default, to dispatch handlers for any asynchronous operations
// performed on the socket." Calls are stranded to protect the socket member.

// Boost async functions are NOT THREAD SAFE for the same socket object.
// This clarifies boost documentation: svn.boost.org/trac10/ticket/10009

namespace libbitcoin {
namespace network {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Construct.
// ----------------------------------------------------------------------------

socket::socket(const logger& log, asio::context& service,
    const parameters& parameters) NOEXCEPT
  : socket(log, service, parameters, {}, {}, false, true)
{
}

socket::socket(const logger& log, asio::context& service,
    const parameters& params, const config::address& address,
    const config::endpoint& endpoint, bool proxied) NOEXCEPT
  : socket(log, service, params, address, endpoint, proxied, false)
{
}

// protected
socket::socket(const logger& log, asio::context& service,
    const parameters& params, const config::address& address,
    const config::endpoint& endpoint, bool proxied, bool inbound) NOEXCEPT
  : inbound_(inbound),
    proxied_(proxied),
    maximum_(params.maximum_request),
    strand_(service.get_executor()),
    service_(service),
    context_(params.context),
    address_(address),
    endpoint_(endpoint),
    timer_(emplace_shared<deadline>(log, strand_, params.timeout)),
    socket_(std::in_place_type<asio::socket>, strand_),
    reporter(log),
    tracker<socket>(log)
{
}

socket::~socket() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "socket is not stopped");
    if (!stopped_.load()) { LOGF("~socket is not stopped."); }
}

// Properties.
// ----------------------------------------------------------------------------

const config::address& socket::address() const NOEXCEPT
{
    return address_;
}

const config::endpoint& socket::endpoint() const NOEXCEPT
{
    return endpoint_;
}

bool socket::inbound() const NOEXCEPT
{
    return inbound_;
}

bool socket::stopped() const NOEXCEPT
{
    return stopped_.load();
}

bool socket::stranded() const NOEXCEPT
{
    return strand_.running_in_this_thread();
}

asio::strand& socket::strand() NOEXCEPT
{
    return strand_;
}

asio::context& socket::service() const NOEXCEPT
{
    return service_;
}

// Context.
// ----------------------------------------------------------------------------
// protected

bool socket::secure() const NOEXCEPT
{
    // TODO: p2p::context, zmq::context.
    return std::holds_alternative<ref<asio::ssl::context>>(context_);
}

bool socket::is_secure() const NOEXCEPT
{
    BC_ASSERT(stranded());
    return std::holds_alternative<asio::ssl::socket>(socket_) ||
        std::holds_alternative<ws::ssl::socket>(socket_);
}

bool socket::is_websocket() const NOEXCEPT
{
    BC_ASSERT(stranded());
    return std::holds_alternative<ws::socket>(socket_) ||
        std::holds_alternative<ws::ssl::socket>(socket_);
}

bool socket::is_base() const NOEXCEPT
{
    BC_ASSERT(stranded());
    return std::holds_alternative<asio::socket>(socket_);
}

// Variant accessors.
// ----------------------------------------------------------------------------
// protected

socket::ws_t socket::get_ws() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(is_websocket());

    return std::visit(overload
    {
        [](asio::socket&) NOEXCEPT -> socket::ws_t
        {
            std::terminate();
        },
        [](asio::ssl::socket&) NOEXCEPT -> socket::ws_t
        {
            std::terminate();
        },
        [](ws::socket& value) NOEXCEPT -> socket::ws_t
        {
            return std::ref(value);
        },
        [](ws::ssl::socket& value) NOEXCEPT -> socket::ws_t
        {
            return std::ref(value);
        }
    }, socket_);
}

socket::tcp_t socket::get_tcp() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(!is_websocket());

    return std::visit(overload
    {
        [](asio::socket& value) NOEXCEPT -> socket::tcp_t
        {
            return std::ref(value);
        },
        [](asio::ssl::socket& value) NOEXCEPT -> socket::tcp_t
        {
            return std::ref(value);
        },
        [](ws::socket&) NOEXCEPT -> socket::tcp_t
        {
            std::terminate();
        },
        [](ws::ssl::socket&) NOEXCEPT -> socket::tcp_t
        {
            std::terminate();
        }
    }, socket_);
}

asio::socket& socket::get_base() NOEXCEPT
{
    BC_ASSERT(stranded());

    return std::visit(overload
    {
        [](asio::socket& value) NOEXCEPT -> asio::socket&
        {
            return value;
        },
        [](asio::ssl::socket& value) NOEXCEPT -> asio::socket&
        {
            return boost::beast::get_lowest_layer(value);
        },
        [](ws::socket& value) NOEXCEPT -> asio::socket&
        {
            return boost::beast::get_lowest_layer(value);
        },
        [](ws::ssl::socket& value) NOEXCEPT -> asio::socket&
        {
            return boost::beast::get_lowest_layer(value);
        }
    }, socket_);
}

asio::ssl::socket& socket::get_ssl() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(is_secure());

    return std::visit(overload
    {
        [](asio::socket&) NOEXCEPT -> asio::ssl::socket&
        {
            std::terminate();
        },
        [](asio::ssl::socket& value) NOEXCEPT -> asio::ssl::socket&
        {
            return value;
        },
        [](ws::socket&) NOEXCEPT -> asio::ssl::socket&
        {
            std::terminate();
        },
        [](ws::ssl::socket& value) NOEXCEPT -> asio::ssl::socket&
        {
            return value.next_layer();
        }
    }, socket_);
}

// Logging.
// ----------------------------------------------------------------------------
// private

void socket::logx(const std::string& context,
    const boost_code& ec) const NOEXCEPT
{
    LOGX("Socket " << context << " error (" << ec.value() << ") "
        << ec.category().name() << " : " << ec.message());
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
