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
#ifndef LIBBITCOIN_NETWORK_ASYNC_SUBSCRIBER_HPP
#define LIBBITCOIN_NETWORK_ASYNC_SUBSCRIBER_HPP

#include <functional>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe, non-virtual.
/// All methods must be invoked on strand, handlers are invoked on strand.
template <typename... Args>
class subscriber final
{
public:
    DELETE_COPY_MOVE(subscriber);

    typedef std::function<void(const code&, Args...)> handler;

    // Strand is only used for assertions.
    subscriber(asio::strand& strand) NOEXCEPT;
    ~subscriber() NOEXCEPT;

    /// If stopped, handler is invoked with error::subscriber_stopped.
    /// Otherwise handler retained. Subscription code is also returned here.
    code subscribe(handler&& handler) NOEXCEPT;

    /// Invoke each handler in order with specified arguments.
    void notify(const code& ec, const Args&... args) const NOEXCEPT;

    /// Invoke each handler in order, with arguments, then drop all.
    void stop(const code& ec, const Args&... args) NOEXCEPT;

    /// Invoke each handler in order, with default arguments, then drop all.
    void stop_default(const code& ec) NOEXCEPT;

    /// The queue size.
    size_t size() const NOEXCEPT;

private:
    // This is thread safe.
    asio::strand& strand_;

    // These are not thread safe.
    bool stopped_{ false };
    std_vector<handler> queue_{};
};

} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/async/subscriber.ipp>

#endif
