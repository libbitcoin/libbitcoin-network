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
template <typename ErrorCode, typename... Args>
class subscriber
  : public std::enable_shared_from_this<subscriber<ErrorCode, Args...>>,
    system::noncopyable
{
public:
    typedef std::function<void(ErrorCode, Args...)> handler;
    typedef std::shared_ptr<subscriber<ErrorCode, Args...>> ptr;

    // Strand is only used for assertions.
    subscriber(asio::strand& strand) noexcept;
    virtual ~subscriber() noexcept;

    /// If stopped this invokes handler(error::subscriber_stopped, Args{}...),
    /// and the handler is dropped. Otherwise the handler is held until stop.
    void subscribe(handler&& handler) noexcept;

    /// Invokes each handler in order, on the strand, with specified arguments.
    void notify(const ErrorCode& ec, const Args&... args) const noexcept;

    /// Invokes each handler in order, on the strand, with specified arguments,
    /// and then drops all handlers.
    void stop(const ErrorCode& ec, const Args&... args) noexcept;

    /// Invokes each handler in order, on the strand, with specified error code
    /// and default arguments, and then drops all handlers.
    void stop_default(const ErrorCode& ec) noexcept;

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
