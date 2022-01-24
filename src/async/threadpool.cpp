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

// Stopping the I/O service will abandon unfinished operations without
// permitting ready handlers to be dispatched. More information here:
// www.boost.org/doc/libs/1_69_0/doc/html/boost_asio/reference/io_context.html#
// boost_asio.reference.io_context.stopping_the_io_context_from_running_out_of_work

// The run() function blocks until all work has finished and there are no
// more handlers to be dispatched, or until the io_context has been stopped.

threadpool::threadpool(size_t number_threads, thread_priority priority)
  : work_(boost::asio::make_work_guard(service_))
{
    // Work keeps the threadpool alive when there are no threads running.

    for (size_t thread = 0; thread < number_threads; ++thread)
    {
        threads_.push_back(network::thread([this, priority]()
        {
            set_priority(priority);
            service_.run();
        }));
    }
}

threadpool::~threadpool()
{
    stop();
    join();
}

void threadpool::stop()
{
    // Clear the work keep-alive.
    // Allows all operations and handlers to finish normally.
    work_.reset();
}

void threadpool::join()
{
    // Join cannot be called from a thread in the threadpool (deadlock).
    BC_DEBUG_ONLY(const auto this_id = boost::this_thread::get_id();)

    for (auto& thread: threads_)
    {
        BC_ASSERT_MSG(thread.joinable(), "unjoinable deadlock");
        BC_ASSERT_MSG(this_id != thread.get_id(), "join deadlock");
        thread.join();
    }

    threads_.clear();
}

asio::io_context& threadpool::service()
{
    return service_;
}

} // namespace network
} // namespace libbitcoin
