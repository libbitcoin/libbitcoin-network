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
#ifndef LIBBITCOIN_NETWORK_ASYNC_RESUBSCRIBER_IPP
#define LIBBITCOIN_NETWORK_ASYNC_RESUBSCRIBER_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {

template <typename Key, typename... Args>
resubscriber<Key, Args...>::resubscriber(asio::strand& strand) NOEXCEPT
  : strand_(strand), stopped_(false)
{
}

template <typename Key, typename... Args>
resubscriber<Key, Args...>::~resubscriber() NOEXCEPT
{
    // Destruction may not occur on the strand.
    BC_ASSERT_MSG(map_.empty(), "resubscriber is not cleared");
}

template <typename Key, typename... Args>
void resubscriber<Key, Args...>::subscribe(handler&& handler,
    const Key& key) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
    {
        /*bool*/ handler(error::subscriber_stopped, Args{}...);
    }
    else if (map_.contains(key))
    {
        /*bool*/ handler(error::subscriber_exists, Args{}...);
    }
    else
    {
        map_.emplace(key, std::move(handler));
    }
}

template <typename Key, typename... Args>
void resubscriber<Key, Args...>::notify(const code& ec,
    const Args&... args) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return;

    // Already on the strand to protect map_, so execute each handler.
    for (auto it = map_.begin(); it != map_.end();)
    {
        // Invoke handler and capture return value for directed erase.
        if (!it->second(ec, args...))
        {
            // desubscribed
            it = map_.erase(it);
        }
        else
        {
            // resubscribed
            ++it;
        }
    }
}

template <typename Key, typename... Args>
bool resubscriber<Key, Args...>::notify(const Key& key, const code& ec,
    const Args&... args) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return false;

    const auto it = map_.find(key);
    if (it != map_.end())
    {
        // Invoke handler and capture return value for directed erase.
        if (!it->second(ec, args...))
        {
            // desubscribed
            map_.erase(it);
            return false;
        }
    }

    // resubscribed
    return true;
}

template <typename Key, typename... Args>
void resubscriber<Key, Args...>::stop(const code& ec,
    const Args&... args) NOEXCEPT
{
    BC_ASSERT_MSG(ec, "resubscriber stopped with success code");
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return;

    notify(ec, args...);
    stopped_ = true;
    map_.clear();
}

template <typename Key, typename... Args>
void resubscriber<Key, Args...>::stop_default(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    stop(ec, Args{}...);
}

} // namespace network
} // namespace libbitcoin

#endif
