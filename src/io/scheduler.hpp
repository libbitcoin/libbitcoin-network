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
#ifndef LIBBITCOIN_NETWORK_IO_SCHEDULER_HPP
#define LIBBITCOIN_NETWORK_IO_SCHEDULER_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/io/context.hpp>

#if defined(HAVE_APPLE) || defined(HAVE_BSD)
    #define HAVE_KQUEUE
#endif

namespace libbitcoin {
namespace network {
namespace io {
namespace detail {

class backend;

using clock = std::chrono::steady_clock;
using timeout = std::optional<clock::duration>;

/// The waiters of a timer, protected by the scheduler mutex.
struct timer_state
{
    struct waiter
    {
        io::executor executor;
        std::function<void(const code&)> handler;
    };

    clock::time_point expiry{};
    std::vector<waiter> waiters{};
    bool queued{};
};

/// The run queue, timer queue, platform backend and resolver thread of a
/// context. Work is counted from queued handlers, pending operations and work
/// guards; when it reaches zero the scheduler stops. Where the backend queues
/// posted handlers (windows, linux) it is the run queue and timers expire on a
/// timer thread; otherwise (apple) handlers are queued here and one thread at
/// a time waits on the backend.
class scheduler
{
public:
    DELETE_COPY_MOVE(scheduler);

    scheduler() NOEXCEPT;
    ~scheduler() NOEXCEPT;

    /// Stop the resolver and destroy queued handlers and timer waiters, whose
    /// destruction may use this scheduler (so it precedes member destruction).
    void shutdown() NOEXCEPT;

    size_t run() NOEXCEPT;
    void stop() NOEXCEPT;
    bool stopped() const NOEXCEPT;
    void restart() NOEXCEPT;
    bool running_in_this_thread() const NOEXCEPT;

    /// Queue a handler (counted as work until it has run).
    void post(handler&& handler) NOEXCEPT;

    /// Queue the completion of an operation counted by work_started().
    void complete(handler&& handler) NOEXCEPT;

    void work_started() NOEXCEPT;
    void work_finished() NOEXCEPT;

    /// Timers (each waiter is counted as work until it completes).
    void wait_timer(const std::shared_ptr<timer_state>& timer,
        timer_state::waiter&& waiter) NOEXCEPT;
    size_t cancel_timer(const std::shared_ptr<timer_state>& timer) NOEXCEPT;
    size_t expire_timer(const std::shared_ptr<timer_state>& timer,
        clock::time_point expiry) NOEXCEPT;

    /// Run a blocking job on the resolver thread (counted as work by caller).
    void resolve(handler&& job) NOEXCEPT;

    detail::backend& backend() NOEXCEPT;

private:
    using timers = std::multimap<clock::time_point,
        std::shared_ptr<timer_state>>;

    static handler to_completion(timer_state::waiter&& waiter,
        const code& ec) NOEXCEPT;

    void enqueue(handler&& handler) NOEXCEPT;
    void wake() NOEXCEPT;
    void stop_locked() NOEXCEPT;
    timeout next_timeout() const NOEXCEPT;
    void unqueue_timer(const std::shared_ptr<timer_state>& timer) NOEXCEPT;
    void expire_timers() NOEXCEPT;
    void run_resolver() NOEXCEPT;

#if !defined(HAVE_KQUEUE)
    void signal_stop() NOEXCEPT;
    void run_timers() NOEXCEPT;
#endif

    std::atomic<size_t> outstanding_{};
    std::atomic<bool> stopped_{};

#if !defined(HAVE_KQUEUE)
    // A stop interrupt is queued on the backend.
    std::atomic<bool> signaled_{};
#endif

    // These are protected by mutex_.
    mutable std::mutex mutex_{};
    timers timers_{};
#if !defined(HAVE_KQUEUE)
    std::condition_variable timer_condition_{};
    std::thread timer_thread_{};
    bool timer_stop_{};
#else
    std::condition_variable condition_{};
    std::deque<handler> queue_{};
    clock::time_point deadline_{};
    size_t waiting_{};
    size_t waiting_backend_{};
    size_t pass_{};
    bool timed_{};
#endif

    // These are protected by resolve_mutex_.
    std::mutex resolve_mutex_{};
    std::condition_variable resolve_condition_{};
    std::deque<handler> resolve_queue_{};
    std::thread resolver_{};
    bool resolve_stop_{};

    std::unique_ptr<detail::backend> backend_;
};

/// The state of a strand, scheduled on its context when holding handlers.
class strand_state
  : public std::enable_shared_from_this<strand_state>
{
public:
    strand_state(io::context& context) NOEXCEPT;

    void post(handler&& handler) NOEXCEPT;

    /// Run the handler now if on a thread of the context and the strand is
    /// idle, otherwise post it.
    void dispatch(handler&& handler) NOEXCEPT;

    bool running_in_this_thread() const NOEXCEPT;

private:
    void run() NOEXCEPT;
    void release() NOEXCEPT;

    io::context& context_;

    // These are protected by mutex_.
    std::mutex mutex_{};
    std::deque<handler> queue_{};
    bool scheduled_{};
};

} // namespace detail
} // namespace io
} // namespace network
} // namespace libbitcoin

#endif
