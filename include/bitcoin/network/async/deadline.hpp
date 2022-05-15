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

/// Not thread safe.
/// Class wrapper for boost::asio::basic_waitable_timer (restartable).
/// This simplifies invocation, eliminates boost-specific error handling and
/// makes timer firing and cancellation conditions safe for shared objects.
class deadline
  : public std::enable_shared_from_this<deadline>, system::noncopyable,
    track<deadline>
{
public:
    typedef std::shared_ptr<deadline> ptr;
    typedef std::function<void(const code&)> handler;
    
    /// Timer notification handler is posted to the service.
    deadline(asio::strand& strand, const duration& timeout=seconds(0));

    /// Assert timer stopped.
    ~deadline();

    /// Start or restart the timer.
    /// Use expired(ec) in handler to test for expiration.
    void start(handler&& handle);

    /// Start or restart the timer.
    /// Use expired(ec) in handler to test for expiration.
    void start(handler&& handle, const duration& timeout);

    /// Cancel the timer. The handler will be invoked.
    void stop();

private:
    void handle_timer(const error::boost_code& ec, const handler& handle);

    // This is thread safe.
    const duration duration_;

    // This is not thread safe.
    asio::wait_timer timer_;
};

} // namespace network
} // namespace libbitcoin

#endif
