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
#include <bitcoin/network/net/proxy.hpp>

#include <algorithm>
#include <functional>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

namespace libbitcoin {
namespace network {

// Bind throws (ok).
// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

using namespace system;
using namespace messages;
using namespace std::placeholders;

// Dump up to this size of payload as hex in order to diagnose failure.
static constexpr size_t invalid_payload_dump_size = 0xff;
static constexpr uint32_t http_magic  = 0x20544547;
static constexpr uint32_t https_magic = 0x02010316;

// This is created in a started state and must be stopped, as the subscribers
// assert if not stopped. Subscribers may hold protocols even if the service
// is not started.
proxy::proxy(const socket::ptr& socket) NOEXCEPT
  : socket_(socket),
    stop_subscriber_(socket->strand()),
    distributor_(socket->strand()),
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

    // TODO: resume of an idle channel results in termination for invalid_magic.
    read_heading();
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

    // Post message handlers to strand and clear/stop accepting subscriptions.
    // On channel_stopped message subscribers should ignore and perform no work.
    distributor_.stop(ec);

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

// Read cycle (read continues until stop called).
// ----------------------------------------------------------------------------

code proxy::notify(identifier id, uint32_t version,
    const data_chunk& source) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // TODO: build witness into feature w/magic and negotiated version.
    // TODO: if self and peer services show witness, set feature true.
    return distributor_.notify(id, version, source);
}

void proxy::read_heading() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Both terminate read loop, paused can be resumed, stopped cannot.
    // Pause only prevents start of the read loop, it does not prevent messages
    // from being issued for sockets already past that point (e.g. waiting).
    // This is mainly for startup coordination, preventing missed messages.
    if (stopped() || paused())
        return;

    // Post handle_read_heading to strand upon stop, error, or buffer full.
    socket_->read(heading_buffer_,
        std::bind(&proxy::handle_read_heading,
            shared_from_this(), _1, _2));
}

void proxy::handle_read_heading(const code& ec, size_t) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        LOGQ("Heading read abort [" << authority() << "]");
        stop(error::channel_stopped);
        return;
    }

    if (ec)
    {
        if (ec != error::peer_disconnect && ec != error::operation_canceled)
        {
            LOGF("Heading read failure [" << authority() << "] "
                << ec.message());
        }

        stop(ec);
        return;
    }

    heading_reader_.set_position(zero);
    const auto head = to_shared(heading::deserialize(heading_reader_));

    if (!heading_reader_)
    {
        LOGR("Invalid heading from [" << authority() << "]");
        stop(error::invalid_heading);
        return;
    }

    if (head->magic != protocol_magic())
    {
        if (head->magic == http_magic || head->magic == https_magic)
        {
            LOGR("Http/s request from [" << authority() << "]");
        }
        else
        {
            LOGR("Invalid heading magic (0x"
                << encode_base16(to_little_endian(head->magic))
                << ") from [" << authority() << "]");
        }

        stop(error::invalid_magic);
        return;
    }

    if (head->payload_size > maximum_payload())
    {
        LOGR("Oversized payload indicated by " << head->command
            << " heading from [" << authority() << "] ("
            << head->payload_size << " bytes)");

        stop(error::oversized_payload);
        return;
    }

    // Buffer capacity increases with each larger message (up to maximum).
    payload_buffer_.resize(head->payload_size);

    // Post handle_read_payload to strand upon stop, error, or buffer full.
    socket_->read(payload_buffer_,
        std::bind(&proxy::handle_read_payload,
            shared_from_this(), _1, _2, head));
}

// Handle errors and post message to subscribers.
// The head object is allocated on another thread and destroyed on this one.
// This introduces cross-thread allocation/deallocation, though size is small.
void proxy::handle_read_payload(const code& ec, size_t LOG_ONLY(payload_size),
    const heading_ptr& head) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        LOGQ("Payload read abort [" << authority() << "]");
        stop(error::channel_stopped);
        return;
    }

    if (ec)
    {
        if (ec != error::peer_disconnect && ec != error::operation_canceled)
        {
            LOGF("Payload read failure [" << authority() << "] "
                << ec.message());
        }

        stop(ec);
        return;
    }

    if (validate_checksum())
    {
        // This hash could be reused as w/txid, but simpler to disable check.
        if (head->checksum != network_checksum(bitcoin_hash(payload_buffer_)))
        {
            LOGR("Invalid " << head->command << " payload from ["
                << authority() << "] bad checksum.");

            stop(error::invalid_checksum);
            return;
        }
    }

    // Notify subscribers of the new message.
    // The message object is allocated on this thread and notify invokes
    // subscribers on the same thread. This significantly reduces deallocation
    // cost in constrast to allowing the object to destroyed on another thread.
    // If object is passed to another thread destruction cost can be very high.
    const auto code = notify(head->id(), version(), payload_buffer_);

    if (code)
    {
        if (head->command == messages::transaction::command ||
            head->command == messages::block::command)
        {
            LOGR("Invalid " << head->command << " payload from [" << authority()
                << "] with hash [" << encode_hash(bitcoin_hash(payload_buffer_)) << "] "
                << code.message());
        }
        else
        {
            LOGR("Invalid " << head->command << " payload from [" << authority()
                << "] with bytes (" << encode_base16(
                    {
                        payload_buffer_.begin(),
                        std::next(payload_buffer_.begin(),
                        std::min(payload_size, invalid_payload_dump_size))
                    })
                << "...) " << code.message());
        }

        stop(code);
        return;
    }

    // Don't retain larger than configured (time-space tradeoff).
    if (minimum_buffer() < payload_buffer_.capacity())
    {
        payload_buffer_.resize(minimum_buffer());
        payload_buffer_.shrink_to_fit();
    }

    LOGX("Recv " << head->command << " from [" << authority() << "] ("
        << payload_size << " bytes)");

    signal_activity();
    read_heading();
}

// Send cycle (send continues until queue is empty).
// ----------------------------------------------------------------------------
// stackoverflow.com/questions/7754695/boost-asio-async-write-how-to-not-
// interleaving-async-write-calls

void proxy::write(const system::chunk_ptr& payload,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        LOGQ("Payload write abort [" << authority() << "]");
        handler(error::channel_stopped);
        return;
    }

    const auto started = !queue_.empty();
    queue_.push_back(std::make_pair(payload, handler));
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
    const system::chunk_ptr& LOG_ONLY(payload),
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
        if (ec != error::peer_disconnect && ec != error::operation_canceled)
        {
            LOGF("Send failure " << heading::get_command(*payload) << " to ["
                << authority() << "] (" << payload->size() << " bytes) "
                << ec.message());
        }

        stop(ec);
        handler(ec);
        return;
    }

    LOGX("Sent " <<  heading::get_command(*payload) << " to ["
        << authority() << "] (" << payload->size() << " bytes)");

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
