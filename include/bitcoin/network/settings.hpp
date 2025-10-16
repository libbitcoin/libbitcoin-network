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
#ifndef LIBBITCOIN_NETWORK_SETTINGS_HPP
#define LIBBITCOIN_NETWORK_SETTINGS_HPP

#include <filesystem>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/peer/messages.hpp>

namespace libbitcoin {
namespace network {

/// Common network configuration settings, properties not thread safe.
struct BCT_API settings
{
    /// tcp/ip server settings (hash bindings/security/connections/timeout).
    /// This is designed for RPC servers that don't require http communication. 
    struct tcp_server
    {
        /// For logging only.
        std::string name{};

        /// Not implemented (TLS).
        bool secure{ false };
        config::authorities binds{};
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
    struct http_server
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
    struct html_server
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

    DEFAULT_COPY_MOVE_DESTRUCT(settings);

    settings() NOEXCEPT;
    settings(system::chain::selection context) NOEXCEPT;

    /// Properties.
    uint32_t threads;
    uint16_t address_upper;
    uint16_t address_lower;
    uint32_t protocol_maximum;
    uint32_t protocol_minimum;
    uint64_t services_maximum;
    uint64_t services_minimum;
    uint64_t invalid_services;
    bool enable_address;
    bool enable_address_v2;
    bool enable_witness_tx;
    bool enable_compact;
    bool enable_alert;
    bool enable_reject;
    bool enable_relay;
    bool enable_ipv6;
    bool enable_loopback;
    bool validate_checksum;
    uint32_t identifier;
    uint16_t inbound_connections;
    uint16_t outbound_connections;
    uint16_t connect_batch_size;
    uint32_t retry_timeout_seconds;
    uint32_t connect_timeout_seconds;
    uint32_t handshake_timeout_seconds;
    uint32_t seeding_timeout_seconds;
    uint32_t channel_heartbeat_minutes;
    uint32_t channel_inactivity_minutes;
    uint32_t channel_expiration_minutes;
    uint32_t maximum_skew_minutes;
    uint32_t host_pool_capacity;
    uint32_t minimum_buffer;
    uint32_t rate_limit;
    std::string user_agent;
    std::filesystem::path path{};
    config::endpoints peers{};
    config::endpoints seeds{};
    config::authorities selfs{};
    config::authorities binds{};
    config::authorities blacklists{};
    config::authorities whitelists{};
    config::authorities friends{};

    /// TODO: move these to node or server.
    /// Client-server settings.
    /// -----------------------------------------------------------------------
    /// native admin web interface, isolated (http/s, stateless html)
    html_server web{ "web" };

    /// native RESTful block explorer (http/s, stateless html/json)
    html_server explore{ "explore" };

    /// native websocket query interface (http/s->tcp/s, json, handshake)
    http_server websocket{ "websocket" };

    /// bitcoind compat interface (http/s, stateless json-rpc-v2)
    http_server bitcoind{ "bitcoind" };

    /// electrum compat interface (tcp/s, json-rpc-v2)
    tcp_server electrum{ "electrum" };

    /// stratum v1 compat interface (tcp/s, json-rpc-v1, auth handshake)
    tcp_server stratum_v1{ "stratum_v1" };

    /// stratum v2 compat interface (tcp[/s], binary, auth/privacy handshake)
    tcp_server stratum_v2{ "stratum_v2" };
    /// -----------------------------------------------------------------------

    /// Set friends.
    virtual void initialize() NOEXCEPT;

    /// Helpers.
    virtual bool witness_node() const NOEXCEPT;
    virtual bool inbound_enabled() const NOEXCEPT;
    virtual bool outbound_enabled() const NOEXCEPT;
    virtual bool advertise_enabled() const NOEXCEPT;
    virtual size_t maximum_payload() const NOEXCEPT;
    virtual config::authority first_self() const NOEXCEPT;
    virtual steady_clock::duration retry_timeout() const NOEXCEPT;
    virtual steady_clock::duration connect_timeout() const NOEXCEPT;
    virtual steady_clock::duration channel_handshake() const NOEXCEPT;
    virtual steady_clock::duration channel_germination() const NOEXCEPT;
    virtual steady_clock::duration channel_heartbeat() const NOEXCEPT;
    virtual steady_clock::duration channel_inactivity() const NOEXCEPT;
    virtual steady_clock::duration channel_expiration() const NOEXCEPT;
    virtual steady_clock::duration maximum_skew() const NOEXCEPT;
    virtual size_t minimum_address_count() const NOEXCEPT;
    virtual std::filesystem::path file() const NOEXCEPT;

    /// Filters.
    virtual bool disabled(const messages::peer::address_item& item) const NOEXCEPT;
    virtual bool insufficient(const messages::peer::address_item& item) const NOEXCEPT;
    virtual bool unsupported(const messages::peer::address_item& item) const NOEXCEPT;
    virtual bool blacklisted(const messages::peer::address_item& item) const NOEXCEPT;
    virtual bool whitelisted(const messages::peer::address_item& item) const NOEXCEPT;
    virtual bool peered(const messages::peer::address_item& item) const NOEXCEPT;
    virtual bool excluded(const messages::peer::address_item& item) const NOEXCEPT;
};

} // namespace network
} // namespace libbitcoin

#endif
