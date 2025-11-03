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
#include <bitcoin/network/net/proxy.hpp>

#include <algorithm>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/memory.hpp>

namespace libbitcoin {
namespace network {

// Bind throws (ok).
// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

using namespace system;
using namespace std::placeholders;

// This is created in a started state and must be stopped, as the subscribers
// assert if not stopped. Subscribers may hold protocols even if the service
// is not started.
proxy::proxy(const socket::ptr& socket) NOEXCEPT
  : socket_(socket),
    stop_subscriber_(socket->strand()),
    reporter(socket->log)
{
}

proxy::~proxy() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "proxy is not stopped");
    if (!stopped()) { LOGF("~proxy is not stopped."); }
}

// Pause (proxy is created paused).
// ----------------------------------------------------------------------------
// public

void proxy::pause() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    paused_ = true;
}

void proxy::resume() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    paused_ = false;
}

bool proxy::paused() const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return paused_;
}

// protected
// override to update timers.
void proxy::waiting() NOEXCEPT
{
}

// Stop (socket/proxy started upon create).
// ----------------------------------------------------------------------------
// The proxy is not allowed to stop itself (internally).

bool proxy::stopped() const NOEXCEPT
{
    return socket_->stopped();
}

void proxy::stop(const code& ec) NOEXCEPT
{
    if (stopped())
        return;

    // Stop the read loop, stop accepting new work, cancel pending work.
    socket_->stop();

    // Overruled by stop, set only for consistency.
    paused_.store(true);

    boost::asio::post(strand(),
        std::bind(&proxy::stopping, shared_from_this(), ec));
}

// protected
// This should not be called internally, as derived rely on stop() override.
void proxy::stopping(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Clear the write buffer, which holds handlers.
    queue_.clear();

    // Post stop handlers to strand and clear/stop accepting subscriptions.
    // The code provides information on the reason that the channel stopped.
    stop_subscriber_.stop(ec);
}

// protected
void proxy::subscribe_stop(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
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
    BC_ASSERT_MSG(stranded(), "strand");
    stop_subscriber_.subscribe(move_copy(handler));
    complete(error::success);
}

// TCP.
// ----------------------------------------------------------------------------

void proxy::read(const asio::mutable_buffer& buffer,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand(),
        std::bind(&proxy::waiting, shared_from_this()));

    socket_->read(buffer, std::move(handler));
}

void proxy::write(const asio::const_buffer& payload,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_write,
            shared_from_this(), payload, std::move(handler)));
}

// HTTP.
// ----------------------------------------------------------------------------

// Method waiting() is invoked directly if read() is called from strand().
void proxy::read(http::flat_buffer& buffer, http::request& request,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand(),
        std::bind(&proxy::waiting, shared_from_this()));

    socket_->http_read(buffer, request, std::move(handler));
}

// Writes are composed but http is half duplex so there is no interleave risk.
void proxy::write(http::response& response,
    count_handler&& handler) NOEXCEPT
{
    socket_->http_write(response, std::move(handler));
}

// WS.
// ----------------------------------------------------------------------------

void proxy::ws_read(http::flat_buffer& out, count_handler&& handler) NOEXCEPT
{
    socket_->ws_read(out, std::move(handler));
}

void proxy::ws_write(const asio::const_buffer& in,
    count_handler&& handler) NOEXCEPT
{
    socket_->ws_write(in, std::move(handler));
}

// Send cycle (send continues until queue is empty).
// ----------------------------------------------------------------------------
// stackoverflow.com/questions/7754695/boost-asio-async-write-how-to-not-
// interleaving-async-write-calls

// private
void proxy::do_write(const asio::const_buffer& payload,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        LOGQ("Payload write abort [" << authority() << "]");
        handler(error::channel_stopped, zero);
        return;
    }

    const auto started = !queue_.empty();
    queue_.push_back(std::make_pair(payload, std::move(handler)));
    total_ = ceilinged_add(total_.load(), payload.size());
    backlog_ = ceilinged_add(backlog_.load(), payload.size());

    LOGX("Queue for [" << authority() << "]: " << queue_.size()
        << " (" << backlog_.load() << " of " << total_.load() << " bytes)");

    // Start the loop if it wasn't already started.
    if (!started)
        write();
}

// private
void proxy::write() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (queue_.empty())
        return;

    auto& job = queue_.front();
    socket_->write({ job.first.data(), job.first.size() },
        std::bind(&proxy::handle_write,
            shared_from_this(), _1, _2, job.first, job.second));
}

// private
void proxy::handle_write(const code& ec, size_t bytes,
    const asio::const_buffer& /* LOG_ONLY(payload) */,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        LOGQ("Send abort [" << authority() << "]");
        return;
    }

    // guarded by write().
    backlog_ = floored_subtract(backlog_.load(), queue_.front().first.size());
    queue_.pop_front();

    LOGX("Dequeue for [" << authority() << "]: " << queue_.size()
        << " (" << backlog_.load() << " backlog)");

    // All handlers must be invoked, so continue regardless of error state.
    // Handlers are invoked in queued order, after all outstanding complete.
    write();

    if (ec)
    {
        // Linux reports error::connect_failed when peer drops here.
        if (ec != error::peer_disconnect && ec != error::operation_canceled &&
            ec != error::connect_failed)
        {
            // BUGBUG: payload changed from data_chunk_ptr to const_buffer.
            // TODO: messages dependency, move to channel.
            ////LOGF("Send failure " << heading::get_command(*payload) << " to ["
            ////    << authority() << "] (" << payload->size() << " bytes) "
            ////    << ec.message());
        }

        handler(ec, zero);
        return;
    }

    // BUGBUG: payload changed from data_chunk_ptr to const_buffer.
    // TODO: messages dependency, move to channel.
    ////LOGX("Sent " <<  heading::get_command(*payload) << " to ["
    ////    << authority() << "] (" << payload->size() << " bytes)");

    handler(ec, bytes);
}

// Properties.
// ----------------------------------------------------------------------------

asio::strand& proxy::strand() NOEXCEPT
{
    return socket_->strand();
}

bool proxy::stranded() const NOEXCEPT
{
    return socket_->stranded();
}

bool proxy::websocket() const NOEXCEPT
{
    return socket_->websocket();
}

void proxy::set_websocket() NOEXCEPT
{
    return socket_->set_websocket();
}

uint64_t proxy::backlog() const NOEXCEPT
{
    return backlog_.load(std::memory_order_relaxed);
}

uint64_t proxy::total() const NOEXCEPT
{
    return total_.load(std::memory_order_relaxed);
}

bool proxy::inbound() const NOEXCEPT
{
    return socket_->inbound();
}

const config::authority& proxy::authority() const NOEXCEPT
{
    return socket_->authority();
}

const config::address& proxy::address() const NOEXCEPT
{
    return socket_->address();
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
