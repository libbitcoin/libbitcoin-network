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
#include <bitcoin/network/async/enable_shared_from_base.hpp>
#include <bitcoin/network/async/time.hpp>
#include <bitcoin/network/async/thread.hpp>
#include <bitcoin/network/async/threadpool.hpp>
////#include <bitcoin/network/async/track.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// Class wrapper for boost::asio::deadline_timer, thread safe.
/// This simplifies invocation, eliminates boost-specific error handling and
/// makes timer firing and cancellation conditions safer.
class BCT_API deadline
  : public enable_shared_from_base<deadline>,
    system::noncopyable
    /*, track<deadline>*/
{
public:
    typedef std::shared_ptr<deadline> ptr;
    typedef std::function<void(const system::code&)> handler;

    /// Construct a deadline timer with a zero duration.
    deadline(threadpool& pool);

    /// Construct a deadline timer with duration relative to start call.
    deadline(threadpool& pool, const duration& span);

    /// Start or restart the timer.
    /// The handler will not be invoked within the scope of this call.
    /// Use expired(ec) in handler to test for expiration.
    /// A std::shared_ptr to the deadline instance must make this call.
    void start(handler handle);

    /// Start or restart the timer.
    /// The handler will not be invoked within the scope of this call.
    /// Use expired(ec) in handler to test for expiration.
    /// A std::shared_ptr to the deadline instance must make this call.
    void start(handler handle, const duration& span);

    /// Cancel the timer. The handler will be invoked.
    void stop();

private:
    void handle_timer(const system::boost_code& ec, handler handle) const;

    wait_timer timer_;
    duration duration_;
    mutable shared_mutex mutex_;
};

} // namespace network
} // namespace libbitcoin

#endif
