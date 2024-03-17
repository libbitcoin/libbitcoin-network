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
#include <bitcoin/network/async/thread.hpp>

#include <thread>

#ifdef HAVE_MSC
    #include <windows.h>
#else
    #include <unistd.h>
    #include <pthread.h>
    #include <sys/resource.h>
    #include <sys/types.h>
    #ifndef PRIO_MAX
        #define PRIO_MAX 20
    #endif
    #define THREAD_PRIORITY_ABOVE_NORMAL (-2)
    #define THREAD_PRIORITY_NORMAL 0
    #define THREAD_PRIORITY_BELOW_NORMAL 2
    #define THREAD_PRIORITY_LOWEST PRIO_MAX
#endif

namespace libbitcoin {
namespace network {

// Privately map the class enum thread priority value to an integer.
static int get_priority(thread_priority priority) NOEXCEPT
{
    switch (priority)
    {
        case thread_priority::lowest:
            return THREAD_PRIORITY_LOWEST;
        case thread_priority::low:
            return THREAD_PRIORITY_BELOW_NORMAL;
        case thread_priority::high:
            return THREAD_PRIORITY_ABOVE_NORMAL;
        default:
        case thread_priority::normal:
            return THREAD_PRIORITY_NORMAL;
    }
}

// Set the thread priority (or process if thread priority is not available).
void set_priority(thread_priority priority) NOEXCEPT
{
    const auto prioritization = get_priority(priority);

#if defined(HAVE_MSC)
    SetThreadPriority(GetCurrentThread(), prioritization);
#elif defined(PRIO_THREAD)
    setpriority(PRIO_THREAD, pthread_self(), prioritization);
#else
    setpriority(PRIO_PROCESS, getpid(), prioritization);
#endif
}

size_t cores() NOEXCEPT
{
    return std::max(std::thread::hardware_concurrency(), 1_u32);
}

} // namespace network
} // namespace libbitcoin
