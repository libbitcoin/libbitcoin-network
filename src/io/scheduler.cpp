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
#include "scheduler.hpp"

#include <algorithm>
#include <bitcoin/network/define.hpp>
#include "backend.hpp"

namespace libbitcoin {
namespace network {
namespace io {
namespace detail {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// The scheduler (and strand) running on this thread, if any.
static thread_local const scheduler* running_scheduler{};
static thread_local const strand_state* running_strand{};

template <typename Type>
class running_scope
{
public:
    running_scope(const Type*& slot, const Type* value) NOEXCEPT
      : slot_(slot), prior_(slot)
    {
        slot_ = value;
    }

    ~running_scope() NOEXCEPT
    {
        slot_ = prior_;
    }

private:
    const Type*& slot_;
    const Type* prior_;
};

// scheduler
// ----------------------------------------------------------------------------

scheduler::scheduler() NOEXCEPT
  : backend_(std::make_unique<detail::backend>(*this))
{
}

scheduler::~scheduler() NOEXCEPT
{
    {
        std::unique_lock lock{ resolve_mutex_ };
        resolve_stop_ = true;
    }

    resolve_condition_.notify_all();
    if (resolver_.joinable())
        resolver_.join();
}

size_t scheduler::run() NOEXCEPT
{
    const running_scope scope{ running_scheduler, this };
    std::vector<handler> completions{};
    size_t count{};

    std::unique_lock lock{ mutex_ };
    if (is_zero(outstanding_))
    {
        stop_locked();
        return count;
    }

    while (!stopped_)
    {
        // Each pass over the queue yields once to the backend, so that posted
        // work cannot starve completions and timers.
        const auto poll = !queue_.empty() && is_zero(pass_) &&
            !waiting_backend_;

        if (!queue_.empty() && !poll)
        {
            if (is_nonzero(pass_))
                --pass_;

            auto next = std::move(queue_.front());
            queue_.pop_front();
            if (!queue_.empty() && is_nonzero(waiting_))
                condition_.notify_one();

            lock.unlock();
            next();
            ++count;
            lock.lock();

            if (is_zero(--outstanding_))
                stop_locked();

            continue;
        }

        // One thread at a time waits on the backend, others on the queue.
        if (!waiting_backend_)
        {
            waiting_backend_ = true;
            const auto duration = poll ? clock::duration::zero() :
                next_timeout();
            lock.unlock();

            completions.clear();
            backend_->wait(duration, completions);

            lock.lock();
            waiting_backend_ = false;
            for (auto& completion: completions)
                queue_.push_back(std::move(completion));

            expire_timers();
            pass_ = queue_.size();
            if (!queue_.empty() && is_nonzero(waiting_))
                condition_.notify_all();

            continue;
        }

        ++waiting_;
        condition_.wait(lock);
        --waiting_;
    }

    return count;
}

void scheduler::stop() NOEXCEPT
{
    std::unique_lock lock{ mutex_ };
    stop_locked();
}

bool scheduler::stopped() const NOEXCEPT
{
    std::unique_lock lock{ mutex_ };
    return stopped_;
}

void scheduler::restart() NOEXCEPT
{
    std::unique_lock lock{ mutex_ };
    stopped_ = false;
}

bool scheduler::running_in_this_thread() const NOEXCEPT
{
    return running_scheduler == this;
}

void scheduler::post(handler&& handler) NOEXCEPT
{
    std::unique_lock lock{ mutex_ };
    ++outstanding_;
    queue_.push_back(std::move(handler));
    wake();
}

void scheduler::complete(handler&& handler) NOEXCEPT
{
    std::unique_lock lock{ mutex_ };
    queue_.push_back(std::move(handler));
    wake();
}

void scheduler::work_started() NOEXCEPT
{
    std::unique_lock lock{ mutex_ };
    ++outstanding_;
}

void scheduler::work_finished() NOEXCEPT
{
    std::unique_lock lock{ mutex_ };
    if (is_zero(--outstanding_))
        stop_locked();
}

detail::backend& scheduler::backend() NOEXCEPT
{
    return *backend_;
}

// private
void scheduler::wake() NOEXCEPT
{
    if (is_nonzero(waiting_))
        condition_.notify_one();
    else if (waiting_backend_)
        backend_->interrupt();
}

// private
void scheduler::stop_locked() NOEXCEPT
{
    stopped_ = true;
    condition_.notify_all();
    if (waiting_backend_)
        backend_->interrupt();
}

// Timers.
// ----------------------------------------------------------------------------

void scheduler::wait_timer(const std::shared_ptr<timer_state>& timer,
    timer_state::waiter&& waiter) NOEXCEPT
{
    std::unique_lock lock{ mutex_ };
    ++outstanding_;
    timer->waiters.push_back(std::move(waiter));
    if (timer->queued)
        return;

    timer->queued = true;
    const auto position = timers_.emplace(timer->expiry, timer);

    // An earlier first timer shortens the backend wait.
    if (position == timers_.begin() && waiting_backend_)
        backend_->interrupt();
}

size_t scheduler::cancel_timer(
    const std::shared_ptr<timer_state>& timer) NOEXCEPT
{
    std::unique_lock lock{ mutex_ };
    unqueue_timer(timer);
    const auto count = timer->waiters.size();
    for (auto& waiter: timer->waiters)
        queue_.push_back(to_completion(std::move(waiter),
            error::operation_canceled));

    timer->waiters.clear();
    if (is_nonzero(count))
        wake();

    return count;
}

size_t scheduler::expire_timer(const std::shared_ptr<timer_state>& timer,
    clock::time_point expiry) NOEXCEPT
{
    std::unique_lock lock{ mutex_ };
    unqueue_timer(timer);
    const auto count = timer->waiters.size();
    for (auto& waiter: timer->waiters)
        queue_.push_back(to_completion(std::move(waiter),
            error::operation_canceled));

    timer->waiters.clear();
    timer->expiry = expiry;
    if (is_nonzero(count))
        wake();

    return count;
}

// private
handler scheduler::to_completion(timer_state::waiter&& waiter,
    const code& ec) NOEXCEPT
{
    return [waiter = std::move(waiter), ec]() NOEXCEPT
    {
        auto handler = waiter.handler;
        waiter.executor.post([handler, ec]() NOEXCEPT
        {
            handler(ec);
        });
    };
}

// private
void scheduler::unqueue_timer(
    const std::shared_ptr<timer_state>& timer) NOEXCEPT
{
    if (!timer->queued)
        return;

    const auto [first, last] = timers_.equal_range(timer->expiry);
    for (auto it = first; it != last; ++it)
    {
        if (it->second == timer)
        {
            timers_.erase(it);
            break;
        }
    }

    timer->queued = false;
}

// private
timeout scheduler::next_timeout() const NOEXCEPT
{
    if (timers_.empty())
        return {};

    const auto now = clock::now();
    const auto first = timers_.begin()->first;
    return first > now ? first - now : clock::duration::zero();
}

// private
void scheduler::expire_timers() NOEXCEPT
{
    const auto now = clock::now();
    while (!timers_.empty() && timers_.begin()->first <= now)
    {
        const auto timer = timers_.begin()->second;
        timers_.erase(timers_.begin());
        timer->queued = false;
        for (auto& waiter: timer->waiters)
            queue_.push_back(to_completion(std::move(waiter),
                error::success));

        timer->waiters.clear();
    }
}

// Resolver.
// ----------------------------------------------------------------------------

void scheduler::resolve(handler&& job) NOEXCEPT
{
    {
        std::unique_lock lock{ resolve_mutex_ };
        resolve_queue_.push_back(std::move(job));
        if (!resolver_.joinable())
            resolver_ = std::thread{ [this]() NOEXCEPT { run_resolver(); } };
    }

    resolve_condition_.notify_one();
}

// private
void scheduler::run_resolver() NOEXCEPT
{
    while (true)
    {
        handler job{};
        {
            std::unique_lock lock{ resolve_mutex_ };
            resolve_condition_.wait(lock, [this]() NOEXCEPT
            {
                return resolve_stop_ || !resolve_queue_.empty();
            });

            if (resolve_queue_.empty())
                return;

            job = std::move(resolve_queue_.front());
            resolve_queue_.pop_front();
        }

        job();
    }
}

// strand_state
// ----------------------------------------------------------------------------

strand_state::strand_state(io::context& context) NOEXCEPT
  : context_(context)
{
}

void strand_state::post(handler&& handler) NOEXCEPT
{
    {
        std::unique_lock lock{ mutex_ };
        queue_.push_back(std::move(handler));
        if (scheduled_)
            return;

        scheduled_ = true;
    }

    context_.post([self = shared_from_this()]() NOEXCEPT
    {
        self->run();
    });
}

bool strand_state::running_in_this_thread() const NOEXCEPT
{
    return running_strand == this;
}

// private
// The handlers queued when the run starts are run, then the strand is
// rescheduled if more were posted, so that other work interleaves.
void strand_state::run() NOEXCEPT
{
    const running_scope scope{ running_strand,
        static_cast<const strand_state*>(this) };

    std::deque<handler> batch{};
    {
        std::unique_lock lock{ mutex_ };
        batch.swap(queue_);
    }

    for (auto& next: batch)
        next();

    {
        std::unique_lock lock{ mutex_ };
        if (queue_.empty())
        {
            scheduled_ = false;
            return;
        }
    }

    context_.post([self = shared_from_this()]() NOEXCEPT
    {
        self->run();
    });
}

BC_POP_WARNING()

} // namespace detail
} // namespace io
} // namespace network
} // namespace libbitcoin
