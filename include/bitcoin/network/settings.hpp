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
#include <bitcoin/network/messages/messages.hpp>

#define BC_HTTP_SERVER_NAME "libbitcoin/4.0"

namespace libbitcoin {
namespace network {

/// The largest p2p payload request when configured for witness blocks.
constexpr uint32_t maximum_request_default
{
    system::possible_narrow_cast<uint32_t>(
        messages::peer::heading::maximum_payload(
            messages::peer::level::canonical, true))
};

/// Common network configuration settings, properties not thread safe.
struct BCT_API settings
{
    struct socks5_client
    {
        DEFAULT_COPY_MOVE_DESTRUCT(socks5_client);
        socks5_client() NOEXCEPT;

        /// Proxy credentials are stored and passed in cleartext.
        std::string username{};
        std::string password{};

        /// Socks5 proxy (default port convention is 1080, but not defaulted).
        config::endpoint socks{};

        /// True if socks::port is non-zero.
        virtual bool proxied() const NOEXCEPT;

        /// False if both username and password are empty.
        virtual bool secured() const NOEXCEPT;
    };

    struct tcp_server
    {
        DEFAULT_COPY_MOVE_DESTRUCT(tcp_server);
        tcp_server(const std::string_view& logging_name) NOEXCEPT;

        /// For logging only.
        std::string name;

        bool secure{ false };
        config::authorities binds{};
        uint16_t connections{ 0 };
        uint32_t inactivity_minutes{ 10 };
        uint32_t expiration_minutes{ 60 };
        uint32_t maximum_request{ maximum_request_default };
        uint32_t minimum_buffer{ maximum_request_default };

        /// Helpers.
        virtual bool enabled() const NOEXCEPT;
        virtual steady_clock::duration inactivity() const NOEXCEPT;
        virtual steady_clock::duration expiration() const NOEXCEPT;
    };

    struct http_server
      : public tcp_server
    {
        using tcp_server::tcp_server;

        /// Sent via responses if configured .
        std::string server{ BC_HTTP_SERVER_NAME };

        /// Validated against hosts/origins if configured.
        config::endpoints hosts{};
        config::endpoints origins{};

        /// Opaque origins are always serialized as "null".
        bool allow_opaque_origin{ false };

        /// Normalized configured hosts/origins helpers.
        virtual system::string_list host_names() const NOEXCEPT;
        virtual system::string_list origin_names() const NOEXCEPT;
    };

    struct websocket_server
      : public http_server
    {
        using http_server::http_server;

        // TODO: settings unique to the websocket aspect.
    };

    struct peer_manual
      : public tcp_server, public socks5_client
    {
        // The friends field must be initialized after peers is set.
        peer_manual(system::chain::selection) NOEXCEPT
          : tcp_server("manual"), socks5_client()
        {
        }

        config::endpoints peers{};
        config::authorities friends{};

        /// Helpers.
        void initialize() NOEXCEPT;
        bool enabled() const NOEXCEPT override;
        virtual bool peered(
            const messages::peer::address_item& item) const NOEXCEPT;
    };

    struct peer_outbound
      : public tcp_server, public socks5_client
    {
        peer_outbound(system::chain::selection context) NOEXCEPT
          : tcp_server("outbound"), socks5_client()
        {
            connections = 10;

            // Use emplace_back due to initializer_list bug:
            // stackoverflow.com/a/20168627/1172329
            switch (context)
            {
                case system::chain::selection::mainnet:
                {
                    seeds.reserve(4);
                    seeds.emplace_back("mainnet1.libbitcoin.net", 8333_u16);
                    seeds.emplace_back("mainnet2.libbitcoin.net", 8333_u16);
                    seeds.emplace_back("mainnet3.libbitcoin.net", 8333_u16);
                    seeds.emplace_back("mainnet4.libbitcoin.net", 8333_u16);
                    break;
                }
                case system::chain::selection::testnet:
                {
                    seeds.reserve(4);
                    seeds.emplace_back("testnet1.libbitcoin.net", 18333_u16);
                    seeds.emplace_back("testnet2.libbitcoin.net", 18333_u16);
                    seeds.emplace_back("testnet3.libbitcoin.net", 18333_u16);
                    seeds.emplace_back("testnet4.libbitcoin.net", 18333_u16);
                    break;
                }
                case system::chain::selection::regtest:
                default: break;
            }
        }

        bool use_ipv6{ false };
        config::endpoints seeds{};
        uint16_t connect_batch_size{ 5 };
        uint32_t host_pool_capacity{ 0 };
        uint32_t seeding_timeout_seconds{ 30 };

        /// Helpers.
        bool enabled() const NOEXCEPT override;
        virtual size_t minimum_address_count() const NOEXCEPT;
        virtual steady_clock::duration seeding_timeout() const NOEXCEPT;
        virtual bool disabled(
            const messages::peer::address_item& item) const NOEXCEPT;
    };
    
    struct peer_inbound
      : public tcp_server
    {
        peer_inbound(system::chain::selection context) NOEXCEPT
          : tcp_server("inbound")
        {
            // Use emplace_back due to initializer_list bug:
            // stackoverflow.com/a/20168627/1172329
            switch (context)
            {
                case system::chain::selection::mainnet:
                {
                    binds.emplace_back(asio::address{}, 8333_u16);
                    break;
                }
                case system::chain::selection::testnet:
                {
                    binds.emplace_back(asio::address{}, 18333_u16);
                    break;
                }
                case system::chain::selection::regtest:
                {
                    binds.emplace_back(asio::address{}, 18444_u16);
                    break;
                }
                default: break;
            }
        }

        bool enable_loopback{ false };
        config::authorities selfs{};

        /// Helpers.
        bool advertise() const NOEXCEPT;
        bool enabled() const NOEXCEPT override;
        config::authority first_self() const NOEXCEPT;
    };

    // [network]
    // ----------------------------------------------------------------------------
    // bitcoin p2p network common settings.

    DEFAULT_COPY_MOVE_DESTRUCT(settings);
    settings(system::chain::selection context) NOEXCEPT;

    /// Bitcoin p2p protocol services.
    network::settings::peer_outbound outbound;
    network::settings::peer_inbound inbound;
    network::settings::peer_manual manual;

    /// Properties.
    uint32_t threads{ 1 };
    uint16_t address_upper{ 10 };
    uint16_t address_lower{ 5 };
    uint32_t protocol_maximum{ messages::peer::level::maximum_protocol };
    uint32_t protocol_minimum{ messages::peer::level::minimum_protocol };
    uint64_t services_maximum{ messages::peer::service::maximum_services };
    uint64_t services_minimum{ messages::peer::service::minimum_services };
    uint64_t invalid_services{ 176 };
    bool enable_address{ false };
    bool enable_address_v2{ false };
    bool enable_witness_tx{ false };
    bool enable_compact{ false };
    bool enable_alert{ false };
    bool enable_reject{ false };
    bool enable_relay{ false };
    bool validate_checksum{ false };
    uint32_t identifier{ 0 };
    uint32_t retry_timeout_seconds{ 1 };
    uint32_t connect_timeout_seconds{ 5 };
    uint32_t handshake_timeout_seconds{ 15 };
    uint32_t channel_heartbeat_minutes{ 5 };
    uint32_t maximum_skew_minutes{ 120 };
    uint32_t rate_limit{ 1024  };
    std::string user_agent{ BC_USER_AGENT };
    std::filesystem::path path{};
    config::authorities blacklists{};
    config::authorities whitelists{};

    /// Helpers.
    virtual bool witness_node() const NOEXCEPT;
    virtual steady_clock::duration retry_timeout() const NOEXCEPT;
    virtual steady_clock::duration connect_timeout() const NOEXCEPT;
    virtual steady_clock::duration channel_handshake() const NOEXCEPT;
    virtual steady_clock::duration channel_heartbeat() const NOEXCEPT;
    virtual steady_clock::duration maximum_skew() const NOEXCEPT;
    virtual std::filesystem::path file() const NOEXCEPT;

    /// Filters.
    virtual bool insufficient(
        const messages::peer::address_item& item) const NOEXCEPT;
    virtual bool unsupported(
        const messages::peer::address_item& item) const NOEXCEPT;
    virtual bool blacklisted(
        const messages::peer::address_item& item) const NOEXCEPT;
    virtual bool whitelisted(
        const messages::peer::address_item& item) const NOEXCEPT;
    virtual bool excluded(
        const messages::peer::address_item& item) const NOEXCEPT;
};

/// Network configuration, thread safe.
class BCT_API configuration
{
public:
    inline configuration(system::chain::selection context) NOEXCEPT
      : network(context)
    {
    }

    /// Settings.
    network::settings network;
};

} // namespace network
} // namespace libbitcoin

#endif
