/**
 * Copyright (c) 2011-2016 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/network/settings.hpp>

#include <algorithm>
#include <limits>
#include <thread>
#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::asio;
using namespace bc::message;

// Common default values (no settings context).
settings::settings()
  : threads(std::max(std::thread::hardware_concurrency(), 1u)),
    protocol_maximum(version::level::maximum),
    protocol_minimum(version::level::minimum),
    services(version::service::none),
    inbound_connections(0),
    outbound_connections(8),
    manual_attempt_limit(0),
    connect_batch_size(5),
    connect_timeout_seconds(5),
    channel_handshake_seconds(30),
    channel_heartbeat_minutes(5),
    channel_inactivity_minutes(10),
    channel_expiration_minutes(1440),
    channel_germination_seconds(30),
    host_pool_capacity(0),
    relay_transactions(false),
    hosts_file("hosts.cache"),
    self(unspecified_network_address),

    // [log]
    debug_file("debug.log"),
    error_file("error.log"),
    archive_directory("archive"),
    rotation_size(0),
    maximum_archive_size(max_uint32),
    minimum_free_space(0),
    maximum_archive_files(max_uint32),
    statsd_server(unspecified_network_address)
{
}

// Use push_back due to initializer_list bug:
// stackoverflow.com/a/20168627/1172329
settings::settings(config::settings context)
  : settings()
{
    // Handle deviations from common defaults.
    switch (context)
    {
        case config::settings::mainnet:
        {
            identifier = 3652501241;
            inbound_port = 8333;

            // Seeds based on bitcoinstats.com/network/dns-servers
            seeds.reserve(6);
            seeds.push_back({ "seed.bitnodes.io", 8333 });
            seeds.push_back({ "seed.bitcoinstats.com", 8333 });
            seeds.push_back({ "seed.bitcoin.sipa.be", 8333 });
            seeds.push_back({ "dnsseed.bluematt.me", 8333 });
            seeds.push_back({ "seed.bitcoin.jonasschnelli.ch", 8333 });
            seeds.push_back({ "dnsseed.bitcoin.dashjr.org", 8333 });
            break;
        }

        case config::settings::testnet:
        {
            identifier = 118034699;
            inbound_port = 18333;

            seeds.reserve(3);
            seeds.push_back({ "testnet-seed.bitcoin.petertodd.org", 18333 });
            seeds.push_back({ "testnet-seed.bitcoin.schildbach.de", 18333 });
            seeds.push_back({ "testnet-seed.bluematt.me", 18333 });
            break;
        }

        default:
        case config::settings::none:
        {
        }
    }
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
