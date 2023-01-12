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
#ifndef LIBBITCOIN_NETWORK_ASYNC_DEADLINE_HPP
#define LIBBITCOIN_NETWORK_ASYNC_DEADLINE_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/async/time.hpp>
#include <bitcoin/network/async/thread.hpp>
#include <bitcoin/network/async/track.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe, non-virtual.
/// Class wrapper for boost::asio::basic_waitable_timer (restartable).
/// This simplifies invocation, eliminates boost-specific error handling and
/// makes timer firing and cancellation conditions safe for shared objects.
class BCT_API deadline final
  : public std::enable_shared_from_this<deadline>, track<deadline>
{
public:
    DELETE_COPY_MOVE(deadline);

    typedef std::shared_ptr<deadline> ptr;
    typedef std::function<void(const code&)> handler;
    
    /// Timer notification handler is posted to the service.
    deadline(asio::strand& strand, const duration& timeout=seconds(0)) NOEXCEPT;

    /// Assert timer stopped.
    ~deadline() NOEXCEPT;

    /// Start or restart the timer.
    /// Use expired(ec) in handler to test for expiration.
    void start(handler&& handle) NOEXCEPT;

    /// Start or restart the timer.
    /// Use expired(ec) in handler to test for expiration.
    void start(handler&& handle, const duration& timeout) NOEXCEPT;

    /// Cancel the timer. The handler will be invoked.
    void stop() NOEXCEPT;

private:
    void handle_timer(const error::boost_code& ec,
        const handler& handle) NOEXCEPT;

    // This is thread safe (const).
    const duration duration_;

    // This is not thread safe.
    asio::wait_timer timer_;
};

} // namespace network
} // namespace libbitcoin

#endif
