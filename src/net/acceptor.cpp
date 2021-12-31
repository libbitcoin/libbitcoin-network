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

acceptor::acceptor(asio::io_context& service, const settings& settings)
  : settings_(settings),
    strand_(service),
    timer_(strand_, settings_.connect_timeout()),
    acceptor_(strand_),
    stopped_(false),
    CONSTRUCT_TRACK(acceptor)
{
}

// Start/stop.
// ----------------------------------------------------------------------------

code acceptor::start(uint16_t port)
{
    static const auto reuse_address = asio::acceptor::reuse_address(true);
    error::boost_code ec;

    // This is hardwired to listen on IPv6.
    asio::endpoint endpoint(asio::tcp::v6(), port);
    acceptor_.open(endpoint.protocol(), ec);

    if (!ec)
        acceptor_.set_option(reuse_address, ec);

    if (!ec)
        acceptor_.bind(endpoint, ec);

    if (!ec)
        acceptor_.listen(asio::max_connections, ec);

    return error::asio_to_error_code(ec);
}

void acceptor::stop()
{
    // strand::dispatch invokes its handler directly if the strand is not busy,
    // which hopefully blocks the strand until the dispatch call completes.
    // Otherwise the handler is posted to the strand for deferred completion.
    strand_.dispatch(std::bind(&acceptor::do_stop, shared_from_this()));
}

// private
void acceptor::do_stop()
{
    // Posts handle_accept to strand.
    error::boost_code ignore;
    acceptor_.cancel(ignore);

    // Posts timer handler to strand (if not expired).
    // But timer handler does not invoke handle_timer on stop.
    timer_.stop();
}

// Methods.
// ----------------------------------------------------------------------------

void acceptor::accept(accept_handler&& handler)
{
    // Dispatch executes within this call if strand is not busy.
    strand_.dispatch(
        std::bind(&acceptor::do_accept,
            shared_from_this(), std::move(handler)));
}

// private
void acceptor::do_accept(accept_handler handler)
{
    // Enables reusability.
    stopped_ = true;

    // The handler is copied by std::bind.
    // Posts timer handler to strand (if not expired).
    // But timer handler does not invoke handle_timer on stop.
    timer_.start(
        std::bind(&acceptor::handle_timer,
            shared_from_this(), _1, handler));

    // Calls on socket are unsafe during socket->accept (ok).
    // io_context is noncopyable, so this references the constructor parameter.
    const auto socket = std::make_shared<network::socket>(strand_.context());

    // Posts handle_accept to strand.
    // This does not post to the socket strand, unlike other socket calls.
    socket->accept(acceptor_,
        std::bind(&acceptor::handle_accept,
            shared_from_this(), _1, socket, std::move(handler)));
}

// private
void acceptor::handle_accept(const code& ec, socket::ptr socket,
    const accept_handler& handler)
{
    BITCOIN_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // Ensure only the handler executes only once, as both may be posted.
    if (stopped_)
        return;

    // Posts timer handler to strand (if not expired).
    // But timer handler does not invoke handle_timer on stop.
    timer_.stop();
    stopped_ = true;

    // socket->accept sets channel_stopped when canceled, otherwise error.
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
void acceptor::handle_timer(const code& ec, const accept_handler& handler)
{
    BITCOIN_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // Ensure only the handler executes only once, as both may be posted.
    if (stopped_)
        return;

    // Posts handle_accept to strand (if not already posted).
    error::boost_code ignore;
    acceptor_.cancel(ignore);
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
