/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#endif

#ifdef HAVE_MSC
    // A few more extreme values are not mapped.
    // #define THREAD_PRIORITY_LOWEST          -2
    // #define THREAD_PRIORITY_BELOW_NORMAL    -1
    // #define THREAD_PRIORITY_NORMAL           0
    // #define THREAD_PRIORITY_ABOVE_NORMAL     1
    // #define THREAD_PRIORITY_HIGHEST          2

    // The extreme "lowest" value is not mapped.
    // #define MEMORY_PRIORITY_VERY_LOW         1
    // #define MEMORY_PRIORITY_LOW              2
    // #define MEMORY_PRIORITY_MEDIUM           3
    // #define MEMORY_PRIORITY_BELOW_NORMAL     4
    // #define MEMORY_PRIORITY_NORMAL           5
#else
    #define THREAD_PRIORITY_HIGHEST             -20
    #define THREAD_PRIORITY_ABOVE_NORMAL        -2
    #define THREAD_PRIORITY_NORMAL               0
    #define THREAD_PRIORITY_BELOW_NORMAL         2
    #define THREAD_PRIORITY_LOWEST               20

    // no equivalent
    #define MEMORY_PRIORITY_NORMAL               0
    #define MEMORY_PRIORITY_BELOW_NORMAL         0
    #define MEMORY_PRIORITY_MEDIUM               0
    #define MEMORY_PRIORITY_LOW                  0
    #define MEMORY_PRIORITY_VERY_LOW             0
#endif

namespace libbitcoin {
namespace network {

#if defined (HAVE_MEMORY_PRIORITY)
// Privately map the class enum memory priority value to an integer.
static unsigned long get_memory_priority(memory_priority priority) NOEXCEPT
{
    switch (priority)
    {
        case memory_priority::lowest:
            return MEMORY_PRIORITY_VERY_LOW;
        case memory_priority::low:
            return MEMORY_PRIORITY_LOW;
        case memory_priority::medium:
            return MEMORY_PRIORITY_MEDIUM;
        case memory_priority::high:
            return MEMORY_PRIORITY_BELOW_NORMAL;
        default:
        case memory_priority::highest:
            return MEMORY_PRIORITY_NORMAL;
    }
}
void set_memory_priority(memory_priority priority) NOEXCEPT
{
    const auto prioritization = get_memory_priority(priority);

    // TODO: handle error conditions.
    MEMORY_PRIORITY_INFORMATION information{ prioritization };
    SetProcessInformation(GetCurrentProcess(), ProcessMemoryPriority,
        &information, sizeof(information));
}
#else
static int get_memory_priority(memory_priority) NOEXCEPT {}
void set_memory_priority(memory_priority) NOEXCEPT {}
#endif

// Privately map the class enum processing priority value to an integer.
static int get_processing_priority(processing_priority priority) NOEXCEPT
{
    switch (priority)
    {
        case processing_priority::lowest:
            return THREAD_PRIORITY_LOWEST;
        case processing_priority::low:
            return THREAD_PRIORITY_BELOW_NORMAL;
        case processing_priority::high:
            return THREAD_PRIORITY_ABOVE_NORMAL;
        case processing_priority::highest:
            return THREAD_PRIORITY_HIGHEST;
        default:
        case processing_priority::medium:
            return THREAD_PRIORITY_NORMAL;
    }
}

// Set the thread processing priority.
void set_processing_priority(processing_priority priority) NOEXCEPT
{
    // TODO: handle error conditions.
    // TODO: handle potential lack of PRIO_THREAD
    // TODO: use proper non-win32 priority levels.
    // TODO: Linux: pthread_setschedprio()
    // TOOD: macOS: somethign else.

    const auto prioritization = get_processing_priority(priority);

#if defined(HAVE_MSC)
    SetThreadPriority(GetCurrentThread(), prioritization);

#elif defined(PRIO_THREAD)
    // lore.kernel.org/lkml/1220278355.3866.21.camel@localhost.localdomain/
    setpriority(PRIO_THREAD, pthread_self(), prioritization);

#else
    // BUGBUG: This will set all threads in the process.
    // man7.org/linux/man-pages/man3/pthread_self.3.html
    setpriority(PRIO_PROCESS, getpid(), prioritization);
#endif
}

size_t cores() NOEXCEPT
{
    return std::max(std::thread::hardware_concurrency(), 1_u32);
}

} // namespace network
} // namespace libbitcoin
