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
#ifndef LIBBITCOIN_NETWORK_NET_DEADLINE_HPP
#define LIBBITCOIN_NETWORK_NET_DEADLINE_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe, non-virtual.
/// Class wrapper for boost::asio::basic_waitable_timer (restartable).
/// This simplifies invocation, eliminates boost-specific error handling and
/// makes timer firing and cancellation conditions safe for shared objects.
class BCT_API deadline final
  : public std::enable_shared_from_this<deadline>, protected tracker<deadline>
{
public:
    DELETE_COPY_MOVE(deadline);

    typedef steady_clock::duration duration;
    typedef std::shared_ptr<deadline> ptr;
    
    /// Timer notification handler is posted to the service.
    deadline(const logger& log, asio::strand& strand,
        const duration& timeout=seconds(0)) NOEXCEPT;

    /// Assert timer stopped.
    ~deadline() NOEXCEPT;

    /// Start or restart the timer.
    /// Sets error::success on expiration, error::operation_canceled on stop.
    void start(result_handler&& handle) NOEXCEPT;

    /// Start or restart the timer.
    /// Sets error::success on expiration, error::operation_canceled on stop.
    void start(result_handler&& handle, const duration& timeout) NOEXCEPT;

    /// Cancel the timer, ok if stopped. The handler will be invoked.
    void stop() NOEXCEPT;

private:
    void handle_timer(const error::boost_code& ec,
        const result_handler& handle) NOEXCEPT;

    // This is thread safe.
    const duration duration_;

    // This is not thread safe.
    asio::steady_timer timer_;
};

} // namespace network
} // namespace libbitcoin

#endif
