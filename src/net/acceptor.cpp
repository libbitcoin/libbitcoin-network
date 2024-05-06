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
#include <bitcoin/network/net/acceptor.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/channel.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace system;
using namespace std::placeholders;

inline asio::endpoint make_endpoint(bool enable_ipv6, uint16_t port) NOEXCEPT
{
    return { enable_ipv6 ? asio::tcp::v6() : asio::tcp::v4(), port };
}

// Construct.
// ----------------------------------------------------------------------------
// Boost: "The io_context object that the acceptor will use to dispatch
// handlers for any asynchronous operations performed on the acceptor."
// Calls are stranded to protect the acceptor member.

acceptor::acceptor(const logger& log, asio::strand& strand,
    asio::io_context& service, const settings& settings,
    std::atomic_bool& suspended) NOEXCEPT
  : settings_(settings),
    service_(service),
    strand_(strand),
    suspended_(suspended),
    acceptor_(strand_),
    reporter(log),
    tracker<acceptor>(log)
{
}

acceptor::~acceptor() NOEXCEPT
{
    BC_ASSERT_MSG(stopped_, "acceptor is not stopped");
    if (!stopped_) { LOGF("~acceptor is not stopped."); }
}

// Start/stop.
// ----------------------------------------------------------------------------

code acceptor::start(uint16_t port) NOEXCEPT
{
    const auto ipv6 = settings_.enable_ipv6;
    const auto protocol = ipv6 ? asio::tcp::v6() : asio::tcp::v4();
    return start({ protocol, port });
}

code acceptor::start(const config::authority& local) NOEXCEPT
{
    return start(local.to_endpoint());
}

// protected
code acceptor::start(const asio::endpoint& point) NOEXCEPT
{
    if (!stopped_)
        return error::operation_failed;

    error::boost_code ec;
    const auto ipv6 = settings_.enable_ipv6;

    // Open the socket.
    acceptor_.open(point.protocol(), ec);

    // When ipv6 is enabled also enable ipv4.
    if (!ec && ipv6)
        acceptor_.set_option(asio::v6_only(false), ec);

    if (!ec)
        acceptor_.set_option(asio::reuse_address(true), ec);

    if (!ec)
        acceptor_.bind(point, ec);

    if (!ec)
        acceptor_.listen(asio::max_connections, ec);

    if (!ec)
    {
        stopped_ = false;
    }
    else
    {
        error::boost_code ignore;
        acceptor_.cancel(ignore);
    }

    return error::asio_to_error_code(ec);
}

void acceptor::stop() NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // Posts handle_accept to strand (if not already posted).
    error::boost_code ignore;
    acceptor_.cancel(ignore);
    stopped_ = true;
}

// Properties.
// ----------------------------------------------------------------------------

config::authority acceptor::local() const NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    return { stopped_ ? asio::endpoint{} : acceptor_.local_endpoint() };
}

// Methods.
// ----------------------------------------------------------------------------

void acceptor::accept(socket_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    if (suspended_.load())
    {
        handler(error::service_suspended, nullptr);
        return;
    }

    // Create the socket.
    const auto socket = std::make_shared<network::socket>(log, service_);

    // Posts handle_accept to the acceptor's strand.
    // Establishes a socket connection by waiting on the socket.
    socket->accept(acceptor_,
        std::bind(&acceptor::handle_accept,
            shared_from_this(), _1, socket, std::move(handler)));
}

// private
void acceptor::handle_accept(const code& ec, const socket::ptr& socket,
    const socket_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (ec)
    {
        socket->stop();
        handler(ec, nullptr);
        return;
    }

    if (suspended_.load())
    {
        socket->stop();
        handler(error::service_suspended, nullptr);
        return;
    }

    if (stopped_)
    {
        socket->stop();
        handler(error::service_stopped, nullptr);
        return;
    }

    // Successful accept.
    handler(error::success, socket);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
