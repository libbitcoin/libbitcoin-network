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
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/async/thread.hpp>
#include <bitcoin/network/async/threadpool.hpp>
#include <bitcoin/network/async/time.hpp>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {

using namespace std::placeholders;

deadline::deadline(asio::strand& strand, const duration& timeout)
  : duration_(timeout),
    strand_(strand),
    timer_(strand),
    track<deadline>()
{
}

// Start cannot be called concurrently with stop, strand restarts.
void deadline::start(handler&& handle)
{
    start(std::move(handle), duration_);
}

// Start cannot be called concurrently with stop, strand restarts.
void deadline::start(handler&& handle, const duration& timeout)
{
    // Handling cancel error code creates exception safety.
    error::boost_code ignore;
    timer_.cancel(ignore);
    timer_.expires_from_now(timeout);

    // Handler posted, invoked once, sets operation_aborted if canceled.
    timer_.async_wait(
        std::bind(&deadline::handle_timer,
            shared_from_this(), _1, std::move(handle)));
}

// Cancellation calls handle_timer with asio::error::operation_aborted.
void deadline::stop()
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // Handling cancel error code creates exception safety.
    error::boost_code ignore;
    timer_.cancel(ignore);
}

// If the timer expires the callback is fired with a success code.
// If the timer fails the callback is fired with the normalized error code.
// If the timer is canceled before it has fired, no call is made (but cleared).
void deadline::handle_timer(const error::boost_code& ec,
    const handler& handle) const
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (!error::asio_is_cancelled(ec))
        handle(error::asio_to_error_code(ec));
}

} // namespace network
} // namespace libbitcoin
