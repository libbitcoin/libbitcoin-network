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
#ifndef LIBBITCOIN_NETWORK_ASYNC_SUBSCRIBER_IPP
#define LIBBITCOIN_NETWORK_ASYNC_SUBSCRIBER_IPP

////#include <boost/asio.hpp>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {

template <typename... Args>
subscriber<Args...>::subscriber(asio::strand& strand) noexcept
  : strand_(strand), stopped_(false)
{
}

template <typename... Args>
subscriber<Args...>::~subscriber() noexcept
{
    // Cannot clear queue on destruct as required parameterization is unknown.
    BC_ASSERT_MSG(queue_.empty(), "subscriber is not cleared");
}

template <typename... Args>
void subscriber<Args...>::subscribe(handler&& notify) noexcept
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (!stopped_)
        queue_.push_back(std::move(notify));
}

template <typename... Args>
void subscriber<Args...>::notify(const Args&... args) const noexcept
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return;

    // We are already on the strand to protect queue_, so execute each handler.
    for (const auto& handler: queue_)
        handler(args...);
}

template <typename... Args>
void subscriber<Args...>::stop(const Args&... args) noexcept
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return;

    notify(args...);
    stopped_ = true;
    queue_.clear();
}

} // namespace network
} // namespace libbitcoin

#endif
