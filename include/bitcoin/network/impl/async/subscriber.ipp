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

#include <functional>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/asio.hpp>

namespace libbitcoin {
namespace network {

// Construct.
// ----------------------------------------------------------------------------

template <typename Service, typename... Args>
subscriber<Service, Args...>::subscriber(Service& service)
  : service_(service),
    queue_(std::make_shared<std::vector<handler>>())
{
}

template <typename Service, typename... Args>
subscriber<Service, Args...>::~subscriber()
{
    BC_ASSERT_MSG(!queue_, "subscriber is not stopped");
}

// Stop.
// ----------------------------------------------------------------------------

template <typename Service, typename... Args>
void subscriber<Service, Args...>::stop(const Args&... args)
{
    notify(true, args...);
}

// Methods.
// ----------------------------------------------------------------------------

template <typename Service, typename... Args>
bool subscriber<Service, Args...>::subscribe(handler&& notify)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (queue_)
    {
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        mutex_.unlock_upgrade_and_lock();
        queue_->push_back(std::move(notify));
        mutex_.unlock();
        //---------------------------------------------------------------------
        return true;
    }

    mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////

    return false;
}

template <typename Service, typename... Args>
void subscriber<Service, Args...>::notify(const Args&... args)
{
    notify(false, args...);
}

// private
template <typename Service, typename... Args>
void subscriber<Service, Args...>::notify(bool stop, const Args&... args)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (queue_)
    {
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        mutex_.unlock_upgrade_and_lock();

        // std::bind copies handler and args for each post (including refs).
        // Post all to service, non-blocking, cannot internally execute handler.
        for (const auto& handler: *queue_)
            boost::asio::post(service_, std::bind(handler, args...));

        if (stop)
            queue_.reset();

        mutex_.unlock();
        //---------------------------------------------------------------------
        return;
    }

    mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////
}

} // namespace network
} // namespace libbitcoin

#endif
