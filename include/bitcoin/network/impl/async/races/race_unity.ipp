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
#ifndef LIBBITCOIN_NETWORK_ASYNC_RACES_RACE_UNITY_IPP
#define LIBBITCOIN_NETWORK_ASYNC_RACES_RACE_UNITY_IPP

#include <memory>
#include <tuple>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

template <typename... Args>
race_unity<Args...>::
race_unity(size_t size) NOEXCEPT
  : size_(size)
{
}

template <typename... Args>
race_unity<Args...>::
~race_unity() NOEXCEPT
{
    BC_ASSERT_MSG(!running() && !complete_, "deleting running race_unity");
}

template <typename... Args>
inline bool race_unity<Args...>::
running() const NOEXCEPT
{
    return to_bool(runners_);
}

template <typename... Args>
bool race_unity<Args...>::
start(handler&& complete) NOEXCEPT
{
    // false implies logic error.
    if (running())
        return false;

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    complete_ = std::make_shared<handler>(std::forward<handler>(complete));
    BC_POP_WARNING()

    failure_ = false;
    runners_ = size_;
    return true;
}

template <typename... Args>
bool race_unity<Args...>::
finish(const Args&... args) NOEXCEPT
{
    // false implies logic error.
    if (!running())
        return false;

    // First argument (by convention) determines failure.
    // Capture parameter pack as tuple of copied arguments.
    auto values = std::tuple<Args...>(args...);
    const auto& ec = std::get<0>(values);
    const auto failer = set_failure(!!ec);

    // Save args for failer (first failure) or last success.
    const auto last = (runners_ == one);
    if (failer || (last && !failure_))
        args_ = std::move(values);

    // false invoke implies logic error.
    return invoke() && last && !failure_;
}

// private
// ----------------------------------------------------------------------------

template <typename... Args>
bool race_unity<Args...>::
set_failure(bool failure) NOEXCEPT
{
    // Return is failer (first to fail) and set failure.
    failure &= !failure_;
    failure_ |= failure;
    return failure;
}

template <typename... Args>
bool race_unity<Args...>::
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

template <typename... Args>
template<size_t... Index>
void race_unity<Args...>::
invoker(const handler& complete, const packed& args, unpack<Index...>) NOEXCEPT
{
    // Expand tuple into parameter pack.
    complete(std::get<Index>(args)...);
}

} // namespace network
} // namespace libbitcoin

#endif
