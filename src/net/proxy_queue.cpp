/**
 * Copyright (c) 2011-2026 libbitcoin developers
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

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

// stackoverflow.com/questions/7754695/boost-asio-async-write-how-to-not-
// interleaving-async-write-calls

namespace libbitcoin {
namespace network {

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace std::placeholders;

// Send cycle (send continues until queue is empty).
// ----------------------------------------------------------------------------
// private

// The memory of a queue entry apart from its payload: the writer closure with
// its bound pointers, the queue slot, control blocks and the message object.
// This bounds queue depth implicitly, as a flood of small messages is charged
// its actual cost and not its wire size.
constexpr size_t entry_size = 256;

void proxy::do_write(pending write_, bool closing) NOEXCEPT
{
    BC_ASSERT(stranded());

    write_.cost = system::ceilinged_add(write_.cost, entry_size);

    if (stopped())
    {
        // Does not queue new work or invoke handler after stop.
        LOGQ("Payload write abort [" << endpoint() << "]");
        return;
    }

    const auto started = !queue_.empty();

    // An idle queue admits any message, so that one that exceeds the backlog
    // remains sendable, as does the final message of the channel. A zmtp
    // publisher is the only lossy channel, as its sequence numbering exposes
    // the gap to the subscriber.
    if (!closing && started &&
        system::ceilinged_add(backlog_, write_.cost) > maximum_backlog_)
    {
        if (socket_->zeromq())
        {
            LOGS("Dropped message on congested channel [" << endpoint() << "]");
            write_.handler(error::message_dropped, zero);
            return;
        }

        LOGS("Stopping congested channel [" << endpoint() << "]");
        stop(error::channel_backlog);
        return;
    }

    backlog_ = system::ceilinged_add(backlog_, write_.cost);
    queue_.push_back(std::move(write_));

    // Start the asynchronous loop if it wasn't already started.
    if (!started)
        write();
}

void proxy::write() NOEXCEPT
{
    BC_ASSERT(stranded());
    if (queue_.empty())
        return;

    // Invokes oldest writer on the queue, completion invokes handle_write.
    writing_ = true;
    queue_.front().call();
}

void proxy::handle_write(const code& ec, size_t bytes,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (queue_.empty())
        return;

    // Handler precedes pop so that a handler send does not start a second
    // write loop (a non-empty queue defers the start to the pop below).
    handler(ec, bytes);
    backlog_ = system::floored_subtract(backlog_, queue_.front().cost);
    queue_.pop_front();

    // All handlers must be invoked unless stopped, so continue despite code.
    write();
}

// Throttle (sent bytes are allocated time at the configured rate).
// ----------------------------------------------------------------------------
// private
// Applied to every send, queued or not, so that the deferral is imposed
// without imposing the queue (http is half duplex, so it is not queued).

// Nanoseconds allocated to the transmission of bytes at the given rate.
inline steady_clock::duration to_allocation(size_t bytes,
    uint32_t rate) NOEXCEPT
{
    using namespace system;
    constexpr auto nanos = 1'000'000'000_u64;

    // Overflow implies an unusable rate/size, saturated at the type maximum.
    const auto span = ceilinged_multiply<uint64_t>(bytes, nanos) / rate;
    return nanoseconds{ limit<nanoseconds::rep>(span) };
}

count_handler proxy::counted(count_handler&& handler) NOEXCEPT
{
    return std::bind(&proxy::handle_counted,
        shared_from_this(), _1, _2, std::move(handler));
}

void proxy::count_received(size_t bytes) NOEXCEPT
{
    received_ = system::ceilinged_add(received_.load(), bytes);
}

void proxy::handle_counted(const code& ec, size_t bytes,
    const count_handler& handler) NOEXCEPT
{
    count_received(bytes);
    handler(ec, bytes);
}

count_handler proxy::metered(count_handler&& handler) NOEXCEPT
{
    // Stamped at issue, so that only transmission time is credited against
    // the allocation. Time spent idle between sends earns nothing.
    return std::bind(&proxy::handle_metered,
        shared_from_this(), _1, _2, steady_clock::now(), std::move(handler));
}

steady_clock::duration proxy::unconsumed(size_t bytes,
    const steady_clock::time_point& start) const NOEXCEPT
{
    BC_ASSERT(stranded());

    // Stop is never deferred, and a null throttle implies no rate limit.
    if (!throttle_ || stopped())
        return {};

    const auto allocated = to_allocation(bytes, rate_limit_);
    const auto consumed = steady_clock::now() - start;
    return consumed < allocated ? allocated - consumed :
        steady_clock::duration{};
}

void proxy::handle_metered(const code& ec, size_t bytes,
    const steady_clock::time_point& start,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    sent_ = system::ceilinged_add(sent_.load(), bytes);
    writing();
    writing_ = false;

    // A send that consumed its full allocation is not deferred.
    const auto delay = unconsumed(bytes, start);
    if (is_zero(delay.count()))
    {
        handler(ec, bytes);
        return;
    }

    // Handler is posted to the strand, and fired by stop (canceled).
    throttle_->start(std::bind(&proxy::handle_charge,
        shared_from_this(), _1, ec, bytes, handler), delay);
}

void proxy::handle_charge(const code&, const code& ec, size_t bytes,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // The timer code is discarded, as the send result is what is reported.
    handler(ec, bytes);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
