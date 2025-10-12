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

#include <ranges>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/messages/rpc/messages.hpp>

namespace libbitcoin {
namespace network {

// utility
static system::string_list to_host_names(const config::endpoints& hosts,
    bool secure) NOEXCEPT
{
    const auto port = secure ? http::default_tls : http::default_http;

    system::string_list out{};
    out.resize(hosts.size());
    std::ranges::transform(hosts, out.begin(), [=](const auto& value) NOEXCEPT
        {
            return value.to_lower(port);
        });

    return out;
}


// tcp_server
// ----------------------------------------------------------------------------

bool tcp_server::enabled() const NOEXCEPT
{
    return !binds.empty() && to_bool(connections);
}

steady_clock::duration tcp_server::timeout() const NOEXCEPT
{
    return seconds{ timeout_seconds };
}

// http_server
// ----------------------------------------------------------------------------

system::string_list http_server::host_names() const NOEXCEPT
{
    // secure changes default port from 80 to 443.
    return to_host_names(hosts, secure);
}

// html_server
// ----------------------------------------------------------------------------

bool html_server::enabled() const NOEXCEPT
{
    return !path.empty() && http_server::enabled();
}

system::string_list html_server::origin_names() const NOEXCEPT
{
    // secure changes default port from 80 to 443.
    return to_host_names(hosts, secure);
}

} // namespace network
} // namespace libbitcoin
