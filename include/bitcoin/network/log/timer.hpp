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
#ifndef LIBBITCOIN_NETWORK_LOG_TIMER_HPP
#define LIBBITCOIN_NETWORK_LOG_TIMER_HPP

#include <chrono>
#include <time.h>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

// Based on: github.com/picanumber/bureaucrat/blob/master/time_lapse.h
// boost::timer::auto_cpu_timer requires the boost timer lib dependency.

/// Thread safe, non-virtual.
/// Class to measure the execution time of a callable.
template <typename Time = milliseconds, class Clock = steady_clock>
struct timer final
{
    /// Returns the duration (in chrono's type system) of the elapsed time.
    template <typename Function, typename ...Args>
    static Time duration(const Function& func, Args&&... args) NOEXCEPT
    {
        auto start = Clock::now();
        func(std::forward<Args>(args)...);
        return std::chrono::duration_cast<Time>(Clock::now() - start);
    }

    /// Returns the quantity (count) of the elapsed time as TimeT units.
    template <typename Function, typename ...Args>
    static typename Time::rep execution(const Function& func,
        Args&&... args) NOEXCEPT
    {
        return duration(func, std::forward<Args>(args)...).count();
    }
};

/// Current zulu (utc) time using the wall clock, as time_t.
BCT_API time_t zulu_time() NOEXCEPT;

/// Current zulu (utc) time using the wall clock, cast to uint32_t.
BCT_API uint32_t unix_time() NOEXCEPT;

/// Specified zulu (utc) time as local time: "yyyy-mm-ddThh:mm:ssL".
BCT_API std::string BCT_API format_local_time(time_t zulu) NOEXCEPT;

/// Specified zulu (utc) time as RFC 3339 utc time:  "yyyy-mm-ddThh:mm:ssZ".
BCT_API std::string BCT_API format_zulu_time(time_t zulu) NOEXCEPT;

} // namespace network
} // namespace libbitcoin

#endif
