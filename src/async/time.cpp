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
#include <bitcoin/network/async/time.hpp>

#include <time.h>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {

// local utilities
// ----------------------------------------------------------------------------
// These are complicated by lack of std::localtime/std::gmtime thread safety,
// and by the fact that msvc reverses the parameters and inverts the result.

inline bool local_time(tm& out_local, time_t time) NOEXCEPT
{
    // std::localtime not threadsafe due to static buffer, use localtime_s.
#ifdef HAVE_MSC
    // proprietary msvc implemention, parameters swapped, returns errno_t.
    return is_zero(localtime_s(&out_local, &time));
#else
    // C++11 implemention returns parameter pointer, nullptr implies failure.
    return !is_null(localtime_r(&time, &out_local));
#endif
}

inline bool zulu_time(tm& out_zulu, time_t time) NOEXCEPT
{
    // std::gmtime is not threadsafe due to static buffer, use gmtime_s.
#ifdef HAVE_MSC
    // proprietary msvc implemention, parameters swapped, returns errno_t.
    return is_zero(gmtime_s(&out_zulu, &time));
#else
    // C++11 implemention returns parameter pointer, nullptr implies failure.
    return !is_null(gmtime_r(&time , &out_zulu));
#endif
}

// published
// ----------------------------------------------------------------------------
// BUGBUG: en.wikipedia.org/wiki/Year_2038_problem

time_t zulu_time() NOEXCEPT
{
    const auto now = wall_clock::now();
    return wall_clock::to_time_t(now);
}

uint32_t unix_time() NOEXCEPT
{
    BC_PUSH_WARNING(NO_CASTS_FOR_ARITHMETIC_CONVERSION)
    return static_cast<uint32_t>(zulu_time());
    BC_POP_WARNING()
}

std::string format_local_time(time_t time) NOEXCEPT
{
    tm out{};
    if (!local_time(out, time))
        return "";

    constexpr auto format = "%FT%T";
    constexpr size_t size = std::size("yyyy-mm-dd hh:mm:ss");
    char buffer[size];

    // Returns number of characters, zero implies failure and undefined buffer.
    BC_PUSH_WARNING(NO_ARRAY_TO_POINTER_DECAY)
    return is_zero(std::strftime(buffer, size, format, &out)) ? "" : buffer;
    BC_POP_WARNING()
}

std::string format_zulu_time(time_t time) NOEXCEPT
{
    tm out{};
    if (!zulu_time(out, time))
        return "";

    // %FT%TZ writes RFC 3339 formatted utc time.
    constexpr auto format = "%FT%TZ";
    constexpr size_t size = std::size("yyyy-mm-ddThh:mm:ssZ");
    char buffer[size];

    // Returns number of characters, zero implies failure and undefined buffer.
    BC_PUSH_WARNING(NO_ARRAY_TO_POINTER_DECAY)
    return is_zero(std::strftime(buffer, size, format, &out)) ? "" : buffer;
    BC_POP_WARNING()
}

} // namespace network
} // namespace libbitcoin
