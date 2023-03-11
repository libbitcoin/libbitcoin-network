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
#ifndef LIBBITCOIN_NETWORK_ASYNC_DESUBSCRIBER_IPP
#define LIBBITCOIN_NETWORK_ASYNC_DESUBSCRIBER_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

template <typename Key, typename... Args>
desubscriber<Key, Args...>::desubscriber(asio::strand& strand) NOEXCEPT
  : strand_(strand)
{
}

template <typename Key, typename... Args>
desubscriber<Key, Args...>::~desubscriber() NOEXCEPT
{
    // Destruction may not occur on the strand.
    BC_ASSERT_MSG(map_.empty(), "desubscriber is not cleared");
}

template <typename Key, typename... Args>
code desubscriber<Key, Args...>::
subscribe(handler&& handler, const Key& key) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    if (stopped_)
    {
        /*bool*/ handler(error::subscriber_stopped, Args{}...);
        return error::subscriber_stopped;
    }
    else if (map_.contains(key))
    {
        /*bool*/ handler(error::subscriber_exists, Args{}...);
        return error::subscriber_exists;
    }
    else
    {
        map_.emplace(key, std::move(handler));
        return error::success;
    }
    BC_POP_WARNING()
}

template <typename Key, typename... Args>
void desubscriber<Key, Args...>::
notify(const code& ec, const Args&... args) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return;

    // Already on the strand to protect map_, so execute each handler.
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    for (auto it = map_.begin(); it != map_.end();)
    {
        // Invoke handler and handle result.
        if (!it->second(ec, args...))
        {
            it = map_.erase(it);
        }
        else
        {
            ++it;
        }
    }
    BC_POP_WARNING()
}

template <typename Key, typename... Args>
bool desubscriber<Key, Args...>::
notify_one(const Key& key, const code& ec, const Args&... args) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return false;

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const auto it = map_.find(key);
    if (it != map_.end())
    {
        // Invoke handler and handle result.
        if (!it->second(ec, args...))
            map_.erase(it);

        return true;
    }
    BC_POP_WARNING()
    return false;
}

template <typename Key, typename... Args>
void desubscriber<Key, Args...>::
stop(const code& ec, const Args&... args) NOEXCEPT
{
    BC_ASSERT_MSG(ec, "desubscriber stopped with success code");
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (stopped_)
        return;

    notify(ec, args...);
    stopped_ = true;
    map_.clear();
}

template <typename Key, typename... Args>
void desubscriber<Key, Args...>::
stop_default(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    stop(ec, Args{}...);
}

template <typename Key, typename... Args>
size_t desubscriber<Key, Args...>::
size() const NOEXCEPT
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    return map_.size();
}

} // namespace network
} // namespace libbitcoin

#endif
