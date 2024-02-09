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
#include <bitcoin/network/net/deadline.hpp>

#include <functional>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

namespace libbitcoin {
namespace network {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace std::placeholders;

BC_DEBUG_ONLY(static const steady_clock::time_point epoch{};)

deadline::deadline(const logger& log, asio::strand& strand,
    const duration& timeout) NOEXCEPT
  : duration_(timeout),
    timer_(strand),
    tracker<deadline>(log)
{
}

deadline::~deadline() NOEXCEPT
{
    BC_ASSERT_MSG(timer_.expiry() == epoch, "");
}

// Start/stop must not be called concurrently.
void deadline::start(result_handler&& handle) NOEXCEPT
{
    start(std::move(handle), duration_);
}

// Start/stop must not be called concurrently.
void deadline::start(result_handler&& handle, const duration& timeout) NOEXCEPT
{
    timer_.cancel();
    timer_.expires_after(timeout);

    // Handler posted, cancel sets asio::error::operation_aborted.
    timer_.async_wait(
        std::bind(&deadline::handle_timer,
            shared_from_this(), _1, std::move(handle)));
}

void deadline::stop() NOEXCEPT
{
    timer_.cancel();
    BC_DEBUG_ONLY(timer_.expires_at(epoch);)
}

// Callback always (cancel or otherwise) fired with the normalized error code.
void deadline::handle_timer(const error::boost_code& ec,
    const result_handler& handle) NOEXCEPT
{
    BC_DEBUG_ONLY(timer_.expires_at(epoch);)

    // asio::error::operation_aborted maps to error::operation_canceled.
    handle(error::asio_to_error_code(ec));
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
