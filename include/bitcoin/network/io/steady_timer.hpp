/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_NETWORK_IO_STEADY_TIMER_HPP
#define LIBBITCOIN_NETWORK_IO_STEADY_TIMER_HPP

#include <chrono>
#include <functional>
#include <memory>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/io/context.hpp>

namespace libbitcoin {
namespace network {
namespace io {

namespace detail { struct timer_state; }

/// Not thread safe. A timer on the steady clock. A wait completes on the
/// executor with success at expiry, or with operation_canceled if canceled or
/// the expiry is changed first.
class BCT_API steady_timer
{
public:
    DELETE_COPY_MOVE(steady_timer);

    using clock_type = std::chrono::steady_clock;
    using time_point = clock_type::time_point;
    using duration = clock_type::duration;
    using wait_handler = std::function<void(const code&)>;
    using executor_type = executor;

    explicit steady_timer(const executor& executor) NOEXCEPT;

    /// Cancels pending waits.
    ~steady_timer() NOEXCEPT;

    executor_type get_executor() const NOEXCEPT;
    time_point expiry() const NOEXCEPT;

    /// Set the expiry, canceling pending waits (returns the number canceled).
    size_t expires_at(const time_point& expiry) NOEXCEPT;
    size_t expires_after(const duration& span) NOEXCEPT;

    /// Wait for the expiry (a past expiry completes as soon as possible).
    void async_wait(wait_handler&& handler) NOEXCEPT;

    /// Cancel pending waits (returns the number canceled).
    size_t cancel() NOEXCEPT;

private:
    executor executor_;
    std::shared_ptr<detail::timer_state> state_;
    time_point expiry_{};
};

} // namespace io
} // namespace network
} // namespace libbitcoin

#endif
