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
#ifndef LIBBITCOIN_NETWORK_ASYNC_RESUBSCRIBER_HPP
#define LIBBITCOIN_NETWORK_ASYNC_RESUBSCRIBER_HPP

#include <functional>
#include <map>
#include <memory>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe, non-virtual.
template <typename Key, typename... Args>
class resubscriber final
{
public:
    DELETE_COPY_MOVE(resubscriber);

    typedef std::function<bool(const code&, Args...)> handler;

    // Strand is only used for assertions.
    resubscriber(asio::strand& strand) NOEXCEPT;
    ~resubscriber() NOEXCEPT;

    /// If stopped this invokes handler(error::subscriber_stopped, Args{}...),
    /// and the handler is dropped. Otherwise the handler is held until stop.
    void subscribe(const Key& key, handler&& handler) NOEXCEPT;

    /// Invokes each handler in order, on the strand, with specified arguments.
    /// Handler returns true to be resubscribed, otherwise it is desubscribed.
    void notify(const code& ec, const Args&... args) NOEXCEPT;

    /// Invokes each handler in order, on the strand, with specified arguments,
    /// and then drops all handlers.
    void stop(const code& ec, const Args&... args) NOEXCEPT;

    /// Invokes each handler in order, on the strand, with specified error code
    /// and default arguments, and then drops all handlers.
    void stop_default(const code& ec) NOEXCEPT;

private:
    // This is thread safe.
    asio::strand& strand_;

    // These are not thread safe.
    bool stopped_;
    std::map<Key, handler> queue_{};
};

} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/async/resubscriber.ipp>

#endif
