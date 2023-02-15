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
#include <time.h>
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

/// Use steady_clock for continuity.
typedef std::chrono::steady_clock steady_clock;
typedef steady_clock::duration duration;
typedef steady_clock::time_point time_point;

/// Use wall_clock for time of day.
/// C++20: std::chrono::system_clock measures Unix Time.
typedef std::chrono::system_clock wall_clock;

/// Current zulu (utc) time using the wall clock, as time_t.
BCT_API time_t zulu_time() NOEXCEPT;

/// Current zulu (utc) time using the wall clock, cast to uint32_t.
BCT_API uint32_t unix_time() NOEXCEPT;

/// Current local date-time text, locale dependent (Sun Oct 17 04:41:13 2010).
BCT_API std::string local_time() NOEXCEPT;

/// Specified zulu (utc) time as, conversion to local date-time text.
BCT_API std::string BCT_API to_local_time(uint32_t zulu) NOEXCEPT;

} // namespace network
} // namespace libbitcoin

#endif
