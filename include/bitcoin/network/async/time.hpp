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
#ifndef LIBBITCOIN_NETWORK_ASYNC_TIME_HPP
#define LIBBITCOIN_NETWORK_ASYNC_TIME_HPP

#include <chrono>
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

/// Use steady_clock for continuity, not time.
typedef std::chrono::steady_clock steady_clock;

/// Use fine_clock for high resolution, not time.
typedef std::chrono::high_resolution_clock fine_clock;

/// Use wall_clock for time of day.
/// C++20: std::chrono::system_clock measures Unix Time.
typedef std::chrono::system_clock wall_clock;

} // namespace network
} // namespace libbitcoin

#endif
