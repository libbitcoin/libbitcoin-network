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
#ifndef LIBBITCOIN_NETWORK_ASYNC_GATE_IPP
#define LIBBITCOIN_NETWORK_ASYNC_GATE_IPP

#include <memory>
#include <tuple>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

template <size_t Knocks, typename Handler, typename... Args>
gate_first<Knocks, Handler, Args...>::
gate_first() NOEXCEPT
{
}

template <size_t Knocks, typename Handler, typename... Args>
gate_first<Knocks, Handler, Args...>::
~gate_first() NOEXCEPT
{
    BC_ASSERT_MSG(!locked() && !handler_, "deleting locked gate");
}

template <size_t Knocks, typename Handler, typename... Args>
bool gate_first<Knocks, Handler, Args...>::
locked() const NOEXCEPT
{
    return to_bool(count_);
}

template <size_t Knocks, typename Handler, typename... Args>
bool gate_first<Knocks, Handler, Args...>::
lock(Handler&& handler) NOEXCEPT
{
    // bad lock?
    if (locked())
        return false;

    handler_ = std::make_shared<Handler>(std::forward<Handler>(handler));
    count_ = Knocks;
    return true;
}

template <size_t Knocks, typename Handler, typename... Args>
bool gate_first<Knocks, Handler, Args...>::
knock(Args&&... args) NOEXCEPT
{
    // bad knock?
    if (!locked())
        return false;

    // Capture parameter pack as tuple.
    if (first_knock())
        args_ = std::forward_as_tuple(args...);

    // bad knock or invoke?
    return !last_knock() || invoke();
}

// private
template <size_t Knocks, typename Handler, typename... Args>
constexpr bool gate_first<Knocks, Handler, Args...>::
first_knock() const NOEXCEPT
{
    return count_ == Knocks;
}

template <size_t Knocks, typename Handler, typename... Args>
bool gate_first<Knocks, Handler, Args...>::
last_knock() NOEXCEPT
{
    // logic error, too many knocks.
    if (!locked())
        return false;

    count_ = sub1(count_);
    return !locked();
}

template <size_t Knocks, typename Handler, typename... Args>
bool gate_first<Knocks, Handler, Args...>::
invoke() NOEXCEPT
{
    // logic error, knock by not locked.
    if (!handler_)
        return false;

    // Invoke completion handler.
    invoker(*handler_, args_, sequence{});

    // Clear all resources.
    handler_.reset();
    args_ = {};
    return true;
}

// private
template <size_t Knocks, typename Handler, typename... Args>
template<size_t... Index>
void gate_first<Knocks, Handler, Args...>::
invoker(const Handler& handler, const arguments& args,
    std::index_sequence<Index...>) NOEXCEPT
{
    // Expand tuple into parameter pack.
    handler(std::get<Index>(args)...);
}

} // namespace network
} // namespace libbitcoin

#endif
