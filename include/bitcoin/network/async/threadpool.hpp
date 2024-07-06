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
#ifndef LIBBITCOIN_NETWORK_ASYNC_THREADPOOL_HPP
#define LIBBITCOIN_NETWORK_ASYNC_THREADPOOL_HPP

#include <thread>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/async/thread.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

// TODO: investigate boost::threadpool.

/// Not thread safe, non-virtual.
/// A collection of threads that share an asio I/O context (service).
class BCT_API threadpool final
{
public:
    DELETE_COPY_MOVE(threadpool);

    /// Threadpool constructor, initializes the specified number of threads.
    threadpool(size_t number_threads=one,
        thread_priority priority=thread_priority::normal) NOEXCEPT;

    /// Stop and join threads.
    ~threadpool() NOEXCEPT;

    /// Destroy the work keep-alive. Safe to call from any thread.
    /// Allows threads to join when all outstanding work is complete.
    void stop() NOEXCEPT;

    /// Block until all threads in the pool terminate.
    /// Safe to call from any thread not in the threadpool.
    /// Returns false if called from within threadpool (would deadlock).
    bool join() NOEXCEPT;

    /// Non-const underlying boost::io_service object (thread safe).
    asio::io_context& service() NOEXCEPT;

private:
    using work_guard = boost::asio::executor_work_guard<asio::executor_type>;
    static inline work_guard keep_alive(asio::io_context& service) NOEXCEPT;

    // This is thread safe.
    asio::io_context service_{};

    // These are not thread safe.
    std_vector<std::thread> threads_{};
    work_guard work_;
};

} // namespace network
} // namespace libbitcoin

#endif
