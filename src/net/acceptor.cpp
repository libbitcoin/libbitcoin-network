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

using namespace bc::system;
using namespace std::placeholders;

// Construct.
// ----------------------------------------------------------------------------
// Boost: "The io_context object that the acceptor will use to dispatch
// handlers for any asynchronous operations performed on the acceptor."
// Calls are stranded to protect the acceptor member.

acceptor::acceptor(asio::strand& strand, asio::io_context& service,
    const settings& settings) NOEXCEPT
  : settings_(settings),
    service_(service),
    strand_(strand),
    acceptor_(strand_),
    stopped_(true)
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
    static const auto reuse_address = asio::acceptor::reuse_address(true);
    error::boost_code ec;

    // This is hardwired to listen on IPv6.
    const asio::endpoint endpoint(asio::tcp::v6(), port);
    acceptor_.open(endpoint.protocol(), ec);

    if (!ec)
        acceptor_.set_option(reuse_address, ec);

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

void acceptor::accept(accept_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    const auto socket = std::make_shared<network::socket>(service_);

    // Posts handle_accept to strand.
    // This does not post to the socket strand, unlike other socket calls.
    socket->accept(acceptor_,
        std::bind(&acceptor::handle_accept,
            shared_from_this(), _1, socket, std::move(handler)));
}

// private
void acceptor::handle_accept(const code& ec, const socket::ptr& socket,
    const accept_handler& handler) NOEXCEPT
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

    const auto channel = std::make_shared<network::channel>(socket, settings_);

    // Successful accept.
    handler(error::success, channel);
}

} // namespace network
} // namespace libbitcoin
