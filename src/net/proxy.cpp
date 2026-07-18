/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/net/proxy.hpp>

#include <utility>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/socket.hpp>

namespace libbitcoin {
namespace network {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// This is created in a started state and must be stopped, as the subscribers
// assert if not stopped. Subscribers may hold protocols even if the service
// is not started.
proxy::proxy(const socket::ptr& socket) NOEXCEPT
  : socket_(socket),
    reporter(socket->log)
{
}

proxy::~proxy() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "proxy is not stopped");
    if (!stopped()) { LOGF("~proxy is not stopped."); }
}

// Stop (socket/proxy started upon create).
// ----------------------------------------------------------------------------
// The proxy does not (must not) stop itself.

void proxy::stop(const code& ec) NOEXCEPT
{
    if (stopped())
        return;

    // Client websocket or timer initated (when ws) stop is async (graceful).
    // error::service_stopped will invoke socket stop() below, which will
    // immediately terminate outstanding lazy_stop().
    if (ec == error::websocket_closed ||
        ec == error::channel_expired ||
        ec == error::channel_inactive)
    {
        // Stop the read loop, stop accepting new work, cancel pending work.
        // Allows for graceful ws/ssl::close, which would hang threadpool if
        // attempted within socket::stop(), as it issues a follow-on iocontext
        // job. A subsequent call to socket::stop() will terminate directly.
        socket_->lazy_stop();
    }
    else
    {
        // Stop the read loop, stop accepting new work, cancel pending work.
        socket_->stop();
    }

    // Overruled by stop, set only for consistency.
    paused_.store(true);

    boost::asio::post(strand(),
        std::bind(&proxy::stopping, shared_from_this(), ec));
}

// protected
// This should not be called internally (invoked by stop()).
void proxy::stopping(const code& ec) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Release any http message parse in progress.
    parser_.reset();

    // Post stop handlers to strand and clear/stop accepting subscriptions.
    // The code provides information on the reason that the channel stopped.
    stop_subscriber_.stop(ec);
}

// protected
void proxy::subscribe_stop(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    stop_subscriber_.subscribe(std::move(handler));
}

void proxy::subscribe_stop(result_handler&& handler,
    result_handler&& complete) NOEXCEPT
{
    if (stopped())
    {
        complete(error::channel_stopped);
        handler(error::channel_stopped);
        return;
    }

    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_subscribe_stop,
            shared_from_this(), std::move(handler), std::move(complete)));
}

// private
void proxy::do_subscribe_stop(const result_handler& handler,
    const result_handler& complete) NOEXCEPT
{
    BC_ASSERT(stranded());
    stop_subscriber_.subscribe(move_copy(handler));
    complete(error::success);
}

// Pause (proxy is created paused).
// ----------------------------------------------------------------------------
// public

void proxy::pause() NOEXCEPT
{
    BC_ASSERT(stranded());
    paused_ = true;
}

void proxy::resume() NOEXCEPT
{
    BC_ASSERT(stranded());
    paused_ = false;
}

bool proxy::paused() const NOEXCEPT
{
    BC_ASSERT(stranded());
    return paused_;
}

// Signal activity.
// ----------------------------------------------------------------------------
// override reading() to update timers.

// protected
void proxy::reading() NOEXCEPT
{
}

// private
void proxy::do_reading() NOEXCEPT
{
    boost::asio::dispatch(strand(),
        std::bind(&proxy::reading, shared_from_this()));
}

// Properties.
// ----------------------------------------------------------------------------

asio::context& proxy::service() const NOEXCEPT
{
    return socket_->service();
}

asio::strand& proxy::strand() NOEXCEPT
{
    return socket_->strand();
}

bool proxy::stranded() const NOEXCEPT
{
    return socket_->stranded();
}

bool proxy::stopped() const NOEXCEPT
{
    return socket_->stopped();
}

bool proxy::inbound() const NOEXCEPT
{
    return socket_->inbound();
}

bool proxy::secure() const NOEXCEPT
{
    return socket_->secure();
}

bool proxy::websocket() const NOEXCEPT
{
    return socket_->websocket();
}

uint64_t proxy::total() const NOEXCEPT
{
    return total_.load(std::memory_order_relaxed);
}

const config::address& proxy::address() const NOEXCEPT
{
    return socket_->address();
}

const config::endpoint& proxy::endpoint() const NOEXCEPT
{
    return socket_->endpoint();
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
