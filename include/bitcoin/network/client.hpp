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
#ifndef LIBBITCOIN_NETWORK_CLIENT_HPP
#define LIBBITCOIN_NETWORK_CLIENT_HPP

#include <filesystem>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// ---------------------------------------------------------------------------

/// tcp/ip server settings (hash bindings/security/connections/timeout).
/// This is designed for RPC servers that don't require http communication. 
struct BCT_API tcp_server
{
    /// Not implemented (TLS).
    bool secure{ false };
    config::endpoints binds{};
    uint16_t connections{ 0 };

    /// Not fully implemented, keep-alive (recommended).
    uint32_t timeout_seconds{ 60 };

    /// !binds.empty() && !is_zero(connections)
    bool enabled() const NOEXCEPT;
    steady_clock::duration timeout() const NOEXCEPT;
};

/// http/s server settings (hash server/host names).
/// This is designed for web servers that don't require origin handling.
/// This includes websockets (handshake) and bitcoind json-rpc.
struct BCT_API http_server
  : public tcp_server
{
    /// Sent via responses if configured (recommended).
    std::string server{ "libbitcoin/4.0" };

    /// Validated against requests if configured (recommended).
    config::endpoints hosts{};

    /// Normalized hosts.
    system::string_list host_names() const NOEXCEPT;
};

/// html (http/s) document server settings (has directory/default).
/// This is for web servers that expose a local file system directory.
struct BCT_API html_server
  : public http_server
{
    /// Directory to serve.
    std::filesystem::path path{};

    /// Default page for default URL (recommended).
    std::string default_{ "index.html" };

    /// Validated against origins if configured (recommended).
    config::endpoints origins{};

    /// Normalized origins.
    system::string_list origin_names() const NOEXCEPT;

    /// !path.empty() && http_server::enabled() [hidden, not virtual]
    bool enabled() const NOEXCEPT;
};

/// ---------------------------------------------------------------------------

/// native admin interface, isolated (http/s, stateless html)
using admin = html_server;

/// native RESTful block explorer (http/s, stateless html/json)
using explore = html_server;

/// native websocket query interface (http/s->tcp/s, json, upgrade handshake)
using websocket = http_server;

/// bitcoind compatibility interface (http/s, stateless json-rpc-v2)
using bitcoind = http_server;

/// electrum compatibility interface (tcp/s, json-rpc-v2)
using electrum = tcp_server;

/// stratum v1 compatibility interface (tcp/s, json-rpc-v1, auth handshake)
using stratum_v1 = tcp_server;

/// stratum v2 compatibility interface (tcp[/s], binary, auth/privacy handshake)
using stratum_v2 = tcp_server;

} // namespace network
} // namespace libbitcoin

#endif
