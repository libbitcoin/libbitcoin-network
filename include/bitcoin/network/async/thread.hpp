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
#ifndef LIBBITCOIN_NETWORK_ASYNC_THREAD_HPP
#define LIBBITCOIN_NETWORK_ASYNC_THREAD_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <boost/thread.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

////// Adapted from: stackoverflow.com/a/18298965/1172329
////#ifndef thread_local
////    #if (__STDC_VERSION__ >= 201112) && (!defined __STDC_NO_THREADS__)
////        #define thread_local _Thread_local
////    #elif defined(_MSC_VER)
////        #define thread_local __declspec(thread)
////    #elif defined(__GNUC__)
////        #define thread_local __thread
////    #else
////        #error "Cannot define thread_local"
////    #endif
////#endif

enum class thread_priority
{
    high,
    normal,
    low,
    lowest
};

// Boost thread is used because of thread_specific_ptr limitation:
// stackoverflow.com/q/22448022/1172329
typedef boost::thread thread;

/// C++17: std::shared_mutex.
typedef boost::shared_mutex shared_mutex;

/// C++??: there is no upgradeable lock concept.
typedef boost::upgrade_mutex upgrade_mutex;

/// C++11: std::unique_lock
typedef boost::unique_lock<shared_mutex> unique_lock;

/// C++14: std::shared_lock
typedef boost::shared_lock<shared_mutex> shared_lock;

typedef std::shared_ptr<boost::shared_mutex> shared_mutex_ptr;
typedef std::shared_ptr<boost::upgrade_mutex> upgrade_mutex_ptr;

BCT_API void set_priority(thread_priority priority);
BCT_API thread_priority priority(bool priority);
BCT_API size_t thread_default(size_t configured);
BCT_API size_t thread_ceiling(size_t configured);
BCT_API size_t thread_floor(size_t configured);

} // namespace network
} // namespace libbitcoin

#endif
