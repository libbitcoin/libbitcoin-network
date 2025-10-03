/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/net/channel.hpp>

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

// Protocols invoke channel stop for application layer protocol violations.
// Channels invoke channel stop for channel timouts and communcation failures.
channel::channel(const logger& log, const socket::ptr& socket,
    const network::settings& settings, uint64_t identifier) NOEXCEPT
  : proxy(socket),
    settings_(settings),
    identifier_(identifier),
    tracker<channel>(log)
{
}

channel::~channel() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "channel is not stopped");
    if (!stopped()) { LOGF("~channel is not stopped."); }
}

// Properties.
// ----------------------------------------------------------------------------

uint64_t channel::nonce() const NOEXCEPT
{
    return nonce_;
}

uint64_t channel::identifier() const NOEXCEPT
{
    return identifier_;
}

network::settings channel::settings() const NOEXCEPT
{
    return settings_;
}

} // namespace network
} // namespace libbitcoin
