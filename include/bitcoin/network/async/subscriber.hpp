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
#ifndef LIBBITCOIN_NETWORK_ASYNC_SUBSCRIBER_HPP
#define LIBBITCOIN_NETWORK_ASYNC_SUBSCRIBER_HPP

#include <functional>
#include <memory>
#include <vector>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/async/enable_shared_from_base.hpp>
#include <bitcoin/network/async/thread.hpp>

namespace libbitcoin {
namespace network {

/// Thread safe event queue.
/// Stop is thread safe and idempotent, may be called multiple times.
/// All handlers are posted to the service.
template <typename Service, typename... Args>
class subscriber
  : public enable_shared_from_base<subscriber<Service, Args...>>,
    system::noncopyable
{
public:
    typedef std::function<void(Args...)> handler;
    typedef std::shared_ptr<subscriber<Service, Args...>> ptr;

    // Construct.
    // ------------------------------------------------------------------------

    /// Event notification handlers are posted to the service.
    subscriber(Service& service);
    virtual ~subscriber();

    // Stop.
    // ------------------------------------------------------------------------

    /// Notification order follows subscription order. Arguments are copied.
    /// Clears all handlers and prevents subsequent subscription (idempotent).
    void stop(const Args&... args);

    // Methods.
    // ------------------------------------------------------------------------

    /// Notification order follows subscription order. Arguments are copied.
    void notify(const Args&... args);

    /// Subscribe to notifications, false if subscriber is stopped.
    /// Subscription handlers are retained in the queue until stop.
    bool subscribe(handler&& notify);

private:
    // Clears all handlers and prevents subsequent subscription if stop true.
    void notify(bool stop, const Args&... args);

    // This is thread safe (asio:io_context or asio::strand).
    Service& service_;

    // This is protected by mutex.
    std::shared_ptr<std::vector<handler>> queue_;
    mutable upgrade_mutex mutex_;
};

} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/async/subscriber.ipp>

#endif
