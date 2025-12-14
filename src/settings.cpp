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
#include <bitcoin/network/settings.hpp>

#include <filesystem>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/messages/http/http.hpp>
#include <bitcoin/network/messages/peer/peer.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace messages::peer;

// tcp_server
// ----------------------------------------------------------------------------

settings::tcp_server::tcp_server(const std::string_view& logging_name) NOEXCEPT
  : name(logging_name)
{
}

bool settings::tcp_server::enabled() const NOEXCEPT
{
    return to_bool(connections);
}

steady_clock::duration settings::tcp_server::inactivity() const NOEXCEPT
{
    return minutes{ inactivity_minutes };
}

steady_clock::duration settings::tcp_server::expiration() const NOEXCEPT
{
    return minutes{ expiration_minutes };
}

// http_server
// ----------------------------------------------------------------------------

system::string_list settings::http_server::host_names() const NOEXCEPT
{
    // secure changes default port from 80 to 443.
    const auto port = secure ? http::default_tls : http::default_http;
    return config::to_host_names(hosts, port);
}

system::string_list settings::http_server::origin_names() const NOEXCEPT
{
    // secure changes default port from 80 to 443.
    const auto port = secure ? http::default_tls : http::default_http;
    return config::to_host_names(hosts, port);
}

// websocket_server
// ----------------------------------------------------------------------------

// [outbound]
// ----------------------------------------------------------------------------

bool settings::peer_outbound::enabled() const NOEXCEPT
{
    return settings::tcp_server::enabled()
        && to_bool(host_pool_capacity)
        && to_bool(connect_batch_size);
}

size_t settings::peer_outbound::minimum_address_count() const NOEXCEPT
{
    // Cannot overflow size_t as long as both are uint16_t.
    return connect_batch_size * peer_outbound::connections;
}

steady_clock::duration settings::peer_outbound::seeding_timeout() const NOEXCEPT
{
    return seconds(seeding_timeout_seconds);
}

bool settings::peer_outbound::disabled(const address_item& item) const NOEXCEPT
{
    return !use_ipv6 && config::is_v6(item.ip);
}

// [inbound]
// ----------------------------------------------------------------------------

config::authority settings::peer_inbound::first_self() const NOEXCEPT
{
    return selfs.empty() ? config::authority{} : selfs.front();
}

bool settings::peer_inbound::advertise() const NOEXCEPT
{
    return enabled() && !selfs.empty();
}

bool settings::peer_inbound::enabled() const NOEXCEPT
{
    return settings::tcp_server::enabled() && !binds.empty();
}

// [manual]
// ----------------------------------------------------------------------------

void settings::peer_manual::initialize() NOEXCEPT
{
    BC_ASSERT_MSG(friends.empty(), "friends not empty");

    // Dynamic conversion of peers is O(N^2), so set on initialize.
    // This converts endpoints to addresses so will produce the default
    // address for any hosts that are DNS names (i.e. not IP addresses).
    friends = system::projection<network::config::authorities>(peers);
}

bool settings::peer_manual::peered(const address_item& item) const NOEXCEPT
{
    // Friends should be mapped from peers by initialize().
    return contains(friends, item);
}

bool settings::peer_manual::enabled() const NOEXCEPT
{
    return settings::tcp_server::enabled() && !peers.empty();
}

// [network]
// ----------------------------------------------------------------------------

static_assert(heading::maximum_payload(level::canonical, true) == 4'000'000);
static_assert(heading::maximum_payload(level::canonical, false) == 1'800'003);

static uint32_t identifier_from_context(chain::selection context) NOEXCEPT
{
    switch (context)
    {
        case chain::selection::mainnet: return 3652501241;
        case chain::selection::testnet: return 118034699;
        case chain::selection::regtest: return 3669344250;
        default: return 0;
    }
}

settings::settings(chain::selection context) NOEXCEPT
  : outbound{ context },
    inbound{ context },
    manual{ context },
    identifier(identifier_from_context(context))
{
}

bool settings::witness_node() const NOEXCEPT
{
    return to_bool(services_minimum & service::node_witness);
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

steady_clock::duration settings::channel_heartbeat() const NOEXCEPT
{
    return minutes(channel_heartbeat_minutes);
}

steady_clock::duration settings::maximum_skew() const NOEXCEPT
{
    return minutes(maximum_skew_minutes);
}

std::filesystem::path settings::file() const NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return path / "hosts.cache";
    BC_POP_WARNING()
}

bool settings::insufficient(const address_item& item) const NOEXCEPT
{
    return (item.services & services_minimum) != services_minimum;
}

bool settings::unsupported(const address_item& item) const NOEXCEPT
{
    return to_bool(item.services & invalid_services);
}

bool settings::blacklisted(const address_item& item) const NOEXCEPT
{
    return contains(blacklists, item);
}

bool settings::whitelisted(const address_item& item) const NOEXCEPT
{
    return whitelists.empty() || contains(whitelists, item);
}

bool settings::excluded(const address_item& item) const NOEXCEPT
{
    return !is_specified(item)
        || outbound.disabled(item)
        || insufficient(item)
        || unsupported(item)
        || manual.peered(item)
        || blacklisted(item)
        || !whitelisted(item);
}

} // namespace network
} // namespace libbitcoin
