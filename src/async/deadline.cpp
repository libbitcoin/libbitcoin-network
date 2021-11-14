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
#include <bitcoin/network/async/deadline.hpp>

#include <functional>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/async/thread.hpp>
#include <bitcoin/network/async/threadpool.hpp>
#include <bitcoin/network/async/time.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace std::placeholders;

// The timer closure captures an instance of this class and the callback.
// Deadline calls handler exactly once unless canceled/restarted.

deadline::deadline(asio::io_context& service, const duration& timeout)
  : duration_(timeout),
    timer_(service),
    CONSTRUCT_TRACK(deadline)
{
}

deadline::deadline(asio::strand& strand, const duration& timeout)
  : duration_(timeout),
    timer_(strand),
    CONSTRUCT_TRACK(deadline)
{
}

void deadline::start(handler handle)
{
    start(handle, duration_);
}

void deadline::start(handler handle, const duration& timeout)
{
    const auto timer_handler =
        std::bind(&deadline::handle_timer,
            shared_from_this(), _1, handle);

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);

    // Handling socket error codes creates exception safety.
    boost_code ignore;
    timer_.cancel(ignore);
    timer_.expires_from_now(timeout);

    // async_wait will not invoke the handler within this function.
    timer_.async_wait(timer_handler);
    ///////////////////////////////////////////////////////////////////////////
}

// Cancellation calls handle_timer with asio::error::operation_aborted.
// We do not handle the cancelation result code, which will return success
// in the case of a race in which the timer is already expired.
void deadline::stop()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);

    // Handling socket error codes creates exception safety.
    boost_code ignore;
    timer_.cancel(ignore);
    ///////////////////////////////////////////////////////////////////////////
}

// If the timer expires the callback is fired with a success code.
// If the timer fails the callback is fired with the normalized error code.
// If the timer is canceled before it has fired, no call is made (but cleared).
void deadline::handle_timer(const boost_code& ec, handler handle) const
{
    // operation_aborted is the result of cancelation.
    if (ec != boost::asio::error::operation_aborted)
        handle(error::boost_to_error_code(ec));
}

} // namespace network
} // namespace libbitcoin
