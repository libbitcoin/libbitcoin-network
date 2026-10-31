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
/// guards; when it reaches zero the scheduler stops.
class scheduler
{
public:
    DELETE_COPY_MOVE(scheduler);

    scheduler() NOEXCEPT;
    ~scheduler() NOEXCEPT;

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

    void wake() NOEXCEPT;
    void stop_locked() NOEXCEPT;
    timeout next_timeout() const NOEXCEPT;
    void unqueue_timer(const std::shared_ptr<timer_state>& timer) NOEXCEPT;
    void expire_timers() NOEXCEPT;
    void run_resolver() NOEXCEPT;

    // These are protected by mutex_.
    mutable std::mutex mutex_{};
    std::condition_variable condition_{};
    std::deque<handler> queue_{};
    timers timers_{};
    size_t outstanding_{};
    size_t waiting_{};
    size_t pass_{};
    bool stopped_{};
    bool waiting_backend_{};

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
    bool running_in_this_thread() const NOEXCEPT;

private:
    void run() NOEXCEPT;

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
