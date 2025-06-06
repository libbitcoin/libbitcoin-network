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
#ifndef LIBBITCOIN_NETWORK_ASYNC_SUBSCRIBER_IPP
#define LIBBITCOIN_NETWORK_ASYNC_SUBSCRIBER_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

template <typename... Args>
subscriber<Args...>::
subscriber(asio::strand& strand) NOEXCEPT
  : strand_(strand)
{
}

template <typename... Args>
subscriber<Args...>::
~subscriber() NOEXCEPT
{
    // Destruction may not occur on the strand.
    BC_ASSERT_MSG(queue_.empty(), "subscriber is not cleared");
}

template <typename... Args>
code subscriber<Args...>::
subscribe(handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    if (stopped_)
    {
        handler(error::subscriber_stopped, Args{}...);
        return error::subscriber_stopped;
    }
    else
    {
        queue_.push_back(std::move(handler));
        return error::success;
    }
    BC_POP_WARNING()
}

template <typename... Args>
void subscriber<Args...>::
notify(const code& ec, const Args&... args) const NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return;

    // Already on the strand to protect queue_, so execute each handler.
    for (const auto& handler: queue_)
    {
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        handler(ec, args...);
        BC_POP_WARNING()
    }
}

template <typename... Args>
void subscriber<Args...>::
stop(const code& ec, const Args&... args) NOEXCEPT
{
    BC_ASSERT_MSG(ec, "subscriber stopped with success code");
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return;

    notify(ec, args...);
    stopped_ = true;
    queue_.clear();
}

template <typename... Args>
void subscriber<Args...>::
stop_default(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    stop(ec, Args{}...);
}

template <typename... Args>
size_t subscriber<Args...>::
size() const NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    return queue_.size();
}

template <typename... Args>
bool subscriber<Args...>::
empty() const NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    return queue_.empty();
}

} // namespace network
} // namespace libbitcoin

#endif
