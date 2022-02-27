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

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <boost/asio.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/net/channel.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace network::config;
using namespace std::placeholders;

// Construct.
// ---------------------------------------------------------------------------

connector::connector(asio::strand& strand, asio::io_context& service,
    const settings& settings)
  : settings_(settings),
    service_(service),
    strand_(strand),
    timer_(std::make_shared<deadline>(strand_, settings_.connect_timeout())),
    resolver_(strand_),
    stopped_(true)
{
}

void connector::stop()
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return;

    // Posts timer handler to strand.
    // resolver and socket.connect are canceled in the timer handler.
    timer_->stop();
}

// Methods.
// ---------------------------------------------------------------------------

void connector::connect(const endpoint& endpoint, connect_handler&& handler)
{
    connect(endpoint.host(), endpoint.port(), std::move(handler));
}

void connector::connect(const authority& authority, connect_handler&& handler)
{
    connect(authority.to_hostname(), authority.port(), std::move(handler));
}

void connector::connect(const std::string& hostname, uint16_t port,
    connect_handler&& handler)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // Enables reusability.
    stopped_ = false;

    const auto socket = std::make_shared<network::socket>(service_);

    // Posts timer handler to strand.
    // The handler is copied by std::bind.
    timer_->start(
        std::bind(&connector::handle_timer,
            shared_from_this(), _1, socket, handler));

    // Posts handle_resolve to strand.
    // async_resolve copies string parameters.
    resolver_.async_resolve(hostname, std::to_string(port),
        std::bind(&connector::handle_resolve,
            shared_from_this(), _1, _2, socket, std::move(handler)));
}

// private
void connector::handle_resolve(const error::boost_code& ec,
    const asio::endpoints& range, socket::ptr socket, connect_handler handler)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // Ensure only the handler executes only once, as both may be posted.
    if (stopped_)
        return;

    if (ec)
    {
        // Resolve result codes return here.
        // Cancel not handled here because handled first in timer.
        handler(error::asio_to_error_code(ec), nullptr);
        return;
    }

    // boost::asio::bind_executor not working.
    ////// Posts handle_connect to strand (after socket strand).
    ////socket->connect(range,
    ////    boost::asio::bind_executor(strand_,
    ////        std::bind(&connector::handle_connect,
    ////            shared_from_this(), _1, socket, std::move(handler))));

    socket->connect(range,
        std::bind(&connector::handle_connect,
            shared_from_this(), _1, socket, handler));
}

// private
void connector::handle_connect(const code& ec, socket::ptr socket,
    connect_handler handler)
{
    boost::asio::post(strand_,
        std::bind(&connector::do_handle_connect,
            shared_from_this(), ec, socket, handler));
}

// private
void connector::do_handle_connect(const code& ec, socket::ptr socket,
    const connect_handler& handler)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // Ensure only the handler executes only once, as both may be posted.
    if (stopped_)
        return;

    stopped_ = true;

    // Posts timer handler to strand (if not expired).
    timer_->stop();

    if (ec)
    {
        // Connect result codes return here.
        handler(ec, nullptr);
        return;
    }

    const auto channel = std::make_shared<network::channel>(socket, settings_);

    // Successful connect.
    handler(error::success, channel);
}

// private
void connector::handle_timer(const code& ec, const socket::ptr& socket,
    const connect_handler& handler)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // Ensure only the handler executes only once, as both may be posted.
    if (stopped_)
        return;

    // Posts handle_resolve to strand (if not already posted).
    socket->stop();
    resolver_.cancel();
    stopped_ = true;

    if (ec)
    {
        // Stop result code (error::operation_canceled) return here.
        handler(ec, nullptr);
        return;
    }

    // Timeout result code (error::success) translated to channel_timeout here.
    handler(error::channel_timeout, nullptr);
}

} // namespace network
} // namespace libbitcoin
