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
#ifndef LIBBITCOIN_NETWORK_ASYNC_DESUBSCRIBER_HPP
#define LIBBITCOIN_NETWORK_ASYNC_DESUBSCRIBER_HPP

#include <functional>
#include <map>
#include <utility>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe, non-virtual.
/// All methods must be invoked on strand, handlers are invoked on strand.
template <typename Key, typename... Args>
class desubscriber final
{
public:
    DELETE_COPY_MOVE(desubscriber);

    using key = Key;
    typedef std::function<bool(const code&, Args...)> handler;
    typedef std::function<void(const code&, const Key&)> completer;

    // Strand is only used for assertions.
    desubscriber(asio::strand& strand) NOEXCEPT;
    ~desubscriber() NOEXCEPT;

    /// If stopped, handler is invoked with error::subscriber_stopped.
    /// If key exists, handler is invoked with error::subscriber_exists.
    /// Otherwise handler retained. Subscription code is also returned here.
    code subscribe(handler&& handler, const Key& key) NOEXCEPT;

    /// Invoke each handler in order with specified arguments.
    /// Handler return true for resubscription, otherwise it is desubscribed.
    void notify(const code& ec, const Args&... args) NOEXCEPT;

    /// Invoke specified handler in order with specified arguments.
    /// Handler return controls resubscription, and is forwarded to the caller.
    /// True if subscription key found (subscribed).
    bool notify_one(const Key& key, const code& ec,
        const Args&... args) NOEXCEPT;

    /// Invoke each handler in order, with arguments, then drop all.
    void stop(const code& ec, const Args&... args) NOEXCEPT;

    /// Invoke each handler in order, with default arguments, then drop all.
    void stop_default(const code& ec) NOEXCEPT;

    /// The map size.
    size_t size() const NOEXCEPT;

private:
    // This is thread safe.
    asio::strand& strand_;

    // These are not thread safe.
    bool stopped_{ false };
    std::map<Key, handler> map_{};
};

} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/async/desubscriber.ipp>

#endif
