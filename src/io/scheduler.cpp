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
    shutdown();
}

void scheduler::shutdown() NOEXCEPT
{
    {
        std::unique_lock lock{ resolve_mutex_ };
        resolve_stop_ = true;
    }

    resolve_condition_.notify_all();
    if (resolver_.joinable())
        resolver_.join();

#if !defined(HAVE_APPLE)
    {
        std::unique_lock lock{ mutex_ };
        timer_stop_ = true;
    }

    timer_condition_.notify_all();
    if (timer_thread_.joinable())
        timer_thread_.join();
#endif

    // Destroyed handlers may queue more (e.g. a timer canceled on destruct).
    while (true)
    {
        std::deque<handler> handlers{};
        std::vector<timer_state::waiter> waiters{};
        {
            std::unique_lock lock{ mutex_ };
#if defined(HAVE_APPLE)
            handlers.swap(queue_);
#endif
            for (auto& entry: timers_)
            {
                auto& timer = *entry.second;
                std::move(timer.waiters.begin(), timer.waiters.end(),
                    std::back_inserter(waiters));
                timer.waiters.clear();
                timer.queued = false;
            }

            timers_.clear();
        }

#if !defined(HAVE_APPLE)
        const auto drained = backend_->drain();
#else
        constexpr auto drained = false;
#endif

        if (handlers.empty() && waiters.empty() && !drained)
            return;
    }
}

#if !defined(HAVE_APPLE)

// Threads wait on the backend together, each running the completions or
// posted handler that return it.
size_t scheduler::run() NOEXCEPT
{
    const running_scope scope{ running_scheduler, this };
    std::vector<handler> completions{};
    size_t count{};

    if (is_zero(outstanding_.load()))
    {
        stop();
        return count;
    }

    while (!stopped_.load())
    {
        completions.clear();

        // A wait returned by the stop interrupt has consumed it.
        if (backend_->wait({}, completions))
            signaled_.store(false);

        for (auto& completion: completions)
        {
            completion();
            completion = nullptr;
            ++count;
            work_finished();
        }
    }

    // A stop returns the waiters one after another.
    signal_stop();
    return count;
}

#else

size_t scheduler::run() NOEXCEPT
{
    const running_scope scope{ running_scheduler, this };
    std::vector<handler> completions{};
    size_t count{};

    std::unique_lock lock{ mutex_ };
    if (is_zero(outstanding_.load()))
    {
        stop_locked();
        return count;
    }

    while (!stopped_)
    {
        // Each pass over the queue yields once to the backend unless a thread
        // waits on it (and on the first timer, if any), so that posted work
        // cannot starve completions and timers.
        const auto poll = !queue_.empty() && is_zero(pass_) &&
            (is_zero(waiting_backend_) || (!timed_ && !timers_.empty()));

        if (!queue_.empty() && !poll)
        {
            if (is_nonzero(pass_))
                --pass_;

            auto next = std::move(queue_.front());
            queue_.pop_front();
            if (!queue_.empty())
                wake();

            // The handler (and its captures) is released before relocking,
            // as its destruction may reenter the scheduler.
            lock.unlock();
            next();
            next = nullptr;
            ++count;
            lock.lock();

            if (is_zero(--outstanding_))
                stop_locked();

            continue;
        }

        // One thread at a time waits on the backend, the others on the queue.
        if (is_zero(waiting_backend_))
        {
            timeout duration{};
            clock::time_point deadline{};
            auto timed = false;

            // One waiter is timed by the first timer (a poll does not sleep).
            if (poll)
            {
                duration = clock::duration::zero();
            }
            else if (!timers_.empty() &&
                (!timed_ || timers_.begin()->first < deadline_))
            {
                timed = true;
                timed_ = true;
                deadline = timers_.begin()->first;
                deadline_ = deadline;
                duration = next_timeout();
            }

            ++waiting_backend_;
            lock.unlock();

            completions.clear();
            backend_->wait(duration, completions);

            lock.lock();
            --waiting_backend_;
            if (timed && deadline_ == deadline)
                timed_ = false;

            for (auto& completion: completions)
                queue_.push_back(std::move(completion));

            expire_timers();
            pass_ = queue_.size();

            // A stop returns the backend waiters one after another.
            if (stopped_ && is_nonzero(waiting_backend_))
                backend_->interrupt();

            // The sole backend waiter runs the queue, so another takes over.
            if (!queue_.empty() && is_nonzero(waiting_))
                condition_.notify_one();

            continue;
        }

        ++waiting_;
        condition_.wait(lock);
        --waiting_;
    }

    return count;
}

#endif

void scheduler::stop() NOEXCEPT
{
    std::unique_lock lock{ mutex_ };
    stop_locked();
}

bool scheduler::stopped() const NOEXCEPT
{
    return stopped_.load();
}

void scheduler::restart() NOEXCEPT
{
    stopped_.store(false);
}

bool scheduler::running_in_this_thread() const NOEXCEPT
{
    return running_scheduler == this;
}

void scheduler::post(handler&& handler) NOEXCEPT
{
    ++outstanding_;
    complete(std::move(handler));
}

void scheduler::complete(handler&& handler) NOEXCEPT
{
#if !defined(HAVE_APPLE)
    backend_->post(std::move(handler));
#else
    std::unique_lock lock{ mutex_ };
    queue_.push_back(std::move(handler));
    wake();
#endif
}

void scheduler::work_started() NOEXCEPT
{
    ++outstanding_;
}

void scheduler::work_finished() NOEXCEPT
{
    if (is_zero(--outstanding_))
        stop();
}

detail::backend& scheduler::backend() NOEXCEPT
{
    return *backend_;
}

// private
void scheduler::enqueue(handler&& handler) NOEXCEPT
{
#if !defined(HAVE_APPLE)
    backend_->post(std::move(handler));
#else
    queue_.push_back(std::move(handler));
#endif
}

// private
void scheduler::wake() NOEXCEPT
{
#if defined(HAVE_APPLE)
    if (is_nonzero(waiting_))
        condition_.notify_one();
    else if (is_nonzero(waiting_backend_))
        backend_->interrupt();
#endif
}

// private
void scheduler::stop_locked() NOEXCEPT
{
    stopped_.store(true);

#if !defined(HAVE_APPLE)
    signal_stop();
#else
    condition_.notify_all();
    if (is_nonzero(waiting_backend_))
        backend_->interrupt();
#endif
}

#if !defined(HAVE_APPLE)

// private
void scheduler::signal_stop() NOEXCEPT
{
    if (!signaled_.exchange(true))
        backend_->interrupt();
}

#endif

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

#if !defined(HAVE_APPLE)
    // An earlier first timer shortens the timer thread wait.
    if (!timer_stop_ && !timer_thread_.joinable())
        timer_thread_ = std::thread{ [this]() NOEXCEPT { run_timers(); } };
    else if (position == timers_.begin())
        timer_condition_.notify_one();
#else
    // An earlier first timer shortens the timed backend wait.
    if (position == timers_.begin() && is_nonzero(waiting_backend_) &&
        (!timed_ || timer->expiry < deadline_))
        backend_->interrupt();
#endif
}

size_t scheduler::cancel_timer(
    const std::shared_ptr<timer_state>& timer) NOEXCEPT
{
    std::unique_lock lock{ mutex_ };
    unqueue_timer(timer);
    const auto count = timer->waiters.size();
    for (auto& waiter: timer->waiters)
        enqueue(to_completion(std::move(waiter), error::operation_canceled));

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
        enqueue(to_completion(std::move(waiter), error::operation_canceled));

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
        waiter.executor.dispatch([handler, ec]() NOEXCEPT
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
    if (timers_.empty())
        return;

    const auto now = clock::now();
    while (!timers_.empty() && timers_.begin()->first <= now)
    {
        const auto timer = timers_.begin()->second;
        timers_.erase(timers_.begin());
        timer->queued = false;
        for (auto& waiter: timer->waiters)
            enqueue(to_completion(std::move(waiter), error::success));

        timer->waiters.clear();
    }
}

#if !defined(HAVE_APPLE)

// private
void scheduler::run_timers() NOEXCEPT
{
    std::unique_lock lock{ mutex_ };
    while (!timer_stop_)
    {
        if (timers_.empty())
            timer_condition_.wait(lock);
        else
            timer_condition_.wait_until(lock, timers_.begin()->first);

        expire_timers();
    }
}

#endif

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

void strand_state::dispatch(handler&& handler) NOEXCEPT
{
    if (!context_.running_in_this_thread())
    {
        post(std::move(handler));
        return;
    }

    {
        std::unique_lock lock{ mutex_ };
        if (scheduled_)
        {
            queue_.push_back(std::move(handler));
            return;
        }

        scheduled_ = true;
    }

    {
        const running_scope scope{ running_strand,
            static_cast<const strand_state*>(this) };

        handler();
    }

    release();
}

bool strand_state::running_in_this_thread() const NOEXCEPT
{
    return running_strand == this;
}

// private
// Only the handlers queued when the run starts are run, so that other work
// interleaves.
void strand_state::run() NOEXCEPT
{
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
    }

    release();
}

// private
// The strand is rescheduled if handlers were posted during the run.
void strand_state::release() NOEXCEPT
{
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
