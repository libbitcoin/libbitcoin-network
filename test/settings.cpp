/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network.hpp>

BOOST_AUTO_TEST_SUITE(settings_tests)

using namespace bc::system::chain;
using namespace messages;

BOOST_AUTO_TEST_CASE(settings__construct__default__expected)
{
    settings instance;

    // [network]
    BOOST_REQUIRE_EQUAL(instance.threads, 1u);
    BOOST_REQUIRE_EQUAL(instance.protocol_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(instance.protocol_maximum, level::maximum_protocol);
    BOOST_REQUIRE_EQUAL(instance.services_minimum, service::minimum_services);
    BOOST_REQUIRE_EQUAL(instance.services_maximum, service::maximum_services);
    BOOST_REQUIRE_EQUAL(instance.invalid_services, 176u);
    BOOST_REQUIRE_EQUAL(instance.enable_reject, false);
    BOOST_REQUIRE_EQUAL(instance.relay_transactions, false);
    BOOST_REQUIRE_EQUAL(instance.validate_checksum, false);
    BOOST_REQUIRE_EQUAL(instance.identifier, 0u);
    BOOST_REQUIRE_EQUAL(instance.inbound_port, 0u);
    BOOST_REQUIRE_EQUAL(instance.inbound_connections, 0u);
    BOOST_REQUIRE_EQUAL(instance.outbound_connections, 8u);
    BOOST_REQUIRE_EQUAL(instance.connect_batch_size, 5u);
    BOOST_REQUIRE_EQUAL(instance.connect_timeout_seconds, 5u);
    BOOST_REQUIRE_EQUAL(instance.channel_handshake_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.channel_germination_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.channel_heartbeat_minutes, 5u);
    BOOST_REQUIRE_EQUAL(instance.channel_inactivity_minutes, 10u);
    BOOST_REQUIRE_EQUAL(instance.channel_expiration_minutes, 1440u);
    BOOST_REQUIRE_EQUAL(instance.host_pool_capacity, 0u);
    BOOST_REQUIRE_EQUAL(instance.hosts_file, "hosts.cache");
    BOOST_REQUIRE(instance.self == unspecified_address_item);
    BOOST_REQUIRE(instance.blacklists.empty());
    BOOST_REQUIRE(instance.peers.empty());
    BOOST_REQUIRE(instance.seeds.empty());
    BOOST_REQUIRE_EQUAL(instance.verbose, false);
}

BOOST_AUTO_TEST_CASE(settings__construct__mainnet__expected)
{
    settings instance(selection::mainnet);

    // unchanged from default
    BOOST_REQUIRE_EQUAL(instance.threads, 1u);
    BOOST_REQUIRE_EQUAL(instance.protocol_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(instance.protocol_maximum, level::maximum_protocol);
    BOOST_REQUIRE_EQUAL(instance.services_minimum, service::minimum_services);
    BOOST_REQUIRE_EQUAL(instance.services_maximum, service::maximum_services);
    BOOST_REQUIRE_EQUAL(instance.invalid_services, 176u);
    BOOST_REQUIRE_EQUAL(instance.enable_reject, false);
    BOOST_REQUIRE_EQUAL(instance.relay_transactions, false);
    BOOST_REQUIRE_EQUAL(instance.validate_checksum, false);
    BOOST_REQUIRE_EQUAL(instance.inbound_connections, 0u);
    BOOST_REQUIRE_EQUAL(instance.outbound_connections, 8u);
    BOOST_REQUIRE_EQUAL(instance.connect_batch_size, 5u);
    BOOST_REQUIRE_EQUAL(instance.connect_timeout_seconds, 5u);
    BOOST_REQUIRE_EQUAL(instance.channel_handshake_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.channel_germination_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.channel_heartbeat_minutes, 5u);
    BOOST_REQUIRE_EQUAL(instance.channel_inactivity_minutes, 10u);
    BOOST_REQUIRE_EQUAL(instance.channel_expiration_minutes, 1440u);
    BOOST_REQUIRE_EQUAL(instance.host_pool_capacity, 0u);
    BOOST_REQUIRE_EQUAL(instance.hosts_file, "hosts.cache");
    BOOST_REQUIRE(instance.self == unspecified_address_item);
    BOOST_REQUIRE(instance.blacklists.empty());
    BOOST_REQUIRE(instance.peers.empty());
    BOOST_REQUIRE_EQUAL(instance.verbose, false);

    // changed from default
    BOOST_REQUIRE_EQUAL(instance.identifier, 3652501241u);
    BOOST_REQUIRE_EQUAL(instance.inbound_port, 8333u);
    BOOST_REQUIRE_EQUAL(instance.seeds.size(), 4u);

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
    BOOST_REQUIRE_EQUAL(instance.protocol_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(instance.protocol_maximum, level::maximum_protocol);
    BOOST_REQUIRE_EQUAL(instance.services_minimum, service::minimum_services);
    BOOST_REQUIRE_EQUAL(instance.services_maximum, service::maximum_services);
    BOOST_REQUIRE_EQUAL(instance.invalid_services, 176u);
    BOOST_REQUIRE_EQUAL(instance.enable_reject, false);
    BOOST_REQUIRE_EQUAL(instance.relay_transactions, false);
    BOOST_REQUIRE_EQUAL(instance.validate_checksum, false);
    BOOST_REQUIRE_EQUAL(instance.inbound_connections, 0u);
    BOOST_REQUIRE_EQUAL(instance.outbound_connections, 8u);
    BOOST_REQUIRE_EQUAL(instance.connect_batch_size, 5u);
    BOOST_REQUIRE_EQUAL(instance.connect_timeout_seconds, 5u);
    BOOST_REQUIRE_EQUAL(instance.channel_handshake_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.channel_germination_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.channel_heartbeat_minutes, 5u);
    BOOST_REQUIRE_EQUAL(instance.channel_inactivity_minutes, 10u);
    BOOST_REQUIRE_EQUAL(instance.channel_expiration_minutes, 1440u);
    BOOST_REQUIRE_EQUAL(instance.host_pool_capacity, 0u);
    BOOST_REQUIRE_EQUAL(instance.hosts_file, "hosts.cache");
    BOOST_REQUIRE(instance.self == unspecified_address_item);
    BOOST_REQUIRE(instance.blacklists.empty());
    BOOST_REQUIRE(instance.peers.empty());
    BOOST_REQUIRE_EQUAL(instance.verbose, false);

    // changed from default
    BOOST_REQUIRE_EQUAL(instance.identifier, 118034699u);
    BOOST_REQUIRE_EQUAL(instance.inbound_port, 18333u);
    BOOST_REQUIRE_EQUAL(instance.seeds.size(), 4u);

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
    BOOST_REQUIRE_EQUAL(instance.protocol_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(instance.protocol_maximum, level::maximum_protocol);
    BOOST_REQUIRE_EQUAL(instance.services_minimum, service::minimum_services);
    BOOST_REQUIRE_EQUAL(instance.services_maximum, service::maximum_services);
    BOOST_REQUIRE_EQUAL(instance.invalid_services, 176u);
    BOOST_REQUIRE_EQUAL(instance.enable_reject, false);
    BOOST_REQUIRE_EQUAL(instance.relay_transactions, false);
    BOOST_REQUIRE_EQUAL(instance.validate_checksum, false);
    BOOST_REQUIRE_EQUAL(instance.inbound_connections, 0u);
    BOOST_REQUIRE_EQUAL(instance.outbound_connections, 8u);
    BOOST_REQUIRE_EQUAL(instance.connect_batch_size, 5u);
    BOOST_REQUIRE_EQUAL(instance.connect_timeout_seconds, 5u);
    BOOST_REQUIRE_EQUAL(instance.channel_handshake_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.channel_germination_seconds, 30u);
    BOOST_REQUIRE_EQUAL(instance.channel_heartbeat_minutes, 5u);
    BOOST_REQUIRE_EQUAL(instance.channel_inactivity_minutes, 10u);
    BOOST_REQUIRE_EQUAL(instance.channel_expiration_minutes, 1440u);
    BOOST_REQUIRE_EQUAL(instance.host_pool_capacity, 0u);
    BOOST_REQUIRE_EQUAL(instance.hosts_file, "hosts.cache");
    BOOST_REQUIRE(instance.self == unspecified_address_item);
    BOOST_REQUIRE(instance.blacklists.empty());
    BOOST_REQUIRE(instance.peers.empty());
    BOOST_REQUIRE(instance.seeds.empty());
    BOOST_REQUIRE_EQUAL(instance.verbose, false);

    // changed from default
    BOOST_REQUIRE_EQUAL(instance.identifier, 3669344250u);
    BOOST_REQUIRE_EQUAL(instance.inbound_port, 18444u);
}

BOOST_AUTO_TEST_CASE(settings__connect_timeout__always__connect_timeout_seconds)
{
    settings instance;
    const auto expected = 42;
    instance.connect_timeout_seconds = expected;
    BOOST_REQUIRE(instance.connect_timeout() == seconds(expected));
}

BOOST_AUTO_TEST_CASE(settings__channel_handshake__always__channel_handshake_seconds)
{
    settings instance;
    const auto expected = 42u;
    instance.channel_handshake_seconds = expected;
    BOOST_REQUIRE(instance.channel_handshake() == seconds(expected));
}

BOOST_AUTO_TEST_CASE(settings__channel_heartbeat__always__channel_heartbeat_minutes)
{
    settings instance;
    const auto expected = 42u;
    instance.channel_heartbeat_minutes = expected;
    BOOST_REQUIRE(instance.channel_heartbeat() == minutes(expected));
}

BOOST_AUTO_TEST_CASE(settings__channel_inactivity__always__channel_inactivity_minutes)
{
    settings instance;
    const auto expected = 42u;
    instance.channel_inactivity_minutes = expected;
    BOOST_REQUIRE(instance.channel_inactivity() == minutes(expected));
}

BOOST_AUTO_TEST_CASE(settings__channel_expiration__always__channel_expiration_minutes)
{
    settings instance;
    const auto expected = 42u;
    instance.channel_expiration_minutes = expected;
    BOOST_REQUIRE(instance.channel_expiration() == minutes(expected));
}

BOOST_AUTO_TEST_CASE(settings__channel_germination__always__channel_germination_seconds)
{
    settings instance;
    const auto expected = 42u;
    instance.channel_germination_seconds = expected;
    BOOST_REQUIRE(instance.channel_germination() == seconds(expected));
}

BOOST_AUTO_TEST_SUITE_END()
