/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#include "test.hpp"

BC_AUTO_TEST_SUITE(settings_tests)

using namespace bc::system::chain;
using namespace messages::peer;

// [network]
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(settings__construct__default__expected)
{
    settings instance{ system::chain::selection::mainnet };

    // [network]
    BC_REQUIRE(instance.network == selection::mainnet);
    BC_REQUIRE_EQUAL(instance.threads, 0u);
    BC_REQUIRE_EQUAL(instance.threads_(), std::min<size_t>(cores(), 32));
    BC_REQUIRE_EQUAL(instance.address_upper, 10u);
    BC_REQUIRE_EQUAL(instance.address_lower, 5u);
    BC_REQUIRE_EQUAL(instance.protocol_maximum, level::maximum_protocol);
    BC_REQUIRE_EQUAL(instance.protocol_minimum, level::headers_protocol);
    BC_REQUIRE_EQUAL(instance.invalid_services, 268'435'632u);
    BC_REQUIRE_EQUAL(instance.enable_address, true);
    BC_REQUIRE_EQUAL(instance.enable_address_v2, true);
    BC_REQUIRE_EQUAL(instance.enable_witness_tx, false);
    BC_REQUIRE_EQUAL(instance.enable_compact, true);
    BC_REQUIRE_EQUAL(instance.enable_alert, false);
    BC_REQUIRE_EQUAL(instance.enable_reject, false);
    BC_REQUIRE_EQUAL(instance.enable_not_found, true);
    BC_REQUIRE_EQUAL(instance.enable_memory_pool, false);
    BC_REQUIRE_EQUAL(instance.enable_relay, true);
    BC_REQUIRE_EQUAL(instance.validate_checksum, false);
    BC_REQUIRE_EQUAL(instance.identifier, 3652501241u);
    BC_REQUIRE_EQUAL(instance.retry_timeout_seconds, 1u);
    BC_REQUIRE_EQUAL(instance.connect_timeout_seconds, 5u);
    BC_REQUIRE_EQUAL(instance.outbound.connect_timeout_seconds, 0u);
    BC_REQUIRE_EQUAL(instance.handshake_timeout_seconds, 15u);
    BC_REQUIRE_EQUAL(instance.channel_heartbeat_minutes, 10u);
    BC_REQUIRE_EQUAL(instance.maximum_skew_minutes, 120u);
    BC_REQUIRE_EQUAL(instance.rate_limit, 0u);
    BC_REQUIRE_EQUAL(instance.user_agent, BC_USER_AGENT);
    BC_REQUIRE(instance.path.empty());
    BC_REQUIRE(instance.blacklists.empty());
    BC_REQUIRE(instance.whitelists.empty());
}

BC_AUTO_TEST_CASE(settings__construct__mainnet__expected)
{
    settings instance{ selection::mainnet };
    BC_REQUIRE(instance.network == selection::mainnet);
    BC_REQUIRE_EQUAL(instance.identifier, 3652501241u);
    BC_REQUIRE_EQUAL(instance.outbound.seeds.size(), 8u);
    BC_REQUIRE_EQUAL(instance.outbound.seeds.front().port(), 8333u);
    BC_REQUIRE_EQUAL(instance.outbound.connections, 100u);
    BC_REQUIRE_EQUAL(instance.outbound.connect_batch_size, 5u);
    BC_REQUIRE_EQUAL(instance.outbound.host_pool_capacity, 10000u);
    BC_REQUIRE_EQUAL(instance.inbound.connections, 100u);
    BC_REQUIRE_EQUAL(instance.inbound.binds.size(), 1u);
    BC_REQUIRE_EQUAL(instance.inbound.binds.front().port(), 8333u);
}

BC_AUTO_TEST_CASE(settings__construct__testnet3__expected)
{
    settings instance{ selection::testnet3 };
    BC_REQUIRE(instance.network == selection::testnet3);
    BC_REQUIRE_EQUAL(instance.identifier, 118034699u);
    BC_REQUIRE_EQUAL(instance.outbound.seeds.size(), 3u);
    BC_REQUIRE_EQUAL(instance.outbound.seeds.front().port(), 18333u);
    BC_REQUIRE_EQUAL(instance.outbound.connections, 50u);
    BC_REQUIRE_EQUAL(instance.outbound.connect_batch_size, 10u);
    BC_REQUIRE_EQUAL(instance.outbound.host_pool_capacity, 10000u);
    BC_REQUIRE_EQUAL(instance.inbound.connections, 100u);
    BC_REQUIRE_EQUAL(instance.inbound.binds.size(), 1u);
    BC_REQUIRE_EQUAL(instance.inbound.binds.front().port(), 18333u);
}

BC_AUTO_TEST_CASE(settings__construct__testnet4__expected)
{
    settings instance{ selection::testnet4 };
    BC_REQUIRE(instance.network == selection::testnet4);
    BC_REQUIRE_EQUAL(instance.identifier, 675223068u);
    BC_REQUIRE_EQUAL(instance.outbound.seeds.size(), 2u);
    BC_REQUIRE_EQUAL(instance.outbound.seeds.front().port(), 48333u);
    BC_REQUIRE_EQUAL(instance.outbound.connections, 50u);
    BC_REQUIRE_EQUAL(instance.outbound.connect_batch_size, 10u);
    BC_REQUIRE_EQUAL(instance.outbound.host_pool_capacity, 10000u);
    BC_REQUIRE_EQUAL(instance.inbound.connections, 100u);
    BC_REQUIRE_EQUAL(instance.inbound.binds.size(), 1u);
    BC_REQUIRE_EQUAL(instance.inbound.binds.front().port(), 48333u);
}

BC_AUTO_TEST_CASE(settings__construct__regtest__expected)
{
    settings instance{ selection::regtest };
    BC_REQUIRE(instance.network == selection::regtest);
    BC_REQUIRE_EQUAL(instance.identifier, 3669344250u);
    BC_REQUIRE_EQUAL(instance.outbound.seeds.size(), 0u);
    BC_REQUIRE_EQUAL(instance.outbound.connections, 0u);
    BC_REQUIRE_EQUAL(instance.outbound.connect_batch_size, 5u);
    BC_REQUIRE_EQUAL(instance.outbound.host_pool_capacity, 0u);
    BC_REQUIRE_EQUAL(instance.inbound.connections, 100u);
    BC_REQUIRE_EQUAL(instance.inbound.binds.size(), 1u);
    BC_REQUIRE_EQUAL(instance.inbound.binds.front().port(), 18444u);
}

BC_AUTO_TEST_CASE(settings__construct__signet__expected)
{
    settings instance{ selection::signet };
    BC_REQUIRE(instance.network == selection::signet);
    BC_REQUIRE_EQUAL(instance.identifier, 1087308554u);
    BC_REQUIRE_EQUAL(instance.outbound.seeds.size(), 2u);
    BC_REQUIRE_EQUAL(instance.outbound.seeds.front().port(), 38333u);
    BC_REQUIRE_EQUAL(instance.outbound.connections, 100u);
    BC_REQUIRE_EQUAL(instance.outbound.connect_batch_size, 5u);
    BC_REQUIRE_EQUAL(instance.outbound.host_pool_capacity, 10000u);
    BC_REQUIRE_EQUAL(instance.inbound.connections, 100u);
    BC_REQUIRE_EQUAL(instance.inbound.binds.size(), 1u);
    BC_REQUIRE_EQUAL(instance.inbound.binds.front().port(), 38333u);
}

// helpers

BC_AUTO_TEST_CASE(settings__retry_timeout__always__between_zero_and_retry_timeout_seconds)
{
    settings instance{ system::chain::selection::mainnet };
    instance.retry_timeout_seconds = 42;
    BC_REQUIRE(instance.retry_timeout() > seconds{ zero });
    BC_REQUIRE(instance.retry_timeout() <= seconds{ instance.retry_timeout_seconds });
}

BC_AUTO_TEST_CASE(settings__connect_timeout__unset_option__between_zero_and_network_seconds)
{
    settings instance{ system::chain::selection::mainnet };
    instance.connect_timeout_seconds = 42;
    BC_REQUIRE(instance.connect_timeout(instance.outbound) > seconds{ zero });
    BC_REQUIRE(instance.connect_timeout(instance.outbound) <= seconds{ instance.connect_timeout_seconds });
}

BC_AUTO_TEST_CASE(settings__connect_timeout__set_option__between_zero_and_option_seconds)
{
    settings instance{ system::chain::selection::mainnet };
    instance.outbound.connect_timeout_seconds = 42;
    BC_REQUIRE(instance.connect_timeout(instance.outbound) > seconds{ zero });
    BC_REQUIRE(instance.connect_timeout(instance.outbound) <= seconds{ instance.outbound.connect_timeout_seconds });
}

BC_AUTO_TEST_CASE(settings__channel_handshake__always__handshake_timeout_seconds)
{
    settings instance{ system::chain::selection::mainnet };
    constexpr auto expected = 42u;
    instance.handshake_timeout_seconds = expected;
    BC_REQUIRE(instance.channel_handshake() == seconds(expected));
}

BC_AUTO_TEST_CASE(settings__channel_heartbeat__always__channel_heartbeat_minutes)
{
    settings instance{ system::chain::selection::mainnet };
    constexpr auto expected = 42u;
    instance.channel_heartbeat_minutes = expected;
    BC_REQUIRE(instance.channel_heartbeat() == minutes(expected));
}

BC_AUTO_TEST_CASE(settings__maximum_skew__always__maximum_skew_minutes)
{
    settings instance{ system::chain::selection::mainnet };
    constexpr auto expected = 42u;
    instance.maximum_skew_minutes = expected;
    BC_REQUIRE(instance.maximum_skew() == minutes(expected));
}

// filters

BC_AUTO_TEST_CASE(settings__unsupported__default__false)
{
    settings instance{ system::chain::selection::mainnet };
    constexpr uint64_t services = 0;
    constexpr messages::peer::address_item loop{ 42, services, ipv6_t{ loopback_ip_address }, 8333 };
    instance.invalid_services = 0;
    BC_REQUIRE(!instance.unsupported(loop));
    instance.invalid_services = 1;
    BC_REQUIRE(!instance.unsupported(loop));
}

BC_AUTO_TEST_CASE(settings__unsupported__match__expected)
{
    settings instance{ system::chain::selection::mainnet };
    constexpr uint64_t services = 0b01010101;
    constexpr messages::peer::address_item loop{ 42, services, ipv6_t{ loopback_ip_address }, 8333 };
    instance.invalid_services = services;
    BC_REQUIRE(instance.unsupported(loop));
    instance.invalid_services = services | 0b00000010;
    BC_REQUIRE(instance.unsupported(loop));
    instance.invalid_services = services & 0b11111110;
    BC_REQUIRE(instance.unsupported(loop));
    instance.invalid_services = 0b10101010;
    BC_REQUIRE(!instance.unsupported(loop));
    instance.invalid_services = 0;
    BC_REQUIRE(!instance.unsupported(loop));
}

BC_AUTO_TEST_CASE(settings__blacklisted__ipv4_subnet__expected)
{
    settings instance{ system::chain::selection::mainnet };
    instance.blacklists.clear();
    BC_REQUIRE(!instance.blacklisted(config::address{ "42.42.42.42" }));

    instance.blacklists.emplace_back("12.12.12.12");
    instance.blacklists.emplace_back("24.24.24.24");
    BC_REQUIRE(!instance.blacklisted(config::address{ "42.42.42.42" }));

    instance.blacklists.emplace_back("42.42.42.0/24");
    BC_REQUIRE(instance.blacklisted(config::address{ "42.42.42.42" }));
}

BC_AUTO_TEST_CASE(settings__blacklisted__ipv4_host__expected)
{
    settings instance{ system::chain::selection::mainnet };
    instance.blacklists.clear();
    BC_REQUIRE(!instance.blacklisted(config::address{ "24.24.24.24" }));

    instance.blacklists.emplace_back("12.12.12.12");
    instance.blacklists.emplace_back("42.42.42.0/24");
    BC_REQUIRE(!instance.blacklisted(config::address{ "24.24.24.24" }));

    instance.blacklists.emplace_back("24.24.24.24");
    BC_REQUIRE(instance.blacklisted(config::address{ "24.24.24.24" }));
}

BC_AUTO_TEST_CASE(settings__blacklisted__ipv6_subnet__expected)
{
    settings instance{ system::chain::selection::mainnet };
    instance.blacklists.clear();
    BC_REQUIRE(!instance.blacklisted(config::address{ "[2020:db8::3]" }));

    instance.blacklists.emplace_back("[2020:db8::1]");
    instance.blacklists.emplace_back("[2020:db8::2]");
    BC_REQUIRE(!instance.blacklisted(config::address{ "[2020:db8::3]" }));

    instance.blacklists.emplace_back("[2020:db8::2]/64");
    BC_REQUIRE(instance.blacklisted(config::address{ "[2020:db8::3]" }));
}

BC_AUTO_TEST_CASE(settings__blacklisted__ipv6_host__expected)
{
    settings instance{ system::chain::selection::mainnet };
    instance.blacklists.clear();
    BC_REQUIRE(!instance.blacklisted(config::address{ "[2020:db8::3]" }));

    instance.blacklists.emplace_back("[2020:db8::1]");
    instance.blacklists.emplace_back("[2020:db8::2]");
    BC_REQUIRE(!instance.blacklisted(config::address{ "[2020:db8::3]" }));

    instance.blacklists.emplace_back("[2020:db8::3]");
    BC_REQUIRE(instance.blacklisted(config::address{ "[2020:db8::3]" }));
}

BC_AUTO_TEST_CASE(settings__whitelisted__ipv4_subnet__expected)
{
    settings instance{ system::chain::selection::mainnet };
    instance.whitelists.clear();
    BC_REQUIRE(instance.whitelisted(config::address{ "42.42.42.42" }));

    instance.whitelists.emplace_back("12.12.12.12");
    instance.whitelists.emplace_back("24.24.24.24");
    BC_REQUIRE(!instance.whitelisted(config::address{ "42.42.42.42" }));

    instance.whitelists.emplace_back("42.42.42.0/24");
    BC_REQUIRE(instance.whitelisted(config::address{ "42.42.42.42" }));
}

BC_AUTO_TEST_CASE(settings__whitelisted__ipv4_host__expected)
{
    settings instance{ system::chain::selection::mainnet };
    instance.whitelists.clear();
    BC_REQUIRE(instance.whitelisted(config::address{ "24.24.24.24" }));

    instance.whitelists.emplace_back("12.12.12.12");
    instance.whitelists.emplace_back("42.42.42.0/24");
    BC_REQUIRE(!instance.whitelisted(config::address{ "24.24.24.24" }));

    instance.whitelists.emplace_back("24.24.24.24");
    BC_REQUIRE(instance.whitelisted(config::address{ "24.24.24.24" }));
}

BC_AUTO_TEST_CASE(settings__whitelisted__ipv6_subnet__expected)
{
    settings instance{ system::chain::selection::mainnet };
    instance.whitelists.clear();
    BC_REQUIRE(instance.whitelisted(config::address{ "[2020:db8::3]" }));

    instance.whitelists.emplace_back("[2020:db8::1]");
    instance.whitelists.emplace_back("[2020:db8::2]");
    BC_REQUIRE(!instance.whitelisted(config::address{ "[2020:db8::3]" }));

    instance.whitelists.emplace_back("[2020:db8::2]/64");
    BC_REQUIRE(instance.whitelisted(config::address{ "[2020:db8::3]" }));
}

BC_AUTO_TEST_CASE(settings__whitelisted__ipv6_host__expected)
{
    settings instance{ system::chain::selection::mainnet };
    instance.whitelists.clear();
    BC_REQUIRE(instance.whitelisted(config::address{ "[2020:db8::3]" }));
    instance.whitelists.emplace_back("[2020:db8::1]");
    instance.whitelists.emplace_back("[2020:db8::2]");
    BC_REQUIRE(!instance.whitelisted(config::address{ "[2020:db8::3]" }));
    instance.whitelists.emplace_back("[2020:db8::3]");
    BC_REQUIRE(instance.whitelisted(config::address{ "[2020:db8::3]" }));
}

BC_AUTO_TEST_CASE(settings__excluded__default__true)
{
    settings instance{ system::chain::selection::mainnet };
    BC_REQUIRE(instance.excluded({}));
}

// services
// ----------------------------------------------------------------------------
constexpr auto maximum_request = system::chain::max_block_weight;
constexpr auto minimum_buffer = 4 * kilobyte;

BC_AUTO_TEST_CASE(settings__socks5__defaults__expected)
{
    const settings::socks5 instance{};

    BC_REQUIRE(!instance.authenticated());
    BC_REQUIRE(instance.username.empty());
    BC_REQUIRE(instance.password.empty());
    BC_REQUIRE(!instance.proxied());
    BC_REQUIRE(instance.socks == config::endpoint{});
}

BC_AUTO_TEST_CASE(settings__tcp_server__defaults__expected)
{
    constexpr auto name = "test";
    const settings::tcp_server instance{ name };

    // tcp_server
    BC_REQUIRE_EQUAL(instance.name, name);
    BC_REQUIRE(instance.binds.empty());
    BC_REQUIRE_EQUAL(instance.bind_port, 0u);
    BC_REQUIRE_EQUAL(instance.connections, 0u);
    BC_REQUIRE_EQUAL(instance.inactivity_minutes, 10u);
    BC_REQUIRE_EQUAL(instance.expiration_minutes, 60u);
    BC_REQUIRE_EQUAL(instance.maximum_request, maximum_request);
    BC_REQUIRE_EQUAL(instance.minimum_buffer, minimum_buffer);
    BC_REQUIRE_EQUAL(instance.maximum_buffer, 64 * kilobyte);
    BC_REQUIRE_EQUAL(instance.maximum_backlog, megabyte);
    BC_REQUIRE_EQUAL(instance.connect_timeout_seconds, 0u);
    BC_REQUIRE_EQUAL(instance.rate_limit, 0u);
    BC_REQUIRE(!instance.enabled());
    BC_REQUIRE(instance.inactivity() == minutes(10));
    BC_REQUIRE(instance.expiration() == minutes(60));
}

BC_AUTO_TEST_CASE(settings__tcp_server_binding__no_binds__unspecified)
{
    const settings::tcp_server instance{ "test" };
    BC_REQUIRE(instance.binding(0).address().is_unspecified());
    BC_REQUIRE(instance.binding(42).address().is_unspecified());
    BC_REQUIRE_EQUAL(instance.binding(42).port(), 0u);
}

BC_AUTO_TEST_CASE(settings__tcp_server_binding__two_binds__group_modulo_expected)
{
    settings::tcp_server instance{ "test" };
    instance.binds.emplace_back(system::ip::ipv4::loopback(), 0_u16);
    instance.binds.emplace_back(system::ip::ipv6::loopback(), 42_u16);
    BC_REQUIRE(instance.binding(0).address() == system::ip::ipv4::loopback());
    BC_REQUIRE_EQUAL(instance.binding(0).port(), 0u);
    BC_REQUIRE(instance.binding(1).address() == system::ip::ipv6::loopback());
    BC_REQUIRE_EQUAL(instance.binding(1).port(), 42u);
    BC_REQUIRE(instance.binding(2).address() == system::ip::ipv4::loopback());
    BC_REQUIRE(instance.binding(3).address() == system::ip::ipv6::loopback());
}

BC_AUTO_TEST_CASE(settings__tcp_server_default_ports__zero_ports__bind_port)
{
    settings::tcp_server instance{ "test" };
    instance.bind_port = 42;
    instance.binds.emplace_back(system::ip::ipv4::loopback(), 0_u16);
    instance.binds.emplace_back(system::ip::ipv6::loopback(), 0_u16);
    instance.default_ports();

    BC_REQUIRE(instance.binds.at(0).ip() == system::ip::ipv4::loopback());
    BC_REQUIRE_EQUAL(instance.binds.at(0).port(), 42u);
    BC_REQUIRE(instance.binds.at(1).ip() == system::ip::ipv6::loopback());
    BC_REQUIRE_EQUAL(instance.binds.at(1).port(), 42u);
}

BC_AUTO_TEST_CASE(settings__tcp_server_default_ports__nonzero_port__unchanged)
{
    settings::tcp_server instance{ "test" };
    instance.bind_port = 42;
    instance.binds.emplace_back(system::ip::ipv4::loopback(), 24_u16);
    instance.default_ports();

    BC_REQUIRE_EQUAL(instance.binds.at(0).port(), 24u);
}

BC_AUTO_TEST_CASE(settings__secure_server_default_ports__zero_ports__bind_and_safe_ports)
{
    settings::secure_server instance{ "test" };
    instance.bind_port = 42;
    instance.safe_port = 43;
    instance.binds.emplace_back(system::ip::ipv4::loopback(), 0_u16);
    instance.safes.emplace_back(system::ip::ipv4::loopback(), 0_u16);
    instance.default_ports();

    BC_REQUIRE_EQUAL(instance.binds.at(0).port(), 42u);
    BC_REQUIRE_EQUAL(instance.safes.at(0).port(), 43u);
}

BC_AUTO_TEST_CASE(settings__tls_server__defaults__expected)
{
    constexpr auto name = "test";
    const settings::tls_server instance{ name };

    // tcp_server
    BC_REQUIRE_EQUAL(instance.name, name);
    BC_REQUIRE(instance.binds.empty());
    BC_REQUIRE_EQUAL(instance.connections, 0u);
    BC_REQUIRE_EQUAL(instance.inactivity_minutes, 10u);
    BC_REQUIRE_EQUAL(instance.expiration_minutes, 60u);
    BC_REQUIRE_EQUAL(instance.maximum_request, maximum_request);
    BC_REQUIRE_EQUAL(instance.minimum_buffer, minimum_buffer);
    BC_REQUIRE(!instance.enabled());
    BC_REQUIRE(instance.inactivity() == minutes(10));
    BC_REQUIRE(instance.expiration() == minutes(60));

    // tls_server
    BC_REQUIRE(!instance.secure());
    BC_REQUIRE(instance.safes.empty());
    BC_REQUIRE(instance.cert_auth.empty());
    BC_REQUIRE(instance.cert_path.empty());
    BC_REQUIRE(instance.key_path.empty());
    BC_REQUIRE(instance.key_pass.empty());
}

BC_AUTO_TEST_CASE(settings__http_server__defaults__expected)
{
    constexpr auto name = "test";
    const settings::http_server instance{ name };

    // tcp_server
    BC_REQUIRE_EQUAL(instance.name, name);
    BC_REQUIRE(instance.binds.empty());
    BC_REQUIRE_EQUAL(instance.connections, 0u);
    BC_REQUIRE_EQUAL(instance.inactivity_minutes, 10u);
    BC_REQUIRE_EQUAL(instance.expiration_minutes, 60u);
    BC_REQUIRE_EQUAL(instance.maximum_request, maximum_request);
    BC_REQUIRE_EQUAL(instance.minimum_buffer, minimum_buffer);
    BC_REQUIRE(!instance.enabled());
    BC_REQUIRE(instance.inactivity() == minutes(10));
    BC_REQUIRE(instance.expiration() == minutes(60));

    // tls_server
    BC_REQUIRE(!instance.secure());
    BC_REQUIRE(instance.safes.empty());
    BC_REQUIRE(instance.cert_auth.empty());
    BC_REQUIRE(instance.cert_path.empty());
    BC_REQUIRE(instance.key_path.empty());
    BC_REQUIRE(instance.key_pass.empty());

    // http_server
    BC_REQUIRE_EQUAL(instance.server, BC_HTTP_SERVER_NAME);
    BC_REQUIRE(instance.hosts.empty());
    BC_REQUIRE(instance.origins.empty());
    BC_REQUIRE(instance.host_names().empty());
    BC_REQUIRE(instance.origin_names().empty());
    BC_REQUIRE(!instance.allow_opaque_origin);
    BC_REQUIRE(!instance.authorize());
    BC_REQUIRE(instance.credentials.empty());

    constexpr auto digest = system::base16_array("d9fa291efa0c924851f3ea14e73b1847506ab451fb834b7101e5a1a88f94f500");
    BC_REQUIRE_EQUAL(config::credential{ ":" }.digest(), digest);
}

BC_AUTO_TEST_CASE(settings__http_server_permitted__unmethoded_credential__all_methods)
{
    settings::http_server instance{ "test" };
    instance.credentials.emplace_back("username:password");

    const auto digest = config::credential{ "username:password" }.digest();
    BC_REQUIRE(instance.authorize());
    BC_REQUIRE(instance.authorized(digest));
    BC_REQUIRE(instance.permitted(digest, "getblockcount"));
}

BC_AUTO_TEST_CASE(settings__http_server_permitted__methoded_credential__listed_methods)
{
    settings::http_server instance{ "test" };
    instance.credentials.emplace_back("username:password:getblockcount,getbestblockhash");

    const auto digest = config::credential{ "username:password" }.digest();
    BC_REQUIRE(instance.authorized(digest));
    BC_REQUIRE(instance.permitted(digest, "getblockcount"));
    BC_REQUIRE(instance.permitted(digest, "getbestblockhash"));
    BC_REQUIRE(!instance.permitted(digest, "getblock"));
}

BC_AUTO_TEST_CASE(settings__http_server_authorized__unconfigured_credential__false)
{
    settings::http_server instance{ "test" };
    instance.credentials.emplace_back("username:password");
    BC_REQUIRE(!instance.authorized(config::credential{ "username:other" }.digest()));
}

BC_AUTO_TEST_CASE(settings__websocket_server__defaults__expected)
{
    constexpr auto name = "test";
    const settings::websocket_server instance{ name };

    // tcp_server
    BC_REQUIRE_EQUAL(instance.name, name);
    BC_REQUIRE(instance.binds.empty());
    BC_REQUIRE_EQUAL(instance.connections, 0u);
    BC_REQUIRE_EQUAL(instance.inactivity_minutes, 10u);
    BC_REQUIRE_EQUAL(instance.expiration_minutes, 60u);
    BC_REQUIRE_EQUAL(instance.maximum_request, maximum_request);
    BC_REQUIRE_EQUAL(instance.minimum_buffer, minimum_buffer);
    BC_REQUIRE(!instance.enabled());
    BC_REQUIRE(instance.inactivity() == minutes(10));
    BC_REQUIRE(instance.expiration() == minutes(60));

    // tls_server
    BC_REQUIRE(!instance.secure());
    BC_REQUIRE(instance.safes.empty());
    BC_REQUIRE(instance.cert_auth.empty());
    BC_REQUIRE(instance.cert_path.empty());
    BC_REQUIRE(instance.key_path.empty());
    BC_REQUIRE(instance.key_pass.empty());

    // http_server
    BC_REQUIRE_EQUAL(instance.server, BC_HTTP_SERVER_NAME);
    BC_REQUIRE(instance.hosts.empty());
    BC_REQUIRE(instance.origins.empty());
    BC_REQUIRE(instance.host_names().empty());
    BC_REQUIRE(instance.origin_names().empty());
    BC_REQUIRE(!instance.allow_opaque_origin);
    BC_REQUIRE(!instance.authorize());
    BC_REQUIRE(instance.credentials.empty());
    // websocket_server (no unique settings yet)
}

BC_AUTO_TEST_CASE(settings__peer_outbound__mainnet__expected)
{
    const settings::peer_outbound instance{ system::chain::selection::mainnet };

    // socks5
    BC_REQUIRE(!instance.authenticated());
    BC_REQUIRE(instance.username.empty());
    BC_REQUIRE(instance.password.empty());
    BC_REQUIRE(!instance.proxied());
    BC_REQUIRE(instance.socks == config::endpoint{});

    // tcp_server
    BC_REQUIRE_EQUAL(instance.name, "outbound");
    BC_REQUIRE(instance.binds.empty());
    BC_REQUIRE_EQUAL(instance.connections, 100u);
    BC_REQUIRE_EQUAL(instance.inactivity_minutes, 10u);
    BC_REQUIRE_EQUAL(instance.expiration_minutes, 60u);
    BC_REQUIRE_EQUAL(instance.maximum_request, maximum_request);
    BC_REQUIRE_EQUAL(instance.minimum_buffer, minimum_buffer);
    BC_REQUIRE(instance.enabled());
    BC_REQUIRE(instance.inactivity() == minutes(10));
    BC_REQUIRE(instance.expiration() == minutes(60));

    // outbound
    BC_REQUIRE_EQUAL(instance.connect_batch_size, 5u);
    BC_REQUIRE_EQUAL(instance.host_pool_capacity, 10000u);
    BC_REQUIRE_EQUAL(instance.seeding_timeout_seconds, 30u);
    BC_REQUIRE_EQUAL(instance.seeds.size(), 8u);
    BC_REQUIRE_EQUAL(instance.minimum_address_count(), 500u);
    BC_REQUIRE(instance.seeding_timeout() == seconds(30));
}

// gossip

BC_AUTO_TEST_CASE(settings__gossip__default__ipv4_only)
{
    const settings instance{ selection::mainnet };
    BC_REQUIRE(instance.gossip_ipv4);
    BC_REQUIRE(!instance.gossip_ipv6);
    BC_REQUIRE(!instance.gossip_tor);
    BC_REQUIRE(!instance.gossip_i2p);
}

BC_AUTO_TEST_CASE(settings__connectable__ip__true)
{
    const settings instance{ selection::mainnet };
    BC_REQUIRE(instance.connectable(config::address{ "42.42.42.42:42" }));
    BC_REQUIRE(instance.connectable(config::address{ "[2001:db8::2]:42" }));
}

BC_AUTO_TEST_CASE(settings__connectable__cjdns__true)
{
    const settings instance{ selection::mainnet };
    BC_REQUIRE(instance.connectable(address_item{ 0, 0, cjdns_t{}, 42 }));
}

BC_AUTO_TEST_CASE(settings__connectable__not_ip__false)
{
    const settings instance{ selection::mainnet };
    BC_REQUIRE(!instance.connectable(address_item{ 0, 0, torv2_t{}, 42 }));
    BC_REQUIRE(!instance.connectable(address_item{ 0, 0, torv3_t{}, 42 }));
    BC_REQUIRE(!instance.connectable(address_item{ 0, 0, i2p_t{}, 42 }));
    BC_REQUIRE(!instance.connectable(address_item{ 0, 0, {}, 42 }));
}

BC_AUTO_TEST_CASE(settings__connectable__proxied__true)
{
    settings instance{ selection::mainnet };
    instance.outbound.socks = { "127.0.0.1:9050" };
    BC_REQUIRE(instance.connectable(address_item{ 0, 0, torv3_t{}, 42 }));
    BC_REQUIRE(instance.connectable(address_item{ 0, 0, i2p_t{}, 42 }));
    BC_REQUIRE(instance.connectable(config::address{ "42.42.42.42:42" }));

    // Tor v2 has no host name form, so remains unroutable.
    BC_REQUIRE(!instance.connectable(address_item{ 0, 0, torv2_t{}, 42 }));
}

BC_AUTO_TEST_CASE(settings__gossiped__ipv4__gossip_ipv4)
{
    settings instance{ selection::mainnet };
    instance.gossip_ipv4 = false;
    BC_REQUIRE(!instance.gossiped(config::address{ "42.42.42.42:42" }));

    instance.gossip_ipv4 = true;
    BC_REQUIRE(instance.gossiped(config::address{ "42.42.42.42:42" }));
}

BC_AUTO_TEST_CASE(settings__gossiped__ipv6__gossip_ipv6)
{
    settings instance{ selection::mainnet };
    instance.gossip_ipv6 = false;
    BC_REQUIRE(!instance.gossiped(config::address{ "[2001:db8::2]:42" }));

    instance.gossip_ipv6 = true;
    BC_REQUIRE(instance.gossiped(config::address{ "[2001:db8::2]:42" }));
}

BC_AUTO_TEST_CASE(settings__gossiped__cjdns__gossip_ipv6)
{
    // Cjdns is an ipv6 address, governed by the ipv6 setting.
    settings instance{ selection::mainnet };
    instance.gossip_ipv6 = true;
    BC_REQUIRE(instance.gossiped(address_item{ 0, 0, cjdns_t{}, 42 }));

    instance.gossip_ipv6 = false;
    BC_REQUIRE(!instance.gossiped(address_item{ 0, 0, cjdns_t{}, 42 }));
}

BC_AUTO_TEST_CASE(settings__gossiped__tor__gossip_tor)
{
    settings instance{ selection::mainnet };
    instance.gossip_tor = true;
    BC_REQUIRE(instance.gossiped(address_item{ 0, 0, torv3_t{}, 42 }));
    BC_REQUIRE(!instance.gossiped(address_item{ 0, 0, torv2_t{}, 42 }));

    instance.gossip_tor = false;
    BC_REQUIRE(!instance.gossiped(address_item{ 0, 0, torv3_t{}, 42 }));
    BC_REQUIRE(!instance.gossiped(address_item{ 0, 0, torv2_t{}, 42 }));
}

BC_AUTO_TEST_CASE(settings__gossiped__i2p__gossip_i2p)
{
    settings instance{ selection::mainnet };
    instance.gossip_i2p = true;
    BC_REQUIRE(instance.gossiped(address_item{ 0, 0, i2p_t{}, 42 }));

    instance.gossip_i2p = false;
    BC_REQUIRE(!instance.gossiped(address_item{ 0, 0, i2p_t{}, 42 }));
}

BC_AUTO_TEST_CASE(settings__gossiped__unspecified__false)
{
    settings instance{ selection::mainnet };
    instance.gossip_ipv4 = true;
    instance.gossip_ipv6 = true;
    instance.gossip_tor = true;
    instance.gossip_i2p = true;
    BC_REQUIRE(!instance.gossiped(address_item{}));
}

BC_AUTO_TEST_CASE(settings__gossip_v2__privacy_networks__expected)
{
    // Only tor and i2p addresses require the v2 address protocol.
    settings instance{ selection::mainnet };
    BC_REQUIRE(!instance.gossip_v2());

    instance.gossip_ipv6 = true;
    BC_REQUIRE(!instance.gossip_v2());

    instance.gossip_tor = true;
    BC_REQUIRE(instance.gossip_v2());

    instance.gossip_tor = false;
    instance.gossip_i2p = true;
    BC_REQUIRE(instance.gossip_v2());
}

BC_AUTO_TEST_CASE(settings__peer_outbound_enabled__true_true_true__true)
{
    settings::peer_outbound instance{ system::chain::selection::mainnet };
    instance.connections = 42;
    instance.host_pool_capacity = 42;
    instance.connect_batch_size = 42;
    BC_REQUIRE(instance.enabled());
}

BC_AUTO_TEST_CASE(settings__peer_outbound_enabled__true_true_false__false)
{
    settings::peer_outbound instance{ system::chain::selection::mainnet };
    instance.connections = 42;
    instance.host_pool_capacity = 42;
    instance.connect_batch_size = 0;
    BC_REQUIRE(!instance.enabled());
}

BC_AUTO_TEST_CASE(settings__peer_outbound_enabled__true_false_true__false)
{
    settings::peer_outbound instance{ system::chain::selection::mainnet };
    instance.connections = 42;
    instance.host_pool_capacity = 0;
    instance.connect_batch_size = 42;
    BC_REQUIRE(!instance.enabled());
}

BC_AUTO_TEST_CASE(settings__peer_outbound_enabled__false_true_true__false)
{
    settings::peer_outbound instance{ system::chain::selection::mainnet };
    instance.connections = 0;
    instance.host_pool_capacity = 42;
    instance.connect_batch_size = 42;
    BC_REQUIRE(!instance.enabled());
}

BC_AUTO_TEST_CASE(settings__peer_outbound_enabled__false_false_false__false)
{
    settings::peer_outbound instance{ system::chain::selection::mainnet };
    instance.connections = 0;
    instance.host_pool_capacity = 0;
    instance.connect_batch_size = 0;
    BC_REQUIRE(!instance.enabled());
}

BC_AUTO_TEST_CASE(settings__peer_outbound_seeding_timeout__always__seeding_timeout_seconds)
{
    settings::peer_outbound instance{ system::chain::selection::mainnet };
    constexpr auto expected = 42u;
    instance.seeding_timeout_seconds = expected;
    BC_REQUIRE(instance.seeding_timeout() == seconds(expected));
}

BC_AUTO_TEST_CASE(settings__peer_outbound_minimum_address_count__always__outbound_product)
{
    settings::peer_outbound instance{ system::chain::selection::mainnet };
    instance.connect_batch_size = 24;
    instance.connections = 42;
    const size_t product = instance.connect_batch_size * instance.connections;
    BC_REQUIRE_EQUAL(instance.minimum_address_count(), product);
}

BC_AUTO_TEST_CASE(settings__peer_inbound__mainnet__expected)
{
    const settings::peer_inbound instance{ system::chain::selection::mainnet };

    // tcp_server
    BC_REQUIRE_EQUAL(instance.name, "inbound");
    BC_REQUIRE_EQUAL(instance.binds.size(), 1u);
    BC_REQUIRE_EQUAL(instance.connections, 100u);
    BC_REQUIRE_EQUAL(instance.inactivity_minutes, 10u);
    BC_REQUIRE_EQUAL(instance.expiration_minutes, 60u);
    BC_REQUIRE_EQUAL(instance.maximum_request, maximum_request);
    BC_REQUIRE_EQUAL(instance.minimum_buffer, minimum_buffer);
    BC_REQUIRE(instance.enabled());
    BC_REQUIRE(instance.inactivity() == minutes(10));
    BC_REQUIRE(instance.expiration() == minutes(60));

    // inbound
    BC_REQUIRE(!instance.enable_loopback);
    BC_REQUIRE(instance.selfs.empty());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_enabled__zero_empty__false)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.connections = 0;
    instance.binds.clear();
    BC_REQUIRE(!instance.enabled());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_enabled__nonzero_empty__false)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.connections = 42;
    instance.binds.clear();
    BC_REQUIRE(!instance.enabled());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_enabled__zero_nonempty__false)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.connections = 0;
    instance.binds.emplace_back();
    BC_REQUIRE(!instance.enabled());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_enabled__nonzero_nonempty__true)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.connections = 42;
    instance.binds.emplace_back();
    BC_REQUIRE(instance.enabled());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_enabled__nonzero_bridged__true)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.connections = 42;
    instance.binds.clear();
    instance.bridge = { "127.0.0.1:7656" };
    BC_REQUIRE(instance.bridged());
    BC_REQUIRE(instance.enabled());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_advertise__bridged_no_selfs__true)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.connections = 42;
    instance.binds.clear();
    instance.selfs.clear();
    instance.bridge = { "127.0.0.1:7656" };
    BC_REQUIRE(instance.advertise());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_advertise__default__false)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    BC_REQUIRE(!instance.advertise());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_advertise__zero_empty_empty__false)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.connections = 0;
    instance.binds.clear();
    instance.selfs.clear();
    BC_REQUIRE(!instance.advertise());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_advertise__zero_empty_nonempty__false)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.connections = 0;
    instance.binds.clear();
    instance.selfs.emplace_back();
    BC_REQUIRE(!instance.advertise());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_advertise__zero_nonempty_empty__false)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.connections = 0;
    instance.binds.emplace_back();
    instance.selfs.clear();
    BC_REQUIRE(!instance.advertise());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_advertise__zero_nonempty_nonempty__false)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.connections = 0;
    instance.binds.emplace_back();
    instance.selfs.emplace_back();
    BC_REQUIRE(!instance.advertise());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_advertise__nonzero_empty_empty__false)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.connections = 42;
    instance.binds.clear();
    instance.selfs.clear();
    BC_REQUIRE(!instance.advertise());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_advertise__nonzero_nonempty_empty__false)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.connections = 42;
    instance.binds.emplace_back();
    instance.selfs.clear();
    BC_REQUIRE(!instance.advertise());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_advertise__nonzero_empty_nonempty__false)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.connections = 42;
    instance.binds.clear();
    instance.selfs.emplace_back();
    BC_REQUIRE(!instance.advertise());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_advertise__nonzero_nonempty_nonempty__true)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.connections = 42;
    instance.binds.emplace_back();
    instance.selfs.emplace_back();
    BC_REQUIRE(instance.advertise());
}

BC_AUTO_TEST_CASE(settings__peer_inbound_first_self__empty_selfs__default)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.selfs.clear();
    BC_REQUIRE(instance.first_self() == config::address{});
}

BC_AUTO_TEST_CASE(settings__peer_inbound_first_self__multiple_selfs__front)
{
    settings::peer_inbound instance{ system::chain::selection::mainnet };
    instance.selfs.clear();
    instance.selfs.push_back(config::address{ "[::]:18333" });
    instance.selfs.emplace_back();
    BC_REQUIRE_EQUAL(instance.first_self(), instance.selfs.front());
}

BC_AUTO_TEST_CASE(settings__peer_manual__mainnet__expected)
{
    const settings::peer_manual instance{ system::chain::selection::mainnet };

    // socks5
    BC_REQUIRE(!instance.authenticated());
    BC_REQUIRE(instance.username.empty());
    BC_REQUIRE(instance.password.empty());
    BC_REQUIRE(!instance.proxied());
    BC_REQUIRE(instance.socks == config::endpoint{});

    // tcp_server
    BC_REQUIRE_EQUAL(instance.name, "manual");
    BC_REQUIRE(instance.binds.empty());
    BC_REQUIRE_EQUAL(instance.connections, 0u);
    BC_REQUIRE_EQUAL(instance.inactivity_minutes, 10u);
    BC_REQUIRE_EQUAL(instance.expiration_minutes, 60u);
    BC_REQUIRE_EQUAL(instance.maximum_request, maximum_request);
    BC_REQUIRE_EQUAL(instance.minimum_buffer, minimum_buffer);
    BC_REQUIRE(!instance.enabled());
    BC_REQUIRE(instance.inactivity() == minutes(10));
    BC_REQUIRE(instance.expiration() == minutes(60));

    // manual
    BC_REQUIRE(instance.peers.empty());
    BC_REQUIRE(instance.friends.empty());
}

BC_AUTO_TEST_CASE(settings__peer_manual_initialize__configured__expected_port_matching)
{
    settings::peer_manual instance{ system::chain::selection::mainnet };
    instance.peers.clear();
    BC_REQUIRE(!instance.peered(config::address{ "34.222.125.43:8333" }));
    BC_REQUIRE(!instance.peered(config::address{ "51.79.80.166:8333" }));
    BC_REQUIRE(!instance.peered(config::address{ "65.109.113.126:8333" }));
    BC_REQUIRE(!instance.peered(config::address{ "77.21.60.152:8333" }));
    BC_REQUIRE(!instance.peered(config::address{ "86.104.228.11:8333" }));
    BC_REQUIRE(!instance.peered(config::address{ "5.14.19.0:8333" }));
    BC_REQUIRE(!instance.peered(config::address{ "89.35.142.168:8333" }));

    instance.peers.emplace_back("34.222.125.43:8333");
    instance.peers.emplace_back("51.79.80.166:8333");
    instance.peers.emplace_back("65.109.113.126:8333");
    ////instance.peers.emplace_back("77.21.60.152:8333");
    instance.peers.emplace_back("86.104.228.11:8333");
    instance.peers.emplace_back("5.14.19.0");
    instance.peers.emplace_back("89.35.142.168");

    instance.initialize();
    BC_REQUIRE(instance.peered(config::address{ "34.222.125.43:8333" }));
    BC_REQUIRE(instance.peered(config::address{ "51.79.80.166:8333" }));
    BC_REQUIRE(instance.peered(config::address{ "65.109.113.126:8333" }));
    BC_REQUIRE(!instance.peered(config::address{ "77.21.60.152:8333" }));
    BC_REQUIRE(instance.peered(config::address{ "86.104.228.11" }));
    BC_REQUIRE(instance.peered(config::address{ "5.14.19.0:8333" }));
    BC_REQUIRE(instance.peered(config::address{ "89.35.142.168" }));
}

BC_AUTO_TEST_CASE(settings__peer_manual_peered__ipv4_host__expected)
{
    settings::peer_manual instance{ system::chain::selection::mainnet };
    instance.peers.clear();
    BC_REQUIRE(!instance.peered(config::address{ "24.24.24.24" }));

    instance.peers.emplace_back("12.12.12.12");
    BC_REQUIRE(!instance.peered(config::address{ "24.24.24.24" }));

    instance.peers.emplace_back("24.24.24.24");
    BC_REQUIRE(!instance.peered(config::address{ "24.24.24.24" }));

    instance.initialize();
    BC_REQUIRE(instance.peered(config::address{ "24.24.24.24" }));
}

BC_AUTO_TEST_CASE(settings__peer_manual_peered__ipv6_host__expected)
{
    settings::peer_manual instance{ system::chain::selection::mainnet };
    instance.peers.clear();
    BC_REQUIRE(!instance.peered(config::address{ "[2020:db8::3]" }));

    instance.peers.emplace_back("[2020:db8::1]");
    instance.peers.emplace_back("[2020:db8::2]");
    BC_REQUIRE(!instance.peered(config::address{ "[2020:db8::3]" }));

    instance.peers.emplace_back("[2020:db8::3]");
    BC_REQUIRE(!instance.peered(config::address{ "[2020:db8::3]" }));

    instance.initialize();
    BC_REQUIRE(instance.peered(config::address{ "[2020:db8::3]" }));
}

// sam
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(settings_sam__bridged__default__false)
{
    const settings::sam instance{};
    BC_REQUIRE(!instance.bridged());
}

BC_AUTO_TEST_CASE(settings_sam__bridged__bridge_port__true)
{
    settings::sam instance{};
    instance.bridge = { "127.0.0.1:7656" };
    BC_REQUIRE(instance.bridged());
}

BC_AUTO_TEST_CASE(settings_sam__authenticated__default__false)
{
    const settings::sam instance{};
    BC_REQUIRE(!instance.authenticated());
}

BC_AUTO_TEST_CASE(settings_sam__authenticated__username__true)
{
    settings::sam instance{};
    instance.username = "user";
    BC_REQUIRE(instance.authenticated());
}

BC_AUTO_TEST_CASE(settings_sam__authenticated__password__true)
{
    settings::sam instance{};
    instance.password = "pass";
    BC_REQUIRE(instance.authenticated());
}

BC_AUTO_TEST_CASE(settings_sam__to_self__invalid_base64__unspecified)
{
    BC_REQUIRE(settings::sam::to_self("~not base64~") == config::address{});
}

BC_AUTO_TEST_CASE(settings_sam__to_self__short_key__unspecified)
{
    BC_REQUIRE(settings::sam::to_self(system::encode_base64(system::data_chunk(42, 0x00))) == config::address{});
}

BC_AUTO_TEST_CASE(settings_sam__to_self__truncated_certificate__unspecified)
{
    system::data_chunk key(387, 0x00);
    key[385] = 0x00;
    key[386] = 0x01;
    BC_REQUIRE(settings::sam::to_self(system::encode_base64(key)) == config::address{});
}

BC_AUTO_TEST_CASE(settings_sam__to_self__destination_key__expected)
{
    system::data_chunk key(387, 0x42);
    key[385] = 0x00;
    key[386] = 0x00;
    const auto self = settings::sam::to_self(system::encode_base64(key));
    BC_REQUIRE_EQUAL(self.to_host(), "fhda2dy47lj3cckiek2od4f2jevgqthdtb4kuolbwmc32ppnuhaa.b32.i2p");
    BC_REQUIRE_EQUAL(self.port(), 0u);
}

BC_AUTO_TEST_CASE(settings_sam__initialize__default__success_empty_key)
{
    settings::sam instance{};
    BC_REQUIRE_EQUAL(instance.initialize(), error::success);
    BC_REQUIRE(instance.key.empty());
}

BC_AUTO_TEST_CASE(settings_sam__initialize__missing_file__success_empty_key)
{
    settings::sam instance{};
    instance.key_path = { TEST_PATH };
    BC_REQUIRE_EQUAL(instance.initialize(), error::success);
    BC_REQUIRE(instance.key.empty());
}

// secure_server
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(settings_secure_server__secure__default__false)
{
    const settings::secure_server instance{ "test" };
    BC_REQUIRE(!instance.secure());
    BC_REQUIRE(!instance.authenticate());
}

BC_AUTO_TEST_CASE(settings_secure_server__initialize_context__default__success)
{
    settings::secure_server instance{ "test" };
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::success);
}

BC_AUTO_TEST_CASE(settings_secure_server__contexts__default__monostate)
{
    const settings::secure_server instance{ "test" };
    BC_REQUIRE(std::holds_alternative<std::monostate>(instance.clear_context()));
    BC_REQUIRE(std::holds_alternative<std::monostate>(instance.secure_context()));
}

// tls_server
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(settings_tls_server__secure__default__false)
{
    const settings::tls_server instance{ "test" };
    BC_REQUIRE(!instance.secure());
    BC_REQUIRE(!instance.authenticate());
}

BC_AUTO_TEST_CASE(settings_tls_server__secure__safes_and_paths__true)
{
    settings::tls_server instance{ "test" };
    instance.safes.emplace_back("127.0.0.1:443");
    instance.cert_path = { "cert.pem" };
    instance.key_path = { "key.pem" };
    BC_REQUIRE(instance.secure());
    BC_REQUIRE(!instance.authenticate());
}

BC_AUTO_TEST_CASE(settings_tls_server__authenticate__cert_auth__true)
{
    settings::tls_server instance{ "test" };
    instance.safes.emplace_back("127.0.0.1:443");
    instance.cert_path = { "cert.pem" };
    instance.key_path = { "key.pem" };
    instance.cert_auth = { "authority" };
    BC_REQUIRE(instance.authenticate());
}

BC_AUTO_TEST_CASE(settings_tls_server__authenticate__cert_auth_unsecured__false)
{
    settings::tls_server instance{ "test" };
    instance.cert_auth = { "authority" };
    BC_REQUIRE(!instance.authenticate());
}

BC_AUTO_TEST_CASE(settings_tls_server__initialize_context__unsecured__success)
{
    settings::tls_server instance{ "test" };
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::success);
    BC_REQUIRE(instance.context);
}

BC_AUTO_TEST_CASE(settings_tls_server__initialize_context__twice__operation_failed)
{
    settings::tls_server instance{ "test" };
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::success);
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::operation_failed);
}

BC_AUTO_TEST_CASE(settings_tls_server__initialize_context__missing_certificate__tls_use_certificate)
{
    settings::tls_server instance{ "test" };
    instance.safes.emplace_back("127.0.0.1:443");
    instance.cert_path = { TEST_PATH };
    instance.key_path = { TEST_PATH };
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::tls_use_certificate);
}

BC_AUTO_TEST_CASE(settings_tls_server__initialize_context__safes_without_certificate_path__tls_use_certificate)
{
    settings::tls_server instance{ "test" };
    instance.safes.emplace_back("127.0.0.1:443");
    instance.key_path = { TEST_PATH };
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::tls_use_certificate);
}

BC_AUTO_TEST_CASE(settings_tls_server__initialize_context__safes_without_key_path__tls_use_private_key)
{
    settings::tls_server instance{ "test" };
    instance.safes.emplace_back("127.0.0.1:443");
    instance.cert_path = { TEST_PATH };
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::tls_use_private_key);
}

BC_AUTO_TEST_CASE(settings_tls_server__secure_context__initialized__tls_context)
{
    settings::tls_server instance{ "test" };
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::success);
    BC_REQUIRE(std::holds_alternative<cref<tls::context>>(instance.secure_context()));
}

// zmtp_server
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(settings_zmtp_server__secure__default__false)
{
    const settings::zmtp_server instance{ "test" };
    BC_REQUIRE(!instance.secure());
    BC_REQUIRE(!instance.authenticate());
}

BC_AUTO_TEST_CASE(settings_zmtp_server__initialize_context__default__success)
{
    settings::zmtp_server instance{ "test" };
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::success);
    BC_REQUIRE(instance.context);
}

BC_AUTO_TEST_CASE(settings_zmtp_server__initialize_context__twice__operation_failed)
{
    settings::zmtp_server instance{ "test" };
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::success);
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::operation_failed);
}

BC_AUTO_TEST_CASE(settings_zmtp_server__initialize_context__secure_key_and_certificate__success_curve)
{
    settings::zmtp_server instance{ "test" };
    instance.safes.emplace_back("127.0.0.1:443");
    instance.key = system::config::base85{ system::data_chunk(32, 0x42) };
    instance.certs.emplace_back(system::data_chunk(32, 0x24));
    BC_REQUIRE(instance.secure());
    BC_REQUIRE(instance.authenticate());
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::success);
    BC_REQUIRE(instance.context->curve());
}

BC_AUTO_TEST_CASE(settings_zmtp_server__initialize_context__short_key__invalid_configuration)
{
    settings::zmtp_server instance{ "test" };
    instance.safes.emplace_back("127.0.0.1:443");
    instance.key = system::config::base85{ system::data_chunk(31, 0x42) };
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::invalid_configuration);
}

BC_AUTO_TEST_CASE(settings_zmtp_server__initialize_context__short_certificate__invalid_configuration)
{
    settings::zmtp_server instance{ "test" };
    instance.safes.emplace_back("127.0.0.1:443");
    instance.key = system::config::base85{ system::data_chunk(32, 0x42) };
    instance.certs.emplace_back(system::data_chunk(31, 0x24));
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::invalid_configuration);
}

BC_AUTO_TEST_CASE(settings_zmtp_server__clear_context__default__zmtp_context)
{
    const settings::zmtp_server instance{ "test" };
    BC_REQUIRE(std::holds_alternative<ref<const zmtp::context>>(instance.clear_context()));
}

BC_AUTO_TEST_CASE(settings_zmtp_server__secure_context__initialized__zmtp_context)
{
    settings::zmtp_server instance{ "test" };
    BC_REQUIRE_EQUAL(instance.initialize_context(), error::success);
    BC_REQUIRE(std::holds_alternative<ref<const zmtp::context>>(instance.secure_context()));
}

// settings
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(settings__construct__invalid_context__zero_identifier)
{
    const settings instance{ static_cast<system::chain::selection>(0xff) };
    BC_REQUIRE_EQUAL(instance.identifier, 0u);
}

BC_AUTO_TEST_CASE(settings__initialize__default__success_peer_identifier)
{
    settings instance{ system::chain::selection::mainnet };
    BC_REQUIRE_EQUAL(instance.initialize(), error::success);
    BC_REQUIRE_EQUAL(instance.peer.identifier, instance.identifier);
}

BC_AUTO_TEST_CASE(settings__peer_context__always__peer)
{
    const settings instance{ system::chain::selection::mainnet };
    BC_REQUIRE(std::holds_alternative<ref<const p2ps::context>>(instance.peer_context()));
}

BC_AUTO_TEST_CASE(settings__file__configured_path__hosts_cache)
{
    settings instance{ system::chain::selection::mainnet };
    instance.path = { TEST_DIRECTORY };
    BC_REQUIRE_EQUAL(instance.file().filename(), "hosts.cache");
}

BC_AUTO_TEST_SUITE_END()
