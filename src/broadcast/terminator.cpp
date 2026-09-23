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
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

using namespace system;

// The address is unspecified for a named target, matching no address.
terminator::terminator(const race::ptr& complete, const code& reason,
    uint64_t channel, const config::endpoint& endpoint) NOEXCEPT
  : race_(complete),
    reason_(reason),
    channel_(channel),
    slots_(max_size_t),
    endpoint_(endpoint),
    address_(endpoint)
{
}

terminator::terminator(const code& reason, size_t slots) NOEXCEPT
  : race_(),
    reason_(reason),
    channel_(zero),
    slots_(slots),
    endpoint_(),
    address_()
{
}

bool terminator::targets(uint64_t identifier, const config::address& address,
    const config::endpoint& endpoint) const NOEXCEPT
{
    if (slots_ != max_size_t)
        return false;

    if (!is_zero(channel_))
        return channel_ == identifier;

    if (endpoint_ == endpoint)
        return true;

    // A named endpoint has no address, which matches no channel address.
    const messages::peer::address_item& item = address_;
    return !messages::peer::is_unspecified(item.address) &&
        address_ == address;
}

size_t terminator::slots() const NOEXCEPT
{
    return slots_;
}

const code& terminator::reason() const NOEXCEPT
{
    return reason_;
}

void terminator::stopped() const NOEXCEPT
{
    if (race_)
        race_->finish(error::success);
}

} // namespace network
} // namespace libbitcoin
