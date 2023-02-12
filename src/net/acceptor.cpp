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
#include <bitcoin/network/net/acceptor.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/error.hpp>
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
    asio::io_context& service, const settings& settings) NOEXCEPT
  : settings_(settings),
    service_(service),
    strand_(strand),
    acceptor_(strand_),
    reporter(log),
    tracker<acceptor>(log)
{
}

acceptor::~acceptor() NOEXCEPT
{
    BC_ASSERT_MSG(stopped_, "acceptor is not stopped");
}

// Start/stop.
// ----------------------------------------------------------------------------

code acceptor::start(uint16_t port) NOEXCEPT
{
    error::boost_code ec;
    const auto ipv6 = settings_.enable_ipv6;
    const auto endpoint = make_endpoint(ipv6, port);

    // Open the socket.
    acceptor_.open(endpoint.protocol(), ec);

    // if ipv6 is enabled, also enable ipv4.
    if (!ec && ipv6)
        acceptor_.set_option(asio::v6_only(false), ec);

    if (!ec)
        acceptor_.set_option(asio::reuse_address(true), ec);

    if (!ec)
        acceptor_.bind(endpoint, ec);

    if (!ec)
        acceptor_.listen(asio::max_connections, ec);

    // This allows accept after stop (restartable).
    if (!ec)
        stopped_ = false;

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

// Methods.
// ----------------------------------------------------------------------------

void acceptor::accept(channel_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    const auto socket = std::make_shared<network::socket>(log(), service_);

    // Posts handle_accept to strand.
    // This does not post to the socket strand, unlike other socket calls.
    socket->accept(acceptor_,
        std::bind(&acceptor::handle_accept,
            shared_from_this(), _1, socket, std::move(handler)));
}

// private
void acceptor::handle_accept(const code& ec, const socket::ptr& socket,
    const channel_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (ec)
    {
        // Prevent non-stop assertion (accept failed but socket is started).
        socket->stop();

        // Connect result codes return here.
        // Stop result code (error::operation_canceled) return here.
        handler(ec, nullptr);
        return;
    }

    if (stopped_)
    {
        // Prevent non-stop assertion (socket unused).
        socket->stop();

        handler(error::service_stopped, nullptr);
        return;
    }

    const auto channel = std::make_shared<network::channel>(log(), socket,
        settings_);

    // Successful accept.
    handler(error::success, channel);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
