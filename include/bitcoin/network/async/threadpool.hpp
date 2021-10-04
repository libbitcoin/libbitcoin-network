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

///This class and the asio service it exposes are thread safe.
/// A collection of threads which can be passed operations through io_service.
class BCT_API threadpool
  : system::noncopyable
{
public:

    /// Threadpool constructor, spawns the specified number of threads.
     threadpool(size_t number_threads=0,
        thread_priority priority=thread_priority::normal);

    virtual ~threadpool();

    /// There are no threads configured in the threadpool.
    bool empty() const;

    /// The number of threads configured in the threadpool.
    size_t size() const;

    /// Destroy the work keep-alive, allowing threads be joined, non-blocking.
    /// Safe to call from any thread. Caller should next invoke join.
    /// Threadpool cannot be restarted following a call to shutdown.
    void shutdown();

    /// Wait for all threads in the pool to terminate.
    /// Safe to call from any thread not in the threadpool.
    void join();

    /// Underlying boost::io_service object.
    asio::service& service();

    /// Underlying boost::io_service object.
    const asio::service& service() const;

private:
    // This is thread safe.
    asio::service service_;

    // These are protected by mutex.

    std::vector<thread> threads_;
    mutable upgrade_mutex threads_mutex_;

    std::shared_ptr<asio::service::work> work_;
    mutable upgrade_mutex work_mutex_;
};

} // namespace network
} // namespace libbitcoin

#endif

