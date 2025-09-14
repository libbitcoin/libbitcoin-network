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
#include <bitcoin/system.hpp>
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

// Stop (socket/proxy started upon create).
// ----------------------------------------------------------------------------

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
        std::bind(&proxy::do_stop, shared_from_this(), ec));
}

// This should not be called internally, as derived rely on stop() override.
void proxy::do_stop(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Clear the write buffer, which holds handlers.
    queue_.clear();

    // Post stop handlers to strand and clear/stop accepting subscriptions.
    // The code provides information on the reason that the channel stopped.
    stop_subscriber_.stop(ec);
}

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

void proxy::do_subscribe_stop(const result_handler& handler,
    const result_handler& complete) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop_subscriber_.subscribe(move_copy(handler));
    complete(error::success);
}

// Read partial (up to buffer-sized) message from peer.
// ----------------------------------------------------------------------------

void proxy::read_some(const data_slab& buffer,
    count_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    socket_->read_some(buffer, std::move(handler));
}

// Read complete (buffer-sized) message from peer.
// ----------------------------------------------------------------------------

void proxy::read(const data_slab& buffer, count_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    socket_->read(buffer, std::move(handler));
}

// Send cycle (send continues until queue is empty).
// ----------------------------------------------------------------------------
// stackoverflow.com/questions/7754695/boost-asio-async-write-how-to-not-
// interleaving-async-write-calls

void proxy::write(const chunk_ptr& payload, result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        LOGQ("Payload write abort [" << authority() << "]");
        handler(error::channel_stopped);
        return;
    }

    const auto started = !queue_.empty();
    queue_.push_back(std::make_pair(payload, std::move(handler)));
    total_ = ceilinged_add(total_.load(), payload->size());
    backlog_ = ceilinged_add(backlog_.load(), payload->size());

    LOGX("Queue for [" << authority() << "]: " << queue_.size()
        << " (" << backlog_.load() << " of " << total_.load() << " bytes)");

    // Start the loop if it wasn't already started.
    if (!started)
        write();
}

void proxy::write() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (queue_.empty())
        return;

    auto& job = queue_.front();
    socket_->write(*job.first,
        std::bind(&proxy::handle_write,
            shared_from_this(), _1, _2, job.first, job.second));
}

void proxy::handle_write(const code& ec, size_t,
    const system::chunk_ptr& /*LOG_ONLY(payload)*/,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        LOGQ("Send abort [" << authority() << "]");
        stop(error::channel_stopped);
        return;
    }

    // guarded by write().
    backlog_ = floored_subtract(backlog_.load(), queue_.front().first->size());
    queue_.pop_front();

    LOGX("Dequeue for [" << authority() << "]: " << queue_.size()
        << " (" << backlog_.load() << " bytes)");

    // All handlers must be invoked, so continue regardless of error state.
    // Handlers are invoked in queued order, after all outstanding complete.
    write();

    if (ec)
    {
        // Linux reports error::connect_failed when peer drops here.
        if (ec != error::peer_disconnect && ec != error::operation_canceled &&
            ec != error::connect_failed)
        {
            // TODO: messages dependency, move to channel.
            ////LOGF("Send failure " << heading::get_command(*payload) << " to ["
            ////    << authority() << "] (" << payload->size() << " bytes) "
            ////    << ec.message());
        }

        stop(ec);
        handler(ec);
        return;
    }

    // TODO: messages dependency, move to channel.
    ////LOGX("Sent " <<  heading::get_command(*payload) << " to ["
    ////    << authority() << "] (" << payload->size() << " bytes)");

    handler(ec);
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
