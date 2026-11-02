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
#include <bitcoin/network/io/steady_timer.hpp>

#include <bitcoin/network/define.hpp>
#include "scheduler.hpp"

namespace libbitcoin {
namespace network {
namespace io {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

steady_timer::steady_timer(const executor& executor) NOEXCEPT
  : executor_(executor), state_(std::make_shared<detail::timer_state>())
{
}

steady_timer::~steady_timer() NOEXCEPT
{
    cancel();
}

steady_timer::executor_type steady_timer::get_executor() const NOEXCEPT
{
    return executor_;
}

steady_timer::time_point steady_timer::expiry() const NOEXCEPT
{
    return expiry_;
}

size_t steady_timer::expires_at(const time_point& expiry) NOEXCEPT
{
    expiry_ = expiry;
    return executor_.context().scheduler().expire_timer(state_, expiry);
}

size_t steady_timer::expires_after(const duration& span) NOEXCEPT
{
    return expires_at(clock_type::now() + span);
}

void steady_timer::async_wait(wait_handler&& handler) NOEXCEPT
{
    executor_.context().scheduler().wait_timer(state_,
        { executor_, std::move(handler) });
}

size_t steady_timer::cancel() NOEXCEPT
{
    return executor_.context().scheduler().cancel_timer(state_);
}

BC_POP_WARNING()

} // namespace io
} // namespace network
} // namespace libbitcoin
