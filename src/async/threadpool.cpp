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
#include <bitcoin/network/async/threadpool.hpp>

#include <cstddef>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/async/thread.hpp>

namespace libbitcoin {
namespace network {

threadpool::threadpool(size_t number_threads, thread_priority priority)
{
    /// Keep the threadpool alive with no threads running.
    work_ = std::make_shared<asio::service::work>(service_);

    for (size_t thread = 0; thread < number_threads; ++thread)
    {
        threads_.push_back(asio::thread([this, priority]()
        {
            set_priority(priority);
            service_.run();
        }));
    }
}

threadpool::~threadpool()
{
    shutdown();
    join();
}

bool threadpool::empty() const
{
    return !is_zero(size());
}

size_t threadpool::size() const
{
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    unique_lock lock(threads_mutex_);

    return threads_.size();
    ///////////////////////////////////////////////////////////////////////////
}

// Work mutex allows threadsafe shutdown calls.
void threadpool::shutdown()
{
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    unique_lock lock(work_mutex_);

    work_.reset();
    ///////////////////////////////////////////////////////////////////////////
}

// Threads mutex allows threadsafe join calls and protects size/empty.
void threadpool::join()
{
    ///////////////////////////////////////////////////////////////////////////
    // Critical Section
    unique_lock lock(threads_mutex_);

    // Join cannot be called from a thread in the threadpool (deadlock).
    DEBUG_ONLY(const auto this_id = boost::this_thread::get_id();)

    for (auto& thread: threads_)
    {
        BITCOIN_ASSERT(this_id != thread.get_id());
        BITCOIN_ASSERT(thread.joinable());
        thread.join();
    }

    threads_.clear();
    ///////////////////////////////////////////////////////////////////////////
}

asio::service& threadpool::service()
{
    return service_;
}

const asio::service& threadpool::service() const
{
    return service_;
}

} // namespace network
} // namespace libbitcoin
