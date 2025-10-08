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

#include <algorithm>
#include <bitcoin/network/config/config.hpp>

namespace libbitcoin {
namespace network {

using namespace system;

// All values default to constructor defaults (zero/empty).

// enabled() helpers
// ----------------------------------------------------------------------------

bool admin::enabled() const NOEXCEPT
{
    // Path is currently required.
    return !path.empty()
        && !binds.empty()
        && to_bool(connections);
}

bool explore::enabled() const NOEXCEPT
{
    // Path is currently required.
    return !path.empty()
        && !binds.empty()
        && to_bool(connections);
}

bool rest::enabled() const NOEXCEPT
{
    return !binds.empty()
        && to_bool(connections);
}

bool websocket::enabled() const NOEXCEPT
{
    return !binds.empty()
        && to_bool(connections);
}

bool bitcoind::enabled() const NOEXCEPT
{
    return !binds.empty()
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

// host_names() helpers
// ----------------------------------------------------------------------------

static string_list to_host_names(const config::endpoints& hosts) NOEXCEPT
{
    string_list out{};
    out.resize(hosts.size());
    std::ranges::transform(hosts, out.begin(), [](const auto& value) NOEXCEPT
    {
        return ascii_to_lower(value.host());
    });

    return out;
}

string_list admin::host_names() const NOEXCEPT
{
    return to_host_names(hosts);
}

string_list explore::host_names() const NOEXCEPT
{
    return to_host_names(hosts);
}

string_list rest::host_names() const NOEXCEPT
{
    return to_host_names(hosts);
}

string_list websocket::host_names() const NOEXCEPT
{
    return to_host_names(hosts);
}

string_list bitcoind::host_names() const NOEXCEPT
{
    return to_host_names(hosts);
}

// timeout() helpers
// ----------------------------------------------------------------------------

steady_clock::duration admin::timeout() const NOEXCEPT
{
    return seconds{ timeout_seconds };
}

steady_clock::duration explore::timeout() const NOEXCEPT
{
    return seconds{ timeout_seconds };
}

steady_clock::duration rest::timeout() const NOEXCEPT
{
    return seconds{ timeout_seconds };
}

steady_clock::duration websocket::timeout() const NOEXCEPT
{
    return seconds{ timeout_seconds };
}

steady_clock::duration bitcoind::timeout() const NOEXCEPT
{
    return seconds{ timeout_seconds };
}

steady_clock::duration electrum::timeout() const NOEXCEPT
{
    return seconds{ timeout_seconds };
}

steady_clock::duration stratum_v1::timeout() const NOEXCEPT
{
    return seconds{ timeout_seconds };
}

steady_clock::duration stratum_v2::timeout() const NOEXCEPT
{
    return seconds{ timeout_seconds };
}

} // namespace network
} // namespace libbitcoin
