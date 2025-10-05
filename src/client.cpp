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
#include <bitcoin/network/client.hpp>

#include <bitcoin/network/config/config.hpp>

namespace libbitcoin {
namespace network {

// All values default to constructor defaults (zero/empty).

bool admin::enabled() const NOEXCEPT
{
    return !path.empty()
        && !binds.empty()
        && !host.host().empty()
        && to_bool(connections);
}

bool explore::enabled() const NOEXCEPT
{
    return !path.empty()
        && !binds.empty()
        && !host.host().empty()
        && to_bool(connections);
}

bool rest::enabled() const NOEXCEPT
{
    return !binds.empty()
        && !host.host().empty()
        && to_bool(connections);
}

bool websocket::enabled() const NOEXCEPT
{
    return !binds.empty()
        && !host.host().empty()
        && to_bool(connections);
}

bool bitcoind::enabled() const NOEXCEPT
{
    return !binds.empty()
        && !host.host().empty()
        && to_bool(connections);
}

bool electrum::enabled() const NOEXCEPT
{
    return !binds.empty()
        && to_bool(connections);
}

bool stratum_v1::enabled() const NOEXCEPT
{
    return !binds.empty()
        && to_bool(connections);
}

bool stratum_v2::enabled() const NOEXCEPT
{
    return !binds.empty()
        && to_bool(connections);
}

} // namespace network
} // namespace libbitcoin
