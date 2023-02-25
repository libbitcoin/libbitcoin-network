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

#include <filesystem>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace messages;

static_assert(heading::maximum_payload(level::canonical, true) == 4'000'000);
static_assert(heading::maximum_payload(level::canonical, false) == 1'800'003);

// TODO: make witness support configurable.
// Common default values (no settings context).
settings::settings() NOEXCEPT
  : threads(1),
    address_maximum(10),
    address_minimum(5),
    protocol_maximum(level::maximum_protocol),
    protocol_minimum(level::minimum_protocol),
    services_maximum(service::maximum_services),
    services_minimum(service::minimum_services),
    invalid_services(176),
    enable_address(false),
    enable_alert(false),
    enable_reject(false),
    enable_transaction(false),
    enable_ipv6(false),
    enable_loopback(false),
    validate_checksum(false),
    identifier(0),
    inbound_port(0),
    inbound_connections(0),
    outbound_connections(8),
    connect_batch_size(5),
    retry_timeout_seconds(1),
    connect_timeout_seconds(5),
    handshake_timeout_seconds(30),
    seeding_timeout_seconds(30),
    channel_heartbeat_minutes(5),
    channel_inactivity_minutes(10),
    channel_expiration_minutes(1440),
    host_pool_capacity(0),
    rate_limit(1024),
    minimum_buffer(4'000'000),
    user_agent(BC_USER_AGENT)
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

void settings::initialize() NOEXCEPT
{
    BC_ASSERT_MSG(friends.empty(), "friends not empty");

    // Dynamic conversion of peers is O(N^2), so set on initialize.
    friends = system::projection<config::authorities>(peers);
}

bool settings::inbound_enabled() const NOEXCEPT
{
    return to_bool(inbound_connections)
        && to_bool(inbound_port);
}

bool settings::outbound_enabled() const NOEXCEPT
{
    return to_bool(outbound_connections)
        && to_bool(host_pool_capacity)
        && to_bool(connect_batch_size);
}

bool settings::advertise_enabled() const NOEXCEPT
{
    return inbound_enabled() && self;
}

size_t settings::maximum_payload() const NOEXCEPT
{
    return heading::maximum_payload(protocol_maximum,
        to_bool(services_maximum & service::node_witness));
}

// Randomized from 50% to maximum milliseconds (specified in seconds).
steady_clock::duration settings::retry_timeout() const NOEXCEPT
{
    const auto from = retry_timeout_seconds * 500_u64;
    const auto to = retry_timeout_seconds * 1'000_u64;
    return milliseconds{ system::pseudo_random::next(from, to) };
}

// Randomized from 50% to maximum milliseconds (specified in seconds).
steady_clock::duration settings::connect_timeout() const NOEXCEPT
{
    const auto from = connect_timeout_seconds * 500_u64;
    const auto to = connect_timeout_seconds * 1'000_u64;
    return milliseconds{ system::pseudo_random::next(from, to) };
}

steady_clock::duration settings::channel_handshake() const NOEXCEPT
{
    return seconds(handshake_timeout_seconds);
}

steady_clock::duration settings::channel_germination() const NOEXCEPT
{
    return seconds(seeding_timeout_seconds);
}

steady_clock::duration settings::channel_heartbeat() const NOEXCEPT
{
    return minutes(channel_heartbeat_minutes);
}

steady_clock::duration settings::channel_inactivity() const NOEXCEPT
{
    return minutes(channel_inactivity_minutes);
}

steady_clock::duration settings::channel_expiration() const NOEXCEPT
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

std::filesystem::path settings::file() const NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return path / "hosts.cache";
    BC_POP_WARNING()
}

bool settings::disabled(const messages::address_item& item) const NOEXCEPT
{
    return !enable_ipv6 && config::is_v6(item.ip);
}

bool settings::insufficient(const messages::address_item& item) const NOEXCEPT
{
    return (item.services & services_minimum) != services_minimum;
}

bool settings::unsupported(const messages::address_item& item) const NOEXCEPT
{
    return to_bool(item.services & invalid_services);
}

bool settings::blacklisted(const messages::address_item& item) const NOEXCEPT
{
    return contains(blacklists, item);
}

bool settings::whitelisted(const messages::address_item& item) const NOEXCEPT
{
    return whitelists.empty() || contains(whitelists, item);
}

bool settings::peered(const messages::address_item& item) const NOEXCEPT
{
    // Friends should be mapped from peers by initialize().
    return contains(friends, item);
}

bool settings::excluded(const messages::address_item& item) const NOEXCEPT
{
    return !messages::is_specified(item)
        || disabled(item)
        || insufficient(item)
        || unsupported(item)
        || peered(item)
        || blacklisted(item)
        || !whitelisted(item);
}

} // namespace network
} // namespace libbitcoin
