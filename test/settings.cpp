/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(settings_tests)

using namespace bc::system::chain;
using namespace messages;

BOOST_AUTO_TEST_CASE(settings__construct__default__expected)
{
    settings instance{};

    // [network]
    BOOST_REQUIRE_EQUAL(instance.threads, 1u);
    BOOST_REQUIRE_EQUAL(instance.address_upper, 10u);
    BOOST_REQUIRE_EQUAL(instance.address_lower, 5u);
    BOOST_REQUIRE_EQUAL(instance.protocol_maximum, level::maximum_protocol);
    BOOST_REQUIRE_EQUAL(instance.protocol_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(instance.services_maximum, service::maximum_services);
    BOOST_REQUIRE_EQUAL(instance.services_minimum, service::minimum_services);
    BOOST_REQUIRE_EQUAL(instance.invalid_services, 176u);
    BOOST_REQUIRE_EQUAL(instance.enable_address, false);
    BOOST_REQUIRE_EQUAL(instance.enable_alert, false);
    BOOST_REQUIRE_EQUAL(instance.enable_reject, false);
    BOOST_REQUIRE_EQUAL(instance.enable_transaction, false);
    BOOST_REQUIRE_EQUAL(instance.enable_ipv6, false);
    BOOST_REQUIRE_EQUAL(instance.enable_loopback, false);
    BOOST_REQUIRE_EQUAL(instance.validate_checksum, false);
    BOOST_REQUIRE_EQUAL(instance.identifier, 0u);
    BOOST_REQUIRE_EQUAL(instance.inbound_connections, 0u);
    BOOST_REQUIRE_EQUAL(instance.outbound_connections, 10u);
    BOOST_REQUIRE_EQUAL(instance.connect_batch_size, 5u);
    BOOST_REQUIRE_EQUAL(instance.retry_timeout_seconds, 1u);
    BOOST_REQUIRE_EQUAL(instance.connect_timeout_seconds, 5u);
    BOOST_REQUIRE_EQUAL(instance.handshake_timeout_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.seeding_timeout_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.channel_heartbeat_minutes, 5u);
    BOOST_REQUIRE_EQUAL(instance.channel_inactivity_minutes, 10u);
    BOOST_REQUIRE_EQUAL(instance.channel_expiration_minutes, 1440u);
    BOOST_REQUIRE_EQUAL(instance.host_pool_capacity, 0u);
    BOOST_REQUIRE_EQUAL(instance.minimum_buffer, heading::maximum_payload(level::canonical, true));
    BOOST_REQUIRE_EQUAL(instance.rate_limit, 1024u);
    BOOST_REQUIRE_EQUAL(instance.user_agent, BC_USER_AGENT);
    BOOST_REQUIRE(instance.path.empty());
    BOOST_REQUIRE(instance.peers.empty());
    BOOST_REQUIRE(instance.selfs.empty());
    BOOST_REQUIRE(instance.binds.empty());
    BOOST_REQUIRE(instance.blacklists.empty());
    BOOST_REQUIRE(instance.whitelists.empty());
    BOOST_REQUIRE(instance.friends.empty());
}

BOOST_AUTO_TEST_CASE(settings__construct__mainnet__expected)
{
    settings instance(selection::mainnet);

    // unchanged from default
    BOOST_REQUIRE_EQUAL(instance.threads, 1u);
    BOOST_REQUIRE_EQUAL(instance.address_upper, 10u);
    BOOST_REQUIRE_EQUAL(instance.address_lower, 5u);
    BOOST_REQUIRE_EQUAL(instance.protocol_maximum, level::maximum_protocol);
    BOOST_REQUIRE_EQUAL(instance.protocol_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(instance.services_maximum, service::maximum_services);
    BOOST_REQUIRE_EQUAL(instance.services_minimum, service::minimum_services);
    BOOST_REQUIRE_EQUAL(instance.invalid_services, 176u);
    BOOST_REQUIRE_EQUAL(instance.enable_address, false);
    BOOST_REQUIRE_EQUAL(instance.enable_alert, false);
    BOOST_REQUIRE_EQUAL(instance.enable_reject, false);
    BOOST_REQUIRE_EQUAL(instance.enable_transaction, false);
    BOOST_REQUIRE_EQUAL(instance.enable_ipv6, false);
    BOOST_REQUIRE_EQUAL(instance.enable_loopback, false);
    BOOST_REQUIRE_EQUAL(instance.validate_checksum, false);
    BOOST_REQUIRE_EQUAL(instance.inbound_connections, 0u);
    BOOST_REQUIRE_EQUAL(instance.outbound_connections, 10u);
    BOOST_REQUIRE_EQUAL(instance.connect_batch_size, 5u);
    BOOST_REQUIRE_EQUAL(instance.retry_timeout_seconds, 1u);
    BOOST_REQUIRE_EQUAL(instance.connect_timeout_seconds, 5u);
    BOOST_REQUIRE_EQUAL(instance.handshake_timeout_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.seeding_timeout_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.channel_heartbeat_minutes, 5u);
    BOOST_REQUIRE_EQUAL(instance.channel_inactivity_minutes, 10u);
    BOOST_REQUIRE_EQUAL(instance.channel_expiration_minutes, 1440u);
    BOOST_REQUIRE_EQUAL(instance.host_pool_capacity, 0u);
    BOOST_REQUIRE_EQUAL(instance.minimum_buffer, heading::maximum_payload(level::canonical, true));
    BOOST_REQUIRE_EQUAL(instance.rate_limit, 1024u);
    BOOST_REQUIRE_EQUAL(instance.user_agent, BC_USER_AGENT);
    BOOST_REQUIRE(instance.path.empty());
    BOOST_REQUIRE(instance.peers.empty());
    BOOST_REQUIRE(instance.selfs.empty());
    BOOST_REQUIRE(instance.blacklists.empty());
    BOOST_REQUIRE(instance.whitelists.empty());
    BOOST_REQUIRE(instance.friends.empty());

    // changed from default
    BOOST_REQUIRE_EQUAL(instance.identifier, 3652501241u);
    BOOST_REQUIRE_EQUAL(instance.binds.size(), 1u);
    BOOST_REQUIRE_EQUAL(instance.seeds.size(), 4u);

    const auto bind0 = config::authority{ asio::address{}, 8333 };
    BOOST_REQUIRE_EQUAL(instance.binds[0], bind0);

    const auto seed0 = config::endpoint{ "mainnet1.libbitcoin.net", 8333 };
    const auto seed1 = config::endpoint{ "mainnet2.libbitcoin.net", 8333 };
    const auto seed2 = config::endpoint{ "mainnet3.libbitcoin.net", 8333 };
    const auto seed3 = config::endpoint{ "mainnet4.libbitcoin.net", 8333 };
    BOOST_REQUIRE_EQUAL(instance.seeds[0], seed0);
    BOOST_REQUIRE_EQUAL(instance.seeds[1], seed1);
    BOOST_REQUIRE_EQUAL(instance.seeds[2], seed2);
    BOOST_REQUIRE_EQUAL(instance.seeds[3], seed3);
}

BOOST_AUTO_TEST_CASE(settings__construct__testnet__expected)
{
    settings instance(selection::testnet);

    // unchanged from default
    BOOST_REQUIRE_EQUAL(instance.threads, 1u);
    BOOST_REQUIRE_EQUAL(instance.address_upper, 10u);
    BOOST_REQUIRE_EQUAL(instance.address_lower, 5u);
    BOOST_REQUIRE_EQUAL(instance.protocol_maximum, level::maximum_protocol);
    BOOST_REQUIRE_EQUAL(instance.protocol_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(instance.services_maximum, service::maximum_services);
    BOOST_REQUIRE_EQUAL(instance.services_minimum, service::minimum_services);
    BOOST_REQUIRE_EQUAL(instance.invalid_services, 176u);
    BOOST_REQUIRE_EQUAL(instance.enable_address, false);
    BOOST_REQUIRE_EQUAL(instance.enable_alert, false);
    BOOST_REQUIRE_EQUAL(instance.enable_reject, false);
    BOOST_REQUIRE_EQUAL(instance.enable_transaction, false);
    BOOST_REQUIRE_EQUAL(instance.enable_ipv6, false);
    BOOST_REQUIRE_EQUAL(instance.enable_loopback, false);
    BOOST_REQUIRE_EQUAL(instance.validate_checksum, false);
    BOOST_REQUIRE_EQUAL(instance.inbound_connections, 0u);
    BOOST_REQUIRE_EQUAL(instance.outbound_connections, 10u);
    BOOST_REQUIRE_EQUAL(instance.connect_batch_size, 5u);
    BOOST_REQUIRE_EQUAL(instance.retry_timeout_seconds, 1u);
    BOOST_REQUIRE_EQUAL(instance.connect_timeout_seconds, 5u);
    BOOST_REQUIRE_EQUAL(instance.handshake_timeout_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.seeding_timeout_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.channel_heartbeat_minutes, 5u);
    BOOST_REQUIRE_EQUAL(instance.channel_inactivity_minutes, 10u);
    BOOST_REQUIRE_EQUAL(instance.channel_expiration_minutes, 1440u);
    BOOST_REQUIRE_EQUAL(instance.host_pool_capacity, 0u);
    BOOST_REQUIRE_EQUAL(instance.minimum_buffer, heading::maximum_payload(level::canonical, true));
    BOOST_REQUIRE_EQUAL(instance.rate_limit, 1024u);
    BOOST_REQUIRE_EQUAL(instance.user_agent, BC_USER_AGENT);
    BOOST_REQUIRE(instance.path.empty());
    BOOST_REQUIRE(instance.peers.empty());
    BOOST_REQUIRE(instance.selfs.empty());
    BOOST_REQUIRE(instance.blacklists.empty());
    BOOST_REQUIRE(instance.whitelists.empty());
    BOOST_REQUIRE(instance.friends.empty());

    // changed from default
    BOOST_REQUIRE_EQUAL(instance.identifier, 118034699u);
    BOOST_REQUIRE_EQUAL(instance.binds.size(), 1u);
    BOOST_REQUIRE_EQUAL(instance.seeds.size(), 4u);

    const auto bind0 = config::authority{ asio::address{}, 18333 };
    BOOST_REQUIRE_EQUAL(instance.binds[0], bind0);

    const auto seed0 = config::endpoint{ "testnet1.libbitcoin.net", 18333 };
    const auto seed1 = config::endpoint{ "testnet2.libbitcoin.net", 18333 };
    const auto seed2 = config::endpoint{ "testnet3.libbitcoin.net", 18333 };
    const auto seed3 = config::endpoint{ "testnet4.libbitcoin.net", 18333 };
    BOOST_REQUIRE_EQUAL(instance.seeds[0], seed0);
    BOOST_REQUIRE_EQUAL(instance.seeds[1], seed1);
    BOOST_REQUIRE_EQUAL(instance.seeds[2], seed2);
    BOOST_REQUIRE_EQUAL(instance.seeds[3], seed3);
}

BOOST_AUTO_TEST_CASE(settings__construct__regtest__expected)
{
    settings instance(selection::regtest);

    // unchanged from default
    BOOST_REQUIRE_EQUAL(instance.threads, 1u);
    BOOST_REQUIRE_EQUAL(instance.address_upper, 10u);
    BOOST_REQUIRE_EQUAL(instance.address_lower, 5u);
    BOOST_REQUIRE_EQUAL(instance.protocol_maximum, level::maximum_protocol);
    BOOST_REQUIRE_EQUAL(instance.protocol_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(instance.services_maximum, service::maximum_services);
    BOOST_REQUIRE_EQUAL(instance.services_minimum, service::minimum_services);
    BOOST_REQUIRE_EQUAL(instance.invalid_services, 176u);
    BOOST_REQUIRE_EQUAL(instance.enable_address, false);
    BOOST_REQUIRE_EQUAL(instance.enable_alert, false);
    BOOST_REQUIRE_EQUAL(instance.enable_reject, false);
    BOOST_REQUIRE_EQUAL(instance.enable_transaction, false);
    BOOST_REQUIRE_EQUAL(instance.enable_ipv6, false);
    BOOST_REQUIRE_EQUAL(instance.enable_loopback, false);
    BOOST_REQUIRE_EQUAL(instance.validate_checksum, false);
    BOOST_REQUIRE_EQUAL(instance.inbound_connections, 0u);
    BOOST_REQUIRE_EQUAL(instance.outbound_connections, 10u);
    BOOST_REQUIRE_EQUAL(instance.connect_batch_size, 5u);
    BOOST_REQUIRE_EQUAL(instance.retry_timeout_seconds, 1u);
    BOOST_REQUIRE_EQUAL(instance.connect_timeout_seconds, 5u);
    BOOST_REQUIRE_EQUAL(instance.handshake_timeout_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.seeding_timeout_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.channel_heartbeat_minutes, 5u);
    BOOST_REQUIRE_EQUAL(instance.channel_inactivity_minutes, 10u);
    BOOST_REQUIRE_EQUAL(instance.channel_expiration_minutes, 1440u);
    BOOST_REQUIRE_EQUAL(instance.host_pool_capacity, 0u);
    BOOST_REQUIRE_EQUAL(instance.minimum_buffer, heading::maximum_payload(level::canonical, true));
    BOOST_REQUIRE_EQUAL(instance.rate_limit, 1024u);
    BOOST_REQUIRE(instance.path.empty());
    BOOST_REQUIRE(instance.peers.empty());
    BOOST_REQUIRE(instance.selfs.empty());
    BOOST_REQUIRE(instance.blacklists.empty());
    BOOST_REQUIRE(instance.whitelists.empty());
    BOOST_REQUIRE(instance.friends.empty());

    // Regtest is private network only, so there is no seeding.
    BOOST_REQUIRE(instance.seeds.empty());

    // changed from default
    BOOST_REQUIRE_EQUAL(instance.identifier, 3669344250u);
    BOOST_REQUIRE_EQUAL(instance.binds.size(), 1u);

    const auto bind0 = config::authority{ asio::address{}, 18444 };
    BOOST_REQUIRE_EQUAL(instance.binds[0], bind0);
}

BOOST_AUTO_TEST_CASE(settings__witness_node__default__false)
{
    settings instance{};
    BOOST_REQUIRE(!instance.witness_node());
}

BOOST_AUTO_TEST_CASE(settings__witness_node__node_witness__true)
{
    settings instance{};
    instance.services_minimum = service::node_witness;
    BOOST_REQUIRE(instance.witness_node());
}

BOOST_AUTO_TEST_CASE(settings__inbound_enabled__default__false)
{
    settings instance{};
    BOOST_REQUIRE(!instance.inbound_enabled());
}

BOOST_AUTO_TEST_CASE(settings__inbound_enabled__zero_empty__false)
{
    settings instance{};
    instance.inbound_connections = 0;
    instance.binds.clear();
    BOOST_REQUIRE(!instance.inbound_enabled());
}

BOOST_AUTO_TEST_CASE(settings__inbound_enabled__nonzero_empty__false)
{
    settings instance{};
    instance.inbound_connections = 42;
    instance.binds.clear();
    BOOST_REQUIRE(!instance.inbound_enabled());
}

BOOST_AUTO_TEST_CASE(settings__inbound_enabled__zero_nonempty__false)
{
    settings instance{};
    instance.inbound_connections = 0;
    instance.binds.emplace_back();
    BOOST_REQUIRE(!instance.inbound_enabled());
}

BOOST_AUTO_TEST_CASE(settings__inbound_enabled__nonzero_nonempty__true)
{
    settings instance{};
    instance.inbound_connections = 42;
    instance.binds.emplace_back();
    BOOST_REQUIRE(instance.inbound_enabled());
}

BOOST_AUTO_TEST_CASE(settings__outbound_enabled__default__true)
{
    settings instance{};

    // Default host pool capacity is zero.
    BOOST_REQUIRE(!instance.outbound_enabled());
}

BOOST_AUTO_TEST_CASE(settings__outbound_enabled__true_true_true__true)
{
    settings instance{};
    instance.outbound_connections = 42;
    instance.host_pool_capacity = 42;
    instance.connect_batch_size = 42;
    BOOST_REQUIRE(instance.outbound_enabled());
}

BOOST_AUTO_TEST_CASE(settings__outbound_enabled__true_true_false__false)
{
    settings instance{};
    instance.outbound_connections = 42;
    instance.host_pool_capacity = 42;
    instance.connect_batch_size = 0;
    BOOST_REQUIRE(!instance.outbound_enabled());
}

BOOST_AUTO_TEST_CASE(settings__outbound_enabled__true_false_true__false)
{
    settings instance{};
    instance.outbound_connections = 42;
    instance.host_pool_capacity = 0;
    instance.connect_batch_size = 42;
    BOOST_REQUIRE(!instance.outbound_enabled());
}

BOOST_AUTO_TEST_CASE(settings__outbound_enabled__false_true_true__false)
{
    settings instance{};
    instance.outbound_connections = 0;
    instance.host_pool_capacity = 42;
    instance.connect_batch_size = 42;
    BOOST_REQUIRE(!instance.outbound_enabled());
}

BOOST_AUTO_TEST_CASE(settings__outbound_enabled__false_false_false__false)
{
    settings instance{};
    instance.outbound_connections = 0;
    instance.host_pool_capacity = 0;
    instance.connect_batch_size = 0;
    BOOST_REQUIRE(!instance.outbound_enabled());
}

BOOST_AUTO_TEST_CASE(settings__advertise_enabled__default__false)
{
    settings instance{};
    BOOST_REQUIRE(!instance.advertise_enabled());
}

BOOST_AUTO_TEST_CASE(settings__advertise_enabled__zero_empty_empty__false)
{
    settings instance{};
    instance.inbound_connections = 0;
    instance.binds.clear();
    instance.selfs.clear();
    BOOST_REQUIRE(!instance.advertise_enabled());
}

BOOST_AUTO_TEST_CASE(settings__advertise_enabled__zero_empty_nonempty__false)
{
    settings instance{};
    instance.inbound_connections = 0;
    instance.binds.clear();
    instance.selfs.emplace_back();
    BOOST_REQUIRE(!instance.advertise_enabled());
}

BOOST_AUTO_TEST_CASE(settings__advertise_enabled__zero_nonempty_empty__false)
{
    settings instance{};
    instance.inbound_connections = 0;
    instance.binds.emplace_back();
    instance.selfs.clear();
    BOOST_REQUIRE(!instance.advertise_enabled());
}

BOOST_AUTO_TEST_CASE(settings__advertise_enabled__zero_nonempty_nonempty__false)
{
    settings instance{};
    instance.inbound_connections = 0;
    instance.binds.emplace_back();
    instance.selfs.emplace_back();
    BOOST_REQUIRE(!instance.advertise_enabled());
}

BOOST_AUTO_TEST_CASE(settings__advertise_enabled__nonzero_empty_empty__false)
{
    settings instance{};
    instance.inbound_connections = 42;
    instance.binds.clear();
    instance.selfs.clear();
    BOOST_REQUIRE(!instance.advertise_enabled());
}

BOOST_AUTO_TEST_CASE(settings__advertise_enabled__nonzero_nonempty_empty__false)
{
    settings instance{};
    instance.inbound_connections = 42;
    instance.binds.emplace_back();
    instance.selfs.clear();
    BOOST_REQUIRE(!instance.advertise_enabled());
}

BOOST_AUTO_TEST_CASE(settings__advertise_enabled__nonzero_empty_nonempty__false)
{
    settings instance{};
    instance.inbound_connections = 42;
    instance.binds.clear();
    instance.selfs.emplace_back();
    BOOST_REQUIRE(!instance.advertise_enabled());
}

BOOST_AUTO_TEST_CASE(settings__advertise_enabled__nonzero_nonempty_nonempty__true)
{
    settings instance{};
    instance.inbound_connections = 42;
    instance.binds.emplace_back();
    instance.selfs.emplace_back();
    BOOST_REQUIRE(instance.advertise_enabled());
}

BOOST_AUTO_TEST_CASE(settings__first_self__empty_selfs__default)
{
    settings instance{};
    instance.selfs.clear();
    BOOST_REQUIRE(!instance.first_self());
}

BOOST_AUTO_TEST_CASE(settings__first_self__multiple_selfs__front)
{
    settings instance{};
    instance.selfs.clear();
    instance.selfs.push_back({ asio::address{}, 18333 });
    instance.selfs.emplace_back();
    BOOST_REQUIRE_EQUAL(instance.first_self(), instance.selfs.front());
}

BOOST_AUTO_TEST_CASE(settings__maximum_payload__default__expected)
{
    settings instance{};
    BOOST_REQUIRE_EQUAL(instance.maximum_payload(), 4'000'000u);
}

BOOST_AUTO_TEST_CASE(settings__maximum_payload__zero_node_none__expected)
{
    settings instance{};
    instance.protocol_maximum = 0;
    instance.services_maximum = service::node_none;
    BOOST_REQUIRE_EQUAL(instance.maximum_payload(), 1'800'003u);
}

BOOST_AUTO_TEST_CASE(settings__maximum_payload__zero_node_witness__expected)
{
    settings instance{};
    instance.protocol_maximum = 0;
    instance.services_maximum = service::node_witness;
    BOOST_REQUIRE_EQUAL(instance.maximum_payload(), 4'000'000u);
}

BOOST_AUTO_TEST_CASE(settings__maximum_payload__maximum_node_witness__expected)
{
    settings instance{};
    instance.protocol_maximum = max_uint32;
    instance.services_maximum = service::node_witness;
    BOOST_REQUIRE_EQUAL(instance.maximum_payload(), 4'000'000u);
}

BOOST_AUTO_TEST_CASE(settings__maximum_payload__maximum_maximum_services__expected)
{
    settings instance{};
    instance.protocol_maximum = max_uint32;
    instance.services_maximum = service::maximum_services;
    BOOST_REQUIRE_EQUAL(instance.maximum_payload(), 4'000'000u);
}

BOOST_AUTO_TEST_CASE(settings__retry_timeout__always__between_zero_and_retry_timeout_seconds)
{
    settings instance{};
    instance.retry_timeout_seconds = 42;
    BOOST_REQUIRE(instance.retry_timeout() > seconds{ zero });
    BOOST_REQUIRE(instance.retry_timeout() <= seconds{ instance.retry_timeout_seconds });
}

BOOST_AUTO_TEST_CASE(settings__connect_timeout__always__between_zero_and_connect_timeout_seconds)
{
    settings instance{};
    instance.connect_timeout_seconds = 42;
    BOOST_REQUIRE(instance.connect_timeout() > seconds{ zero });
    BOOST_REQUIRE(instance.connect_timeout() <= seconds{ instance.connect_timeout_seconds });
}

BOOST_AUTO_TEST_CASE(settings__channel_handshake__always__handshake_timeout_seconds)
{
    settings instance{};
    constexpr auto expected = 42u;
    instance.handshake_timeout_seconds = expected;
    BOOST_REQUIRE(instance.channel_handshake() == seconds(expected));
}

BOOST_AUTO_TEST_CASE(settings__channel_heartbeat__always__channel_heartbeat_minutes)
{
    settings instance{};
    constexpr auto expected = 42u;
    instance.channel_heartbeat_minutes = expected;
    BOOST_REQUIRE(instance.channel_heartbeat() == minutes(expected));
}

BOOST_AUTO_TEST_CASE(settings__channel_inactivity__always__channel_inactivity_minutes)
{
    settings instance{};
    const auto expected = 42u;
    instance.channel_inactivity_minutes = expected;
    BOOST_REQUIRE(instance.channel_inactivity() == minutes(expected));
}

BOOST_AUTO_TEST_CASE(settings__channel_expiration__always__channel_expiration_minutes)
{
    settings instance{};
    constexpr auto expected = 42u;
    instance.channel_expiration_minutes = expected;
    BOOST_REQUIRE(instance.channel_expiration() == minutes(expected));
}

BOOST_AUTO_TEST_CASE(settings__channel_germination__always__seeding_timeout_seconds)
{
    settings instance{};
    constexpr auto expected = 42u;
    instance.seeding_timeout_seconds = expected;
    BOOST_REQUIRE(instance.channel_germination() == seconds(expected));
}

BOOST_AUTO_TEST_CASE(settings__minimum_address_count__always__outbound_product)
{
    settings instance{};
    instance.connect_batch_size = 24;
    instance.outbound_connections = 42;
    const size_t product = instance.connect_batch_size * instance.outbound_connections;
    BOOST_REQUIRE_EQUAL(instance.minimum_address_count(), product);
}

// disabled

BOOST_AUTO_TEST_CASE(settings__disabled__enable_ipv6__both_false)
{
    settings instance{};
    instance.enable_ipv6 = true;
    BOOST_REQUIRE(!instance.disabled(config::address("42.42.42.42:27")));
    BOOST_REQUIRE(!instance.disabled(config::address("[42:42::42:2]:27")));
}

BOOST_AUTO_TEST_CASE(settings__disabled__ipv4__false)
{
    settings instance{};
    instance.enable_ipv6 = false;
    BOOST_REQUIRE(!instance.disabled(config::address{ "42.42.42.42" }));
    BOOST_REQUIRE(!instance.disabled(config::address{ "42.42.42.42:42" }));
    instance.enable_ipv6 = true;
    BOOST_REQUIRE(!instance.disabled(config::address{ "42.42.42.42" }));
    BOOST_REQUIRE(!instance.disabled(config::address{ "42.42.42.42:42" }));
}

BOOST_AUTO_TEST_CASE(settings__disabled__ipv6__expected)
{
    settings instance{};
    instance.enable_ipv6 = false;
    BOOST_REQUIRE(instance.disabled(config::address{ "[2001:db8::2]" }));
    BOOST_REQUIRE(instance.disabled(config::address{ "[2001:db8::2]:42" }));
    instance.enable_ipv6 = true;
    BOOST_REQUIRE(!instance.disabled(config::address{ "[2001:db8::2]" }));
    BOOST_REQUIRE(!instance.disabled(config::address{ "[2001:db8::2]:42" }));
}

// insufficient

BOOST_AUTO_TEST_CASE(settings__insufficient__default__false)
{
    settings instance{};
    constexpr uint64_t services = 0;
    constexpr messages::address_item loop{ 42, services, loopback_ip_address, 8333 };
    instance.services_minimum = 0;
    BOOST_REQUIRE(!instance.insufficient(loop));
    instance.services_minimum = 1;
    BOOST_REQUIRE(instance.insufficient(loop));
}

BOOST_AUTO_TEST_CASE(settings__insufficient__match__expected)
{
    settings instance{};
    constexpr uint64_t services = 0b01010101;
    constexpr messages::address_item loop{ 42, services, loopback_ip_address, 8333 };
    instance.services_minimum = services;
    BOOST_REQUIRE(!instance.insufficient(loop));
    instance.services_minimum = services | 0b00000010;
    BOOST_REQUIRE(instance.insufficient(loop));
    instance.services_minimum = services & 0b11111110;
    BOOST_REQUIRE(!instance.insufficient(loop));
}

// unsupported

BOOST_AUTO_TEST_CASE(settings__unsupported__default__false)
{
    settings instance{};
    constexpr uint64_t services = 0;
    constexpr messages::address_item loop{ 42, services, loopback_ip_address, 8333 };
    instance.invalid_services = 0;
    BOOST_REQUIRE(!instance.unsupported(loop));
    instance.invalid_services = 1;
    BOOST_REQUIRE(!instance.unsupported(loop));
}

BOOST_AUTO_TEST_CASE(settings__unsupported__match__expected)
{
    settings instance{};
    constexpr uint64_t services = 0b01010101;
    constexpr messages::address_item loop{ 42, services, loopback_ip_address, 8333 };
    instance.invalid_services = services;
    BOOST_REQUIRE(instance.unsupported(loop));
    instance.invalid_services = services | 0b00000010;
    BOOST_REQUIRE(instance.unsupported(loop));
    instance.invalid_services = services & 0b11111110;
    BOOST_REQUIRE(instance.unsupported(loop));
    instance.invalid_services = 0b10101010;
    BOOST_REQUIRE(!instance.unsupported(loop));
    instance.invalid_services = 0;
    BOOST_REQUIRE(!instance.unsupported(loop));
}

// whitelisted

BOOST_AUTO_TEST_CASE(settings__whitelisted__ipv4_subnet__expected)
{
    settings instance{};
    instance.whitelists.clear();
    BOOST_REQUIRE(instance.whitelisted(config::address{ "42.42.42.42" }));

    instance.whitelists.emplace_back("12.12.12.12");
    instance.whitelists.emplace_back("24.24.24.24");
    BOOST_REQUIRE(!instance.whitelisted(config::address{ "42.42.42.42" }));

    instance.whitelists.emplace_back("42.42.42.0/24");
    BOOST_REQUIRE(instance.whitelisted(config::address{ "42.42.42.42" }));
}

BOOST_AUTO_TEST_CASE(settings__whitelisted__ipv4_host__expected)
{
    settings instance{};
    instance.whitelists.clear();
    BOOST_REQUIRE(instance.whitelisted(config::address{ "24.24.24.24" }));

    instance.whitelists.emplace_back("12.12.12.12");
    instance.whitelists.emplace_back("42.42.42.0/24");
    BOOST_REQUIRE(!instance.whitelisted(config::address{ "24.24.24.24" }));

    instance.whitelists.emplace_back("24.24.24.24");
    BOOST_REQUIRE(instance.whitelisted(config::address{ "24.24.24.24" }));
}

BOOST_AUTO_TEST_CASE(settings__whitelisted__ipv6_subnet__expected)
{
    settings instance{};
    instance.whitelists.clear();
    BOOST_REQUIRE(instance.whitelisted(config::address{ "[2020:db8::3]" }));

    instance.whitelists.emplace_back("[2020:db8::1]");
    instance.whitelists.emplace_back("[2020:db8::2]");
    BOOST_REQUIRE(!instance.whitelisted(config::address{ "[2020:db8::3]" }));

    instance.whitelists.emplace_back("[2020:db8::2]/64");
    BOOST_REQUIRE(instance.whitelisted(config::address{ "[2020:db8::3]" }));
}

BOOST_AUTO_TEST_CASE(settings__whitelisted__ipv6_host__expected)
{
    settings instance{};
    instance.whitelists.clear();
    BOOST_REQUIRE(instance.whitelisted(config::address{ "[2020:db8::3]" }));
    instance.whitelists.emplace_back("[2020:db8::1]");
    instance.whitelists.emplace_back("[2020:db8::2]");
    BOOST_REQUIRE(!instance.whitelisted(config::address{ "[2020:db8::3]" }));
    instance.whitelists.emplace_back("[2020:db8::3]");
    BOOST_REQUIRE(instance.whitelisted(config::address{ "[2020:db8::3]" }));
}

// blacklisted

BOOST_AUTO_TEST_CASE(settings__blacklisted__ipv4_subnet__expected)
{
    settings instance{};
    instance.blacklists.clear();
    BOOST_REQUIRE(!instance.blacklisted(config::address{ "42.42.42.42" }));

    instance.blacklists.emplace_back("12.12.12.12");
    instance.blacklists.emplace_back("24.24.24.24");
    BOOST_REQUIRE(!instance.blacklisted(config::address{ "42.42.42.42" }));

    instance.blacklists.emplace_back("42.42.42.0/24");
    BOOST_REQUIRE(instance.blacklisted(config::address{ "42.42.42.42" }));
}

BOOST_AUTO_TEST_CASE(settings__blacklisted__ipv4_host__expected)
{
    settings instance{};
    instance.blacklists.clear();
    BOOST_REQUIRE(!instance.blacklisted(config::address{ "24.24.24.24" }));

    instance.blacklists.emplace_back("12.12.12.12");
    instance.blacklists.emplace_back("42.42.42.0/24");
    BOOST_REQUIRE(!instance.blacklisted(config::address{ "24.24.24.24" }));

    instance.blacklists.emplace_back("24.24.24.24");
    BOOST_REQUIRE(instance.blacklisted(config::address{ "24.24.24.24" }));
}

BOOST_AUTO_TEST_CASE(settings__blacklisted__ipv6_subnet__expected)
{
    settings instance{};
    instance.blacklists.clear();
    BOOST_REQUIRE(!instance.blacklisted(config::address{ "[2020:db8::3]" }));

    instance.blacklists.emplace_back("[2020:db8::1]");
    instance.blacklists.emplace_back("[2020:db8::2]");
    BOOST_REQUIRE(!instance.blacklisted(config::address{ "[2020:db8::3]" }));

    instance.blacklists.emplace_back("[2020:db8::2]/64");
    BOOST_REQUIRE(instance.blacklisted(config::address{ "[2020:db8::3]" }));
}

BOOST_AUTO_TEST_CASE(settings__blacklisted__ipv6_host__expected)
{
    settings instance{};
    instance.blacklists.clear();
    BOOST_REQUIRE(!instance.blacklisted(config::address{ "[2020:db8::3]" }));

    instance.blacklists.emplace_back("[2020:db8::1]");
    instance.blacklists.emplace_back("[2020:db8::2]");
    BOOST_REQUIRE(!instance.blacklisted(config::address{ "[2020:db8::3]" }));

    instance.blacklists.emplace_back("[2020:db8::3]");
    BOOST_REQUIRE(instance.blacklisted(config::address{ "[2020:db8::3]" }));
}

// peered/initialize

BOOST_AUTO_TEST_CASE(settings__initialize__configured__expected_port_matching)
{
    settings instance{};
    instance.peers.clear();
    BOOST_REQUIRE(!instance.peered(config::address{ "34.222.125.43:8333" }));
    BOOST_REQUIRE(!instance.peered(config::address{ "51.79.80.166:8333" }));
    BOOST_REQUIRE(!instance.peered(config::address{ "65.109.113.126:8333" }));
    BOOST_REQUIRE(!instance.peered(config::address{ "77.21.60.152:8333" }));
    BOOST_REQUIRE(!instance.peered(config::address{ "86.104.228.11:8333" }));
    BOOST_REQUIRE(!instance.peered(config::address{ "5.14.19.0:8333" }));
    BOOST_REQUIRE(!instance.peered(config::address{ "89.35.142.168:8333" }));

    instance.peers.emplace_back("34.222.125.43:8333");
    instance.peers.emplace_back("51.79.80.166:8333");
    instance.peers.emplace_back("65.109.113.126:8333");
    ////instance.peers.emplace_back("77.21.60.152:8333");
    instance.peers.emplace_back("86.104.228.11:8333");
    instance.peers.emplace_back("5.14.19.0");
    instance.peers.emplace_back("89.35.142.168");

    instance.initialize();
    BOOST_REQUIRE(instance.peered(config::address{ "34.222.125.43:8333" }));
    BOOST_REQUIRE(instance.peered(config::address{ "51.79.80.166:8333" }));
    BOOST_REQUIRE(instance.peered(config::address{ "65.109.113.126:8333" }));
    BOOST_REQUIRE(!instance.peered(config::address{ "77.21.60.152:8333" }));
    BOOST_REQUIRE(instance.peered(config::address{ "86.104.228.11" }));
    BOOST_REQUIRE(instance.peered(config::address{ "5.14.19.0:8333" }));
    BOOST_REQUIRE(instance.peered(config::address{ "89.35.142.168" }));
}

BOOST_AUTO_TEST_CASE(settings__peered__ipv4_host__expected)
{
    settings instance{};
    instance.peers.clear();
    BOOST_REQUIRE(!instance.peered(config::address{ "24.24.24.24" }));

    instance.peers.emplace_back("12.12.12.12");
    BOOST_REQUIRE(!instance.peered(config::address{ "24.24.24.24" }));

    instance.peers.emplace_back("24.24.24.24");
    BOOST_REQUIRE(!instance.peered(config::address{ "24.24.24.24" }));

    instance.initialize();
    BOOST_REQUIRE(instance.peered(config::address{ "24.24.24.24" }));
}

BOOST_AUTO_TEST_CASE(settings__peered__ipv6_host__expected)
{
    settings instance{};
    instance.peers.clear();
    BOOST_REQUIRE(!instance.peered(config::address{ "[2020:db8::3]" }));

    instance.peers.emplace_back("[2020:db8::1]");
    instance.peers.emplace_back("[2020:db8::2]");
    BOOST_REQUIRE(!instance.peered(config::address{ "[2020:db8::3]" }));

    instance.peers.emplace_back("[2020:db8::3]");
    BOOST_REQUIRE(!instance.peered(config::address{ "[2020:db8::3]" }));

    instance.initialize();
    BOOST_REQUIRE(instance.peered(config::address{ "[2020:db8::3]" }));
}

// excluded

BOOST_AUTO_TEST_CASE(settings__excluded__default__true)
{
    settings instance{};
    instance.initialize();
    BOOST_REQUIRE(instance.excluded({}));
}

BOOST_AUTO_TEST_SUITE_END()
