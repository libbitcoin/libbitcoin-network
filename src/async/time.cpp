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
#include <bitcoin/network/async/time.hpp>

#include <time.h>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {

time_t zulu_time() NOEXCEPT
{
    const auto now = wall_clock::now();
    return wall_clock::to_time_t(now);
}

// BUGBUG: en.wikipedia.org/wiki/Year_2038_problem
uint32_t unix_time() NOEXCEPT
{
    BC_PUSH_WARNING(NO_STATIC_CAST)
    return static_cast<uint32_t>(zulu_time());
    BC_POP_WARNING()
}

// local
static bool local_time(tm& out_local, time_t zulu) NOEXCEPT
{
    // localtime not threadsafe due to static buffer return, use localtime_s.
#ifdef HAVE_MSC
    // proprietary msvc implemention, parameters swapped, returns errno_t.
    return localtime_s(&out_local, &zulu) == 0;
#else
    // C++11 implemention returns parameter pointer, nullptr implies failure.
    return localtime_r(&zulu, &out_local) != nullptr;
#endif
}

// local
static std::string local_time(time_t zulu) NOEXCEPT
{
    tm out_local{};
    if (!local_time(out_local, zulu))
        return "";

    // %c writes standard date and time string, e.g.
    // Sun Oct 17 04:41:13 2010 (locale dependent)
    static constexpr auto format = "%c";
    static constexpr size_t size = 25;
    char buffer[size];

    // std::strftime is required because gcc doesn't implement std::put_time.
    // Returns number of characters, zero implies failure and undefined buffer.
    BC_PUSH_WARNING(NO_ARRAY_TO_POINTER_DECAY)
    return is_zero(std::strftime(buffer, size, format, &out_local)) ? "" : buffer;
    BC_POP_WARNING()
}

std::string local_time() NOEXCEPT
{
    return local_time(zulu_time());
}

std::string to_local_time(uint32_t zulu) NOEXCEPT
{
    BC_PUSH_WARNING(NO_STATIC_CAST)
    return local_time(static_cast<time_t>(zulu));
    BC_POP_WARNING()
}

} // namespace network
} // namespace libbitcoin
