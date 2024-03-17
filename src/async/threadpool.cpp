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
#include <bitcoin/network/async/threadpool.hpp>

#include <thread>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/async/thread.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

// Work keeps the threadpool alive when there are no threads running.
threadpool::work_guard threadpool::keep_alive(asio::io_context& service) NOEXCEPT
{
    // If make_work_guard throws, application will abort at startup.
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return boost::asio::make_work_guard(service);
    BC_POP_WARNING()
}

// The run() function blocks until all work has finished and there are no
// more handlers to be dispatched, or until the io_context has been stopped.
threadpool::threadpool(size_t number_threads, thread_priority priority) NOEXCEPT
  : work_(keep_alive(service_))
{
    for (size_t thread = 0; thread < number_threads; ++thread)
    {
        threads_.push_back(std::thread([this, priority]() NOEXCEPT
        {
            set_priority(priority);

            // If service.run throws, application will abort at startup.
            BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
            service_.run();
            BC_POP_WARNING()
        }));
    }
}

threadpool::~threadpool() NOEXCEPT
{
    stop();
    join();
}

// Stopping the I/O service would abandon unfinished operations without
// permitting ready handlers to be dispatched. More information here:
// www.boost.org/doc/libs/1_69_0/doc/html/boost_asio/reference/io_context.html#
// boost_asio.reference.io_context.stopping_the_io_context_from_running_out_of_work
void threadpool::stop() NOEXCEPT
{
    // Clear the work keep-alive.
    // Allows all operations and handlers to finish normally.
    work_.reset();
}

bool threadpool::join() NOEXCEPT
{
    const auto this_id = std::this_thread::get_id();

    for (auto& thread: threads_)
    {
        // Thread must be joinable.
        if (!thread.joinable())
            return false;

        // Join cannot be called from a thread in the threadpool (deadlock).
        if (this_id == thread.get_id())
            return false;

        // Join should not throw given deadlock guard above, but just in case.
        try
        {
            thread.join();
        }
        catch (std::exception&)
        {
            return false;
        }
    }

    threads_.clear();
    return true;
}

asio::io_context& threadpool::service() NOEXCEPT
{
    return service_;
}

} // namespace network
} // namespace libbitcoin
