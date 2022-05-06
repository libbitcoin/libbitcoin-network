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
#ifndef LIBBITCOIN_NETWORK_ASYNC_THREADPOOL_HPP
#define LIBBITCOIN_NETWORK_ASYNC_THREADPOOL_HPP

#include <cstddef>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/async/thread.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

// TODO: investigate boost::threadpool.

/// Not thread safe.
/// A collection of threads that share an asio I/O context (service).
class BCT_API threadpool
  : system::noncopyable
{
public:
    /// Threadpool constructor, initializes the specified number of threads.
    threadpool(size_t number_threads=one,
        thread_priority priority=thread_priority::normal);

    /// Stop and join threads.
    virtual ~threadpool();

    /// Destroy the work keep-alive. Safe to call from any thread.
    /// Allows threads to join when all outstanding work is complete.
    void stop();

    /// Block until all threads in the pool terminate.
    /// Safe to call from any thread not in the threadpool.
    /// Returns false if called from within threadpool (would deadlock).
    bool join();

    /// Non-const underlying boost::io_service object (thread safe).
    asio::io_context& service();

private:
    // This is thread safe.
    asio::io_context service_;

    // These are not thread safe.
    std::vector<thread> threads_;
    boost::asio::executor_work_guard<asio::executor_type> work_;
};

} // namespace network
} // namespace libbitcoin

#endif
