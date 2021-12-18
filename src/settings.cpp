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

#include <cstddef>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace messages;

// Common default values (no settings context).
settings::settings()
  : threads(0),
    protocol_maximum(level::maximum_protocol),
    protocol_minimum(level::minimum_protocol),
    services(service::node_none),
    invalid_services(176),
    relay_transactions(false),
    validate_checksum(false),
    inbound_connections(0),
    outbound_connections(8),
    manual_attempt_limit(0),
    connect_batch_size(5),
    connect_timeout_seconds(5),
    channel_handshake_seconds(30),
    channel_germination_seconds(30),
    channel_heartbeat_minutes(5),
    channel_inactivity_minutes(10),
    channel_expiration_minutes(1440),
    host_pool_capacity(0),
    hosts_file("hosts.cache"),
    self(unspecified_address_item),

    // [log]
    debug_file("debug.log"),
    error_file("error.log"),
    archive_directory("archive"),
    rotation_size(0),
    minimum_free_space(0),
    maximum_archive_size(0),
    maximum_archive_files(0),
    statistics_server(unspecified_address_item),
    verbose(false)
{
}

// Use push_back due to initializer_list bug:
// stackoverflow.com/a/20168627/1172329
settings::settings(chain::selection context)
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
            seeds.push_back({ "mainnet1.libbitcoin.net", 8333 });
            seeds.push_back({ "mainnet2.libbitcoin.net", 8333 });
            seeds.push_back({ "mainnet3.libbitcoin.net", 8333 });
            seeds.push_back({ "mainnet4.libbitcoin.net", 8333 });
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

size_t settings::minimum_connections() const
{
    return system::ceilinged_add<size_t>(outbound_connections, peers.size());
}

duration settings::connect_timeout() const
{
    return seconds(connect_timeout_seconds);
}

duration settings::channel_handshake() const
{
    return seconds(channel_handshake_seconds);
}

duration settings::channel_heartbeat() const
{
    return minutes(channel_heartbeat_minutes);
}

duration settings::channel_inactivity() const
{
    return minutes(channel_inactivity_minutes);
}

duration settings::channel_expiration() const
{
    return minutes(channel_expiration_minutes);
}

duration settings::channel_germination() const
{
    return seconds(channel_germination_seconds);
}

} // namespace network
} // namespace libbitcoin
