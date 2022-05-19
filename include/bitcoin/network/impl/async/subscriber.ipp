/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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

namespace libbitcoin {
namespace network {

template <typename Code, typename... Args>
subscriber<Code, Args...>::subscriber(asio::strand& strand) noexcept
  : strand_(strand), stopped_(false)
{
}

template <typename Code, typename... Args>
subscriber<Code, Args...>::~subscriber() noexcept
{
    // Destruction may not occur on the strand.
    BC_ASSERT_MSG(queue_.empty(), "subscriber is not cleared");
}

template <typename Code, typename... Args>
void subscriber<Code, Args...>::subscribe(handler&& handler) noexcept
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        handler(error::subscriber_stopped, Args{}...);
    else
        queue_.push_back(std::move(handler));
}

template <typename Code, typename... Args>
void subscriber<Code, Args...>::notify(const Code& ec,
    const Args&... args) const noexcept
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return;

    // We are already on the strand to protect queue_, so execute each handler.
    for (const auto& handler: queue_)
        handler(ec, args...);
}

template <typename Code, typename... Args>
void subscriber<Code, Args...>::stop(const Code& ec,
    const Args&... args) noexcept
{
    BC_ASSERT_MSG(ec, "subscriber stopped with success code");
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return;

    notify(ec, args...);
    stopped_ = true;
    queue_.clear();
}

template <typename Code, typename... Args>
void subscriber<Code, Args...>::stop_default(const Code& ec) noexcept
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    stop(ec, Args{}...);
}

} // namespace network
} // namespace libbitcoin

#endif
