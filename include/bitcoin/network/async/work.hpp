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
#ifndef LIBBITCOIN_NETWORK_ASYNC_WORK_HPP
#define LIBBITCOIN_NETWORK_ASYNC_WORK_HPP

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/async/sequencer.hpp>
#include <bitcoin/network/async/threadpool.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// work->()
/// work->[strand|service|sequence->service]

/// This  class is thread safe.
/// boost asio class wrapper to enable work heap management.
class BCT_API work
  : system::noncopyable
{
public:
    typedef std::shared_ptr<work> ptr;

    /// Create an instance.
    work(threadpool& pool, const std::string& name);

    /// Local execution for any operation, equivalent to std::bind.
    template <typename Handler, typename... Args>
    static void bound(Handler&& handler, Args&&... args)
    {
        BIND_HANDLER(handler, args)();
    }

    /// Concurrent execution for any operation.
    template <typename Handler, typename... Args>
    void concurrent(Handler&& handler, Args&&... args)
    {
        // TODO: io_context::post is deprecated, use post.
        // Service post ensures the job does not execute in the current thread.
        service_.post(BIND_HANDLER(handler, args));
    }

    /// Sequential execution for synchronous operations.
    template <typename Handler, typename... Args>
    void ordered(Handler&& handler, Args&&... args)
    {
        // Use a strand to prevent concurrency and post vs. dispatch to ensure
        // that the job is not executed in the current thread.
        strand_.post(BIND_HANDLER(handler, args));
    }

    /// Non-concurrent execution for synchronous operations.
    template <typename Handler, typename... Args>
    void unordered(Handler&& handler, Args&&... args)
    {
        // TODO: io_context::post is deprecated, use post.
        // TODO: io_context::strand::wrap is deprecated, use bind_executor.
        // Use a strand wrapper to prevent concurrency and a service post
        // to deny ordering while ensuring execution on another thread.
        service_.post(strand_.wrap(BIND_HANDLER(handler, args)));
    }

    /// Begin sequential execution for a set of asynchronous operations.
    /// The operation will be queued until the lock is free and then executed.
    template <typename Handler, typename... Args>
    void lock(Handler&& handler, Args&&... args)
    {
        // Use a sequence to track the asynchronous operation to completion,
        // ensuring each asynchronous op executes independently and in order.
        sequence_.lock(BIND_HANDLER(handler, args));
    }

    /// Complete sequential execution.
    void unlock()
    {
        sequence_.unlock();
    }

private:
    // These are thread safe.
    const std::string name_;
    asio::service& service_;
    asio::service::strand strand_;
    sequencer sequence_;
};

} // namespace network
} // namespace libbitcoin

#endif
