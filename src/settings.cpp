/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/settings.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace messages;

// Common default values (no settings context).
settings::settings() NOEXCEPT
    : threads(1),
    protocol_minimum(level::minimum_protocol),
    protocol_maximum(level::maximum_protocol),
    services_minimum(service::minimum_services),
    services_maximum(service::maximum_services),
    invalid_services(176),
    enable_alert(false),
    enable_reject(false),
    enable_relay(false),
    validate_checksum(false),
    identifier(0),
    inbound_port(0),
    inbound_connections(0),
    outbound_connections(8),
    connect_batch_size(5),
    connect_timeout_seconds(5),
    channel_handshake_seconds(30),
    channel_germination_seconds(30),
    channel_heartbeat_minutes(5),
    channel_inactivity_minutes(10),
    channel_expiration_minutes(1440),
    host_pool_capacity(0),
    rate_limit(1024),
    user_agent(BC_USER_AGENT),
    path("hosts.cache"),
    self(unspecified_address_item)
{
}

// Use push_back due to initializer_list bug:
// stackoverflow.com/a/20168627/1172329
settings::settings(chain::selection context) NOEXCEPT
  : settings()
{
    // Handle deviations from common defaults.
    switch (context)
    {
        case chain::selection::mainnet:
        {
            identifier = 3652501241;
            inbound_port = 8333;
            seeds.reserve(4);
            seeds.push_back({ "194.5.152.211", 8333 });
            seeds.push_back({ "96.126.123.143", 8333 });
            seeds.push_back({ "165.227.196.254", 8333 });
            seeds.push_back({ "34.73.164.207", 8333 });
            break;
        }

        case chain::selection::testnet:
        {
            identifier = 118034699;
            inbound_port = 18333;
            seeds.reserve(4);
            seeds.push_back({ "testnet1.libbitcoin.net", 18333 });
            seeds.push_back({ "testnet2.libbitcoin.net", 18333 });
            seeds.push_back({ "testnet3.libbitcoin.net", 18333 });
            seeds.push_back({ "testnet4.libbitcoin.net", 18333 });
            break;
        }

        case chain::selection::regtest:
        {
            identifier = 3669344250;
            inbound_port = 18444;

            // Regtest is private network only, so there is no seeding.
            break;
        }

        default:
        case chain::selection::none:
        {
        }
    }
}

duration settings::connect_timeout() const NOEXCEPT
{
    return seconds(connect_timeout_seconds);
}

duration settings::channel_handshake() const NOEXCEPT
{
    return seconds(channel_handshake_seconds);
}

duration settings::channel_germination() const NOEXCEPT
{
    return seconds(channel_germination_seconds);
}

duration settings::channel_heartbeat() const NOEXCEPT
{
    return minutes(channel_heartbeat_minutes);
}

duration settings::channel_inactivity() const NOEXCEPT
{
    return minutes(channel_inactivity_minutes);
}

duration settings::channel_expiration() const NOEXCEPT
{
    return minutes(channel_expiration_minutes);
}

size_t settings::minimum_address_count() const NOEXCEPT
{
    // Guarded by parameterization (config).
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return system::safe_multiply(connect_batch_size, outbound_connections);
    BC_POP_WARNING()
}

} // namespace network
} // namespace libbitcoin
