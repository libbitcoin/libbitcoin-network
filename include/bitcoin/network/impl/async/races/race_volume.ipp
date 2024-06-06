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
#ifndef LIBBITCOIN_NETWORK_ASYNC_RACES_RACE_VOLUME_IPP
#define LIBBITCOIN_NETWORK_ASYNC_RACES_RACE_VOLUME_IPP

#include <memory>
#include <tuple>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

template <error::error_t Success, error::error_t Fail>
race_volume<Success, Fail>::
race_volume(size_t size, size_t required) NOEXCEPT
  : size_(size), required_(required)
{
}

template <error::error_t Success, error::error_t Fail>
race_volume<Success, Fail>::
~race_volume() NOEXCEPT
{
    BC_ASSERT_MSG(!running() && !complete_, "deleting running race_volume");
}

template <error::error_t Success, error::error_t Fail>
inline bool race_volume<Success, Fail>::
running() const NOEXCEPT
{
    return to_bool(runners_);
}

template <error::error_t Success, error::error_t Fail>
bool race_volume<Success, Fail>::
start(handler&& sufficient, handler&& complete) NOEXCEPT
{
    // false implies logic error.
    if (running())
        return false;

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    sufficient_ = std::make_shared<handler>(std::forward<handler>(sufficient));
    complete_ = std::make_shared<handler>(std::forward<handler>(complete));
    BC_POP_WARNING()

    runners_ = size_;
    return true;
}

template <error::error_t Success, error::error_t Fail>
bool race_volume<Success, Fail>::
finish(size_t count) NOEXCEPT
{
    // false implies logic error.
    if (!running())
        return false;

    bool winner{ false };

    // Determine sufficiency if not yet reached.
    if (sufficient_)
    {
        // Invoke sufficient and clear resources before race is finished.
        if (count >= required_)
        {
            winner = true;
            (*sufficient_)(Success);
            sufficient_.reset();
        }
        else if (runners_ == one)
        {
            (*sufficient_)(Fail);
            sufficient_.reset();
        }
    }

    // false invoke implies logic error.
    return invoke() && winner;
}

// private
// ----------------------------------------------------------------------------

template <error::error_t Success, error::error_t Fail>
bool race_volume<Success, Fail>::
invoke() NOEXCEPT
{
    // false implies logic error.
    if (!running())
        return false;

    --runners_;
    if (running())
        return true;

    // false implies logic error.
    if (!complete_)
        return false;

    // Invoke completion handler, always success.
    (*complete_)(Success);

    // Clear resources.
    complete_.reset();
    return true;
}

} // namespace network
} // namespace libbitcoin

#endif
