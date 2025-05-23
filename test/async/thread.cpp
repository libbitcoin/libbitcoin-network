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
#include "../test.hpp"

#if defined(HAVE_MSC)
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

BOOST_AUTO_TEST_SUITE(thread_tests)

static int get_thread_priority_test()
{
#if defined(HAVE_MSC)
    return GetThreadPriority(GetCurrentThread());
#elif defined(PRIO_THREAD)
    return getpriority(PRIO_LWP, pthread_self());
#else
    return getpriority(PRIO_PROCESS, getpid());
#endif
}

void set_thread_priority_test(int priority)
{
#if defined(HAVE_MSC)
    SetThreadPriority(GetCurrentThread(), priority);
#elif defined(PRIO_THREAD)
    setpriority(PRIO_THREAD, pthread_self(), priority);
#else
    setpriority(PRIO_PROCESS, getpid(), priority);
#endif
}

#ifdef HAVE_MSC

BOOST_AUTO_TEST_CASE(thread__set_thread_priorites__all__set_as_expected)
{
    // Save so we can restore at the end of this test case.
    const int save = get_thread_priority_test();

    set_priority(thread_priority::highest);
    BOOST_REQUIRE_EQUAL(THREAD_PRIORITY_HIGHEST, get_thread_priority_test());
    set_priority(thread_priority::high);
    BOOST_REQUIRE_EQUAL(THREAD_PRIORITY_ABOVE_NORMAL, get_thread_priority_test());
    set_priority(thread_priority::normal);
    BOOST_REQUIRE_EQUAL(THREAD_PRIORITY_NORMAL, get_thread_priority_test());
    set_priority(thread_priority::low);
    BOOST_REQUIRE_EQUAL(THREAD_PRIORITY_BELOW_NORMAL, get_thread_priority_test());
    set_priority(thread_priority::lowest);
    BOOST_REQUIRE_EQUAL(THREAD_PRIORITY_LOWEST, get_thread_priority_test());

    // Restore and verify test execution thread priority to minimize side effect.
    set_thread_priority_test(save);
    BOOST_REQUIRE_EQUAL(save, get_thread_priority_test());
}

#else

BOOST_AUTO_TEST_CASE(thread__set_thread_priorites__all__set_as_expected)
{
    // Save so we can restore at the end of this test case.
    const int save = get_thread_priority_test();

    // Haven't had any luck matching the set and get priority calls as in win.
    BOOST_REQUIRE_NO_THROW(set_priority(thread_priority::highest));
    BOOST_REQUIRE_NO_THROW(set_priority(thread_priority::high));
    BOOST_REQUIRE_NO_THROW(set_priority(thread_priority::normal));
    BOOST_REQUIRE_NO_THROW(set_priority(thread_priority::low));
    BOOST_REQUIRE_NO_THROW(set_priority(thread_priority::lowest));

    // Restore and verify test execution thread priority to minimize side effect.
    set_thread_priority_test(save);
}

#endif

BOOST_AUTO_TEST_SUITE_END()
