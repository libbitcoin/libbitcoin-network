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

namespace libbitcoin {
namespace network {

/// Not thread safe.
template <typename... Args>
class subscriber
  : public std::enable_shared_from_this<subscriber<Args...>>,
    system::noncopyable
{
public:
    typedef std::function<void(Args...)> handler;
    typedef std::shared_ptr<subscriber<Args...>> ptr;

    // Strand is only used for assertions.
    subscriber(asio::strand& strand) noexcept;
    virtual ~subscriber() noexcept;

    void subscribe(handler&& notify) noexcept;
    void notify(const Args&... args) const noexcept;
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
