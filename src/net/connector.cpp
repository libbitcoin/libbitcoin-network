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
#include <bitcoin/network/net/channel.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace bc::system::config;
using namespace std::placeholders;

// Construct.
// ---------------------------------------------------------------------------

connector::connector(asio::io_context& service, const settings& settings)
  : settings_(settings),
    strand_(service),
    timer_(strand_, settings_.connect_timeout()),
    resolver_(strand_),
    stopped_(true),
    CONSTRUCT_TRACK(connector)
{
}

// Stop.
// ---------------------------------------------------------------------------

void connector::stop(const code&)
{
    // strand::dispatch invokes its handler directly if the strand is not busy,
    // which hopefully blocks the strand until the dispatch call completes.
    // Otherwise the handler is posted to the strand for deferred completion.
    strand_.dispatch(std::bind(&connector::do_stop, shared_from_this()));
}

// private
void connector::do_stop()
{
    // Posts handle_resolve to strand.
    resolver_.cancel();

    // Posts timer handler to strand (if not expired).
    // But timer handler does not invoke handle_timer on stop.
    timer_.stop();
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
    // hostname is copied by std::bind, may be discarded by caller.
    // Dispatch executes within this call if strand is not busy.
    strand_.dispatch(
        std::bind(&connector::do_resolve,
            shared_from_this(), hostname, port, std::move(handler)));
}

void connector::do_resolve(const std::string& hostname, uint16_t port,
    connect_handler handler)
{
    // Enables reusability.
    stopped_ = true;

    // Socket is required by timer, so create here.
    // io_context is noncopyable, so this references the constructor parameter.
    const auto socket = std::make_shared<network::socket>(strand_.context());

    // The handler is copied by std::bind.
    // Posts timer handler to strand (if not expired).
    // But timer handler does not invoke handle_timer on stop.
    timer_.start(
        std::bind(&connector::handle_timer,
            shared_from_this(), _1, socket, handler));

    // async_resolve copies string parameters.
    // Posts handle_resolve to strand.
    resolver_.async_resolve(hostname, std::to_string(port),
        std::bind(&connector::handle_resolve,
            shared_from_this(), _1, _2, socket, std::move(handler)));
}

// private
void connector::handle_resolve(const boost_code& ec, const asio::iterator& it,
    socket::ptr socket, connect_handler handler)
{
    BITCOIN_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // operation_aborted is the result of cancelation.
    if (ec == boost::asio::error::operation_aborted)
    {
        handler(system::error::channel_stopped, nullptr);
        return;
    }

    if (ec)
    {
        handler(error::boost_to_error_code(ec), nullptr);
        return;
    }

    // socket.connect copies iterator.
    // Posts handle_connect to strand (after socket strand).
    socket->connect(it,
        boost::asio::bind_executor(strand_,
            std::bind(&connector::handle_connect,
                shared_from_this(), _1, socket, std::move(handler))));
}

// private
void connector::handle_connect(const code& ec, socket::ptr socket,
    const connect_handler& handler)
{
    BITCOIN_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // Ensure only the handler executes only once, as both may be posted.
    if (stopped_)
        return;

    // Posts timer handler to strand (if not expired).
    // But timer handler does not invoke handle_timer on stop.
    timer_.stop();
    stopped_ = true;

    // socket->connect sets channel_stopped when canceled, otherwise error.
    if (ec)
    {
        handler(ec, nullptr);
        return;
    }

    const auto created = std::make_shared<channel>(socket, settings_);

    // Successful channel creation.
    handler(error::success, created);
}

// private
void connector::handle_timer(const code& ec, socket::ptr socket,
    const connect_handler& handler)
{
    BITCOIN_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // Ensure only the handler executes only once, as both may be posted.
    if (stopped_)
        return;

    // Posts handle_resolve to strand (if not already posted).
    resolver_.cancel();
    stopped_ = true;

    // Timer does not invoke handler when canceled, so this is an error.
    if (ec)
    {
        handler(ec, nullptr);
        return;
    }

    // Unsuccessful channel creation.
    handler(error::channel_timeout, nullptr);
}

} // namespace network
} // namespace libbitcoin
