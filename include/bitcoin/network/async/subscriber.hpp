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
#include <bitcoin/network/async/thread.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe.
/// All handlers are posted to the strand.
template <typename... Args>
class subscriber
  : public std::enable_shared_from_this<subscriber<Args...>>,
    system::noncopyable
{
public:
    typedef std::function<void(Args...)> handler;
    typedef std::shared_ptr<subscriber<Args...>> ptr;

    /// Event notification handlers are posted to the given strand.
    subscriber(asio::strand& strand) noexcept;
    virtual ~subscriber() noexcept;
    
    /// Subscription handlers are retained in the queue until stop.
    /// No invocation occurs if the subscriber is stopped at time of subscribe.
    void subscribe(handler&& notify) noexcept;

    /// Notification order follows subscription order. Arguments are copied.
    void notify(const Args&... args) const noexcept;

    /// Notification order follows subscription order. Arguments are copied.
    /// Clears all handlers and prevents subsequent subscription (idempotent).
    void stop(const Args&... args) noexcept;

private:
    // This is thread safe.
    asio::strand& strand_;

    // These are not thread safe.
    bool stopped_;
    std::vector<handler> queue_;
};

} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/async/subscriber.ipp>

#endif
