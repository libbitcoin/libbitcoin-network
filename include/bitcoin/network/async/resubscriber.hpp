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

#include <utility>
#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe, non-virtual.
/// All methods must be invoked on strand, handlers are invoked on strand.
template <typename Key, typename... Args>
class resubscriber final
{
public:
    DELETE_COPY_MOVE(resubscriber);

    using key = Key;
    typedef std::function<bool(const code&, Args...)> handler;
    typedef std::function<void(const code&, const Key&)> completer;

    // Strand is only used for assertions.
    resubscriber(asio::strand& strand) NOEXCEPT;
    ~resubscriber() NOEXCEPT;

    /// If key exists, handler is invoked with error::subscriber_exists.
    /// If stopped, handler is invoked with error::subscriber_stopped/defaults,
    /// handler dropped. Otherwise it is held until stop/drop. False if failed.
    bool subscribe(handler&& handler, const Key& key) NOEXCEPT;

    /// Invoke each handler in order with specified arguments.
    /// Handler return true for resubscription, otherwise it is desubscribed.
    void notify(const code& ec, const Args&... args) NOEXCEPT;

    /// Invoke specified handler in order with specified arguments.
    /// Handler return controls resubscription, and is forwarded to the caller.
    /// False if subscription key not not found (not subscribed).
    bool notify_one(const Key& key, const code& ec,
        const Args&... args) NOEXCEPT;

    /// Invoke each handler in order, with arguments, then drop all.
    void stop(const code& ec, const Args&... args) NOEXCEPT;

    /// Invoke each handler in order, with default arguments, then drop all.
    void stop_default(const code& ec) NOEXCEPT;

private:
    // This is thread safe.
    asio::strand& strand_;

    // These are not thread safe.
    bool stopped_{ false };
    std::map<Key, handler> map_{};
};

} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/async/resubscriber.ipp>

#endif
