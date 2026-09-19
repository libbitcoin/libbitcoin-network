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
#include <bitcoin/network/broadcast/terminator.hpp>

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

terminator::terminator(const race::ptr& complete, const code& reason,
    uint64_t channel, const config::address& address) NOEXCEPT
  : race_(complete),
    reason_(reason),
    channel_(channel),
    address_(address)
{
}

bool terminator::targets(uint64_t identifier,
    const config::address& address) const NOEXCEPT
{
    return is_zero(channel_) ? address_ == address : channel_ == identifier;
}

const code& terminator::reason() const NOEXCEPT
{
    return reason_;
}

void terminator::stopped() const NOEXCEPT
{
    race_->finish(error::success);
}

} // namespace network
} // namespace libbitcoin
