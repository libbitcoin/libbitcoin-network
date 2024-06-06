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
#ifndef LIBBITCOIN_NETWORK_ASYNC_RACES_RACE_SPEED_IPP
#define LIBBITCOIN_NETWORK_ASYNC_RACES_RACE_SPEED_IPP

#include <memory>
#include <tuple>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

template <size_t Size, typename... Args>
race_speed<Size, Args...>::
race_speed() NOEXCEPT
{
}

template <size_t Size, typename... Args>
race_speed<Size, Args...>::
~race_speed() NOEXCEPT
{
    BC_ASSERT_MSG(!running() && !complete_, "deleting running race_speed");
}

template <size_t Size, typename... Args>
inline bool race_speed<Size, Args...>::
running() const NOEXCEPT
{
    return to_bool(runners_);
}

template <size_t Size, typename... Args>
bool race_speed<Size, Args...>::
start(handler&& complete) NOEXCEPT
{
    // false implies logic error.
    if (running())
        return false;

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    complete_ = std::make_shared<handler>(std::forward<handler>(complete));
    BC_POP_WARNING()

    runners_ = Size;
    return true;
}

template <size_t Size, typename... Args>
bool race_speed<Size, Args...>::
finish(const Args&... args) NOEXCEPT
{
    // false implies logic error.
    if (!running())
        return false;

    // Capture parameter pack as tuple of copied arguments.
    const auto winner = is_winner();

    // Save args for winner (first to finish).
    if (winner)
        args_ = std::tuple<Args...>(args...);

    // false implies logic error.
    return invoke() && winner;
}

// private
// ----------------------------------------------------------------------------

template <size_t Size, typename... Args>
bool race_speed<Size, Args...>::
is_winner() const NOEXCEPT
{
    // Return is winner (first to finish).
    return runners_ == Size;
}

template <size_t Size, typename... Args>
bool race_speed<Size, Args...>::
invoke() NOEXCEPT
{
    // false implies logic error.
    if (!running())
        return false;

    --runners_;
    if (running())
        return true;

    // false implies logic error.
    if (!complete_)
        return false;

    // Invoke completion handler.
    invoker(*complete_, args_, sequence{});

    // Clear all resources.
    complete_.reset();
    args_ = {};
    return true;
}

template <size_t Size, typename... Args>
template<size_t... Index>
void race_speed<Size, Args...>::
invoker(const handler& complete, const packed& args, unpack<Index...>) NOEXCEPT
{
    // Expand tuple into parameter pack.
    complete(std::get<Index>(args)...);
}

} // namespace network
} // namespace libbitcoin

#endif
