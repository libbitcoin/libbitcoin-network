/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_ASYNC_TIME_HPP
#define LIBBITCOIN_NETWORK_ASYNC_TIME_HPP

#include <chrono>
#include <cstddef>
#include <time.h>
#include <string>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// Time aliases.
typedef std::chrono::hours hours;
typedef std::chrono::minutes minutes;
typedef std::chrono::seconds seconds;
typedef std::chrono::milliseconds milliseconds;
typedef std::chrono::microseconds microseconds;
typedef std::chrono::nanoseconds nanoseconds;

/// Use steady_clock for continuity, wall_clock for time of day.
typedef std::chrono::steady_clock steady_clock;
typedef steady_clock::duration duration;
typedef steady_clock::time_point time_point;
typedef std::chrono::system_clock wall_clock;

/// Current zulu (utc) time using the wall clock.
BCT_API time_t zulu_time() NOEXCEPT;

/// Current local time using the wall clock, false and empty string on failure.
BCT_API bool local_time(tm& out_local, time_t zulu) NOEXCEPT;

/// Standard date-time string, e.g. Sun Oct 17 04:41:13 2010, locale dependent.
BCT_API std::string local_time() NOEXCEPT;

} // namespace network
} // namespace libbitcoin

#endif
