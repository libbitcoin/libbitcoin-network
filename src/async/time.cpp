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
#include <bitcoin/network/async/time.hpp>

#include <chrono>
#include <format>
#include <string>

namespace libbitcoin {
namespace network {

// Time aliases
using hours = std::chrono::hours;
using minutes = std::chrono::minutes;
using seconds = std::chrono::seconds;
using milliseconds = std::chrono::milliseconds;
using microseconds = std::chrono::microseconds;
using nanoseconds = std::chrono::nanoseconds;

using steady_clock = std::chrono::steady_clock;
using fine_clock = std::chrono::high_resolution_clock;
using wall_clock = std::chrono::system_clock;

// Current UTC time as time_t.
time_t zulu_time() NOEXCEPT
{
    return wall_clock::to_time_t(wall_clock::now());
}

// Current UTC time as uint32_t (Unix timestamp).
uint32_t unix_time() NOEXCEPT
{
    BC_PUSH_WARNING(NO_CASTS_FOR_ARITHMETIC_CONVERSION)
    return static_cast<uint32_t>(zulu_time());
    BC_POP_WARNING()
}

// Format time as local time: "yyyy-mm-ddThh:mm:ss".
std::string format_local_time(time_t time) NOEXCEPT
{
    try
    {
        using namespace std::chrono;
        const auto point = wall_clock::from_time_t(time);
        const auto trunc = time_point_cast<seconds>(point);
        const auto zoned = zoned_seconds{ current_zone(), trunc };
        return std::format("{:%FT%T}", zoned);
    }
    catch (...)
    {
        return {};
    }
}

// Format time as RFC 3339 UTC: "yyyy-mm-ddThh:mm:ssZ".
std::string format_zulu_time(time_t time) NOEXCEPT
{
    try
    {
        using namespace std::chrono;
        const auto point = wall_clock::from_time_t(time);
        const auto trunc = time_point_cast<seconds>(point);
        const auto zoned = zoned_seconds{ "UTC", trunc };
        return std::format("{:%FT%TZ}", zoned);
    }
    catch (...)
    {
        return {};
    }
}

// Format time as RFC 7231 UTC: "Day, DD Mon YYYY HH:MM:SS GMT".
std::string format_http_time(time_t time) NOEXCEPT
{
    try
    {
        using namespace std::chrono;
        const auto point = wall_clock::from_time_t(time);
        const auto trunc = time_point_cast<seconds>(point);
        const auto zoned = zoned_seconds{ "UTC", trunc };
        return std::format("{:%a, %d %b %Y %H:%M:%S GMT}", zoned);
    }
    catch (...)
    {
        return {};
    }
}

} // namespace network
} // namespace libbitcoin
