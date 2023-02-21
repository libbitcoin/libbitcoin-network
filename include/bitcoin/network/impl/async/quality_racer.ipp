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
#ifndef LIBBITCOIN_NETWORK_ASYNC_QUALITY_RACER_IPP
#define LIBBITCOIN_NETWORK_ASYNC_QUALITY_RACER_IPP

#include <memory>
#include <tuple>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

template <size_t Size, typename... Args>
quality_racer<Size, Args...>::
quality_racer() NOEXCEPT
{
}

template <size_t Size, typename... Args>
quality_racer<Size, Args...>::
~quality_racer() NOEXCEPT
{
    BC_ASSERT_MSG(!running() && !complete_, "deleting running quality_racer");
}

template <size_t Size, typename... Args>
bool quality_racer<Size, Args...>::
running() const NOEXCEPT
{
    return to_bool(runners_);
}

template <size_t Size, typename... Args>
bool quality_racer<Size, Args...>::
start(handler&& complete) NOEXCEPT
{
    // bad lock?
    if (running())
        return false;

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    complete_ = std::make_shared<handler>(std::forward<handler>(complete));
    BC_POP_WARNING()

    runners_ = Size;
    return true;
}

template <size_t Size, typename... Args>
bool quality_racer<Size, Args...>::
finish(const Args&... args) NOEXCEPT
{
    // bad knock?
    if (!running())
        return false;

    // Capture parameter pack as tuple of copied arguments.
    if (is_winner())
        args_ = std::tuple<Args...>(args...);

    // bad knock or invoke?
    return !is_loser() || invoke();
}

// private
// ----------------------------------------------------------------------------

template <size_t Size, typename... Args>
bool quality_racer<Size, Args...>::
is_winner() const NOEXCEPT
{
    return runners_ == Size;
}

template <size_t Size, typename... Args>
bool quality_racer<Size, Args...>::
is_loser() NOEXCEPT
{
    // logic error, too many knocks.
    if (!running())
        return false;

    runners_ = sub1(runners_);
    return !running();
}

template <size_t Size, typename... Args>
bool quality_racer<Size, Args...>::
invoke() NOEXCEPT
{
    // logic error, knock by not running.
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
void quality_racer<Size, Args...>::
invoker(const handler& complete, const packed& args, unpack<Index...>) NOEXCEPT
{
    // Expand tuple into parameter pack.
    complete(std::get<Index>(args)...);
}

} // namespace network
} // namespace libbitcoin

#endif
