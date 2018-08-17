/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#include <cstdio>
#include <future>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>
#include <bitcoin/network.hpp>

using namespace bc;
using namespace bc::message;
using namespace bc::network;

#define TEST_SET_NAME \
    "p2p_tests"

#define TEST_NAME \
    boost::unit_test::framework::current_test_case().p_name

// TODO: build mock and/or use dedicated test service.
#define SEED1 "testnet1.libbitcoin.net:18333"
#define SEED2 "testnet2.libbitcoin.net:18333"

// NOTE: this is insufficient as the address varies.
#define SEED_AUTHORITIES \
{ \
    { "72.65.246.83" }, \
    { "72.65.246.84" }, \
}

#define SETTINGS_TESTNET_ONE_THREAD_NO_CONNECTIONS(name) \
    auto name = network::settings(bc::config::settings::testnet); \
    name.threads = 1; \
    name.outbound_connections = 0; \
    name.manual_attempt_limit = 2

#define SETTINGS_TESTNET_ONE_THREAD_ONE_SEED(name) \
    SETTINGS_TESTNET_ONE_THREAD_NO_CONNECTIONS(name); \
    name.host_pool_capacity = 42; \
    name.seeds = { { SEED1 } }; \
    name.hosts_file = get_log_path(TEST_NAME, "hosts")

#define SETTINGS_TESTNET_THREE_THREADS_ONE_SEED_FIVE_OUTBOUND(name) \
    auto name = network::settings(bc::config::settings::testnet); \
    name.threads = 3; \
    name.host_pool_capacity = 42; \
    name.outbound_connections = 5; \
    name.seeds = { { SEED1 } }; \
    name.hosts_file = get_log_path(TEST_NAME, "hosts")

std::string get_log_path(const std::string& test, const std::string& file)
{
    const auto path = test + "." + file + ".log";
    boost::filesystem::remove_all(path);
    return path;
}

static void print_headers(const std::string& test)
{
    const auto header = "=========== " + test + " ==========";
    LOG_INFO(TEST_SET_NAME) << header;
}

static int start_result(p2p& network)
{
    std::promise<code> promise;
    const auto handler = [&promise](code ec)
    {
        promise.set_value(ec);
    };
    network.start(handler);
    return promise.get_future().get().value();
}

static int connect_result(p2p& network, const config::endpoint& host)
{
    std::promise<code> promise;
    const auto handler = [&promise](code ec, channel::ptr)
    {
        promise.set_value(ec);
    };
    network.connect(host.host(), host.port(), handler);
    return promise.get_future().get().value();
}

static int run_result(p2p& network)
{
    std::promise<code> promise;
    const auto handler = [&promise](code ec)
    {
        promise.set_value(ec);
    };
    network.run(handler);
    return promise.get_future().get().value();
}

static int subscribe_result(p2p& network)
{
    std::promise<code> promise;
    const auto handler = [&promise](code ec, channel::ptr)
    {
        promise.set_value(ec);
        return false;
    };
    network.subscribe_connection(handler);
    return promise.get_future().get().value();
}

static int subscribe_connect1_result(p2p& network, const config::endpoint& host)
{
    std::promise<code> promise;
    const auto handler = [&promise](code ec, channel::ptr)
    {
        promise.set_value(ec);
        return false;
    };
    network.subscribe_connection(handler);
    network.connect(host.host(), host.port());
    return promise.get_future().get().value();
}

static int subscribe_connect2_result(p2p& network, const config::endpoint& host)
{
    std::promise<code> promise;
    const auto handler = [&promise](code ec, channel::ptr)
    {
        promise.set_value(ec);
        return false;
    };
    network.subscribe_connection(handler);
    network.connect(host);
    return promise.get_future().get().value();
}

template<class Message>
static int send_result(const Message& message, p2p& network, int channels)
{
    const auto channel_counter = [&channels](code ec, channel::ptr )
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        --channels;
    };

    std::promise<code> promise;
    const auto completion_handler = [&promise](code ec)
    {
        promise.set_value(ec);
    };

    network.broadcast(message, channel_counter, completion_handler);
    const auto result = promise.get_future().get().value();

    BOOST_REQUIRE_EQUAL(channels, 0);
    return result;
}

// Trivial tests just validate static inits (required because p2p tests disabled in travis).
BOOST_AUTO_TEST_SUITE(empty_tests)

BOOST_AUTO_TEST_CASE(empty_test)
{
    BOOST_REQUIRE(true);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(p2p_tests)

BOOST_AUTO_TEST_CASE(p2p__top_block__default__zero_null_hash)
{
    print_headers(TEST_NAME);
    const network::settings configuration;
    p2p network(configuration);
    BOOST_REQUIRE_EQUAL(network.top_block().height(), 0);
    BOOST_REQUIRE(network.top_block().hash() == null_hash);
}

BOOST_AUTO_TEST_CASE(p2p__set_top_block1__values__expected)
{
    print_headers(TEST_NAME);
    const network::settings configuration;
    p2p network(configuration);
    const size_t expected_height = 42;
    const auto expected_hash = hash_literal("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b");
    network.set_top_block({ expected_hash, expected_height });
    BOOST_REQUIRE(network.top_block().hash() == expected_hash);
    BOOST_REQUIRE_EQUAL(network.top_block().height(), expected_height);
}

BOOST_AUTO_TEST_CASE(p2p__set_top_block2__values__expected)
{
    print_headers(TEST_NAME);
    const network::settings configuration;
    p2p network(configuration);
    const size_t expected_height = 42;
    const auto hash = hash_literal("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b");
    const config::checkpoint expected{ hash, expected_height };
    network.set_top_block(expected);
    BOOST_REQUIRE(network.top_block().hash() == expected.hash());
    BOOST_REQUIRE_EQUAL(network.top_block().height(), expected.height());
}

BOOST_AUTO_TEST_CASE(p2p__top_header__default__zero_null_hash)
{
    print_headers(TEST_NAME);
    const network::settings configuration;
    p2p network(configuration);
    BOOST_REQUIRE_EQUAL(network.top_header().height(), 0);
    BOOST_REQUIRE(network.top_header().hash() == null_hash);
}

BOOST_AUTO_TEST_CASE(p2p__set_top_header1__values__expected)
{
    print_headers(TEST_NAME);
    const network::settings configuration;
    p2p network(configuration);
    const size_t expected_height = 42;
    const auto expected_hash = hash_literal("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b");
    network.set_top_header({ expected_hash, expected_height });
    BOOST_REQUIRE(network.top_header().hash() == expected_hash);
    BOOST_REQUIRE_EQUAL(network.top_header().height(), expected_height);
}

BOOST_AUTO_TEST_CASE(p2p__set_top_header2__values__expected)
{
    print_headers(TEST_NAME);
    const network::settings configuration;
    p2p network(configuration);
    const size_t expected_height = 42;
    const auto hash = hash_literal("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b");
    const config::checkpoint expected{ hash, expected_height };
    network.set_top_header(expected);
    BOOST_REQUIRE(network.top_header().hash() == expected.hash());
    BOOST_REQUIRE_EQUAL(network.top_header().height(), expected.height());
}

BOOST_AUTO_TEST_CASE(p2p__start__no_sessions__start_success)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_NO_CONNECTIONS(configuration);
    p2p network(configuration);
    BOOST_REQUIRE_EQUAL(start_result(network), error::success);
}

BOOST_AUTO_TEST_CASE(p2p__start__no_connections__start_stop_success)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_NO_CONNECTIONS(configuration);
    p2p network(configuration);
    BOOST_REQUIRE_EQUAL(start_result(network), error::success);
    BOOST_REQUIRE(network.stop());
}

BOOST_AUTO_TEST_CASE(p2p__start__no_sessions__start_success_start_operation_fail)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_NO_CONNECTIONS(configuration);
    p2p network(configuration);
    BOOST_REQUIRE_EQUAL(start_result(network), error::success);
    BOOST_REQUIRE_EQUAL(start_result(network), error::operation_failed);
}

////BOOST_AUTO_TEST_CASE(p2p__start__seed_session__start_stop_start_success)
////{
////    print_headers(TEST_NAME);
////    SETTINGS_TESTNET_ONE_THREAD_ONE_SEED(configuration);
////    p2p network(configuration);
////    BOOST_REQUIRE_EQUAL(start_result(network), error::success);
////    BOOST_REQUIRE(network.stop());
////    BOOST_REQUIRE_EQUAL(start_result(network), error::success);
////}

BOOST_AUTO_TEST_CASE(p2p__start__seed_session_handshake_timeout__start_peer_throttling_stop_success)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_ONE_SEED(configuration);
    configuration.channel_handshake_seconds = 0;
    p2p network(configuration);

    // The (timeout) error on the individual connection is ignored.
    // The connection failure results in a failure to generate any addresses.
    // The failure to generate an increase of 100 addresses produces error::peer_throttling.
    BOOST_REQUIRE_EQUAL(start_result(network), error::peer_throttling);

    // The service never started but stop will still succeed (and is optional).
    BOOST_REQUIRE(network.stop());
}

BOOST_AUTO_TEST_CASE(p2p__start__seed_session_connect_timeout__start_peer_throttling_stop_success)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_ONE_SEED(configuration);
    configuration.connect_timeout_seconds = 0;
    p2p network(configuration);
    BOOST_REQUIRE_EQUAL(start_result(network), error::peer_throttling);
    BOOST_REQUIRE(network.stop());
}

BOOST_AUTO_TEST_CASE(p2p__start__seed_session_germination_timeout__start_peer_throttling_stop_success)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_ONE_SEED(configuration);
    configuration.channel_germination_seconds = 0;
    p2p network(configuration);
    BOOST_REQUIRE_EQUAL(start_result(network), error::peer_throttling);
    BOOST_REQUIRE(network.stop());
}

BOOST_AUTO_TEST_CASE(p2p__start__seed_session_inactivity_timeout__start_peer_throttling_stop_success)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_ONE_SEED(configuration);
    configuration.channel_inactivity_minutes = 0;
    p2p network(configuration);
    BOOST_REQUIRE_EQUAL(start_result(network), error::peer_throttling);
    BOOST_REQUIRE(network.stop());
}

BOOST_AUTO_TEST_CASE(p2p__start__seed_session_expiration_timeout__start_peer_throttling_stop_success)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_ONE_SEED(configuration);
    configuration.channel_expiration_minutes = 0;
    p2p network(configuration);
    BOOST_REQUIRE_EQUAL(start_result(network), error::peer_throttling);
    BOOST_REQUIRE(network.stop());
}

// This may fail due to missing blacklist entries for the specified seed hosts.
BOOST_AUTO_TEST_CASE(p2p__start__seed_session_blacklisted__start_peer_throttling_stop_success)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_NO_CONNECTIONS(configuration);
    configuration.host_pool_capacity = 42;
    configuration.hosts_file = get_log_path(TEST_NAME, "hosts");
    configuration.seeds = { { SEED1 }, { SEED2 } };
    configuration.blacklists = SEED_AUTHORITIES;
    p2p network(configuration);
    BOOST_REQUIRE_EQUAL(start_result(network), error::peer_throttling);
    BOOST_REQUIRE(network.stop());
}

BOOST_AUTO_TEST_CASE(p2p__start__outbound_no_seeds__success)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_NO_CONNECTIONS(configuration);
    configuration.outbound_connections = 1;
    p2p network(configuration);
    BOOST_REQUIRE_EQUAL(start_result(network), error::success);
}

BOOST_AUTO_TEST_CASE(p2p__connect__not_started__service_stopped)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_NO_CONNECTIONS(configuration);
    p2p network(configuration);
    const config::endpoint host(SEED1);
    BOOST_REQUIRE_EQUAL(connect_result(network, host), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(p2p__connect__started__success)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_NO_CONNECTIONS(configuration);
    p2p network(configuration);
    const config::endpoint host(SEED1);
    BOOST_REQUIRE_EQUAL(start_result(network), error::success);
    BOOST_REQUIRE_EQUAL(run_result(network), error::success);
    BOOST_REQUIRE_EQUAL(connect_result(network, host), error::success);
}

// Disabled for live test reliability.
// This may fail due to connecting to the same host on different addresses.
////BOOST_AUTO_TEST_CASE(p2p__connect__twice__address_in_use)
////{
////    print_headers(TEST_NAME);
////    SETTINGS_TESTNET_ONE_THREAD_NO_CONNECTIONS(configuration);
////    p2p network(configuration);
////    const config::endpoint host(SEED1);
////    BOOST_REQUIRE_EQUAL(start_result(network), error::success);
////    BOOST_REQUIRE_EQUAL(run_result(network), error::success);
////    BOOST_REQUIRE_EQUAL(connect_result(network, host), error::success);
////    BOOST_REQUIRE_EQUAL(connect_result(network, host), error::address_in_use);
////}

BOOST_AUTO_TEST_CASE(p2p__subscribe__stopped__service_stopped)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_NO_CONNECTIONS(configuration);
    p2p network(configuration);

    // Expect immediate return because service is not started.
    BOOST_REQUIRE_EQUAL(subscribe_result(network), error::service_stopped);
}

BOOST_AUTO_TEST_CASE(p2p__subscribe__started_stop__service_stopped)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_NO_CONNECTIONS(configuration);
    p2p network(configuration);
    BOOST_REQUIRE_EQUAL(start_result(network), error::success);

    std::promise<code> promise;
    const auto handler = [](code ec, channel::ptr channel)
    {
        BOOST_REQUIRE(!channel);
        BOOST_REQUIRE_EQUAL(ec, error::service_stopped);
        return false;
    };

    // Expect queued handler until destruct because service is started.
    network.subscribe_connection(handler);
}

BOOST_AUTO_TEST_CASE(p2p__subscribe__started_connect1__success)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_NO_CONNECTIONS(configuration);
    p2p network(configuration);
    const config::endpoint host(SEED1);
    BOOST_REQUIRE_EQUAL(start_result(network), error::success);
    BOOST_REQUIRE_EQUAL(subscribe_connect1_result(network, host), error::success);
}

BOOST_AUTO_TEST_CASE(p2p__subscribe__started_connect2__success)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_NO_CONNECTIONS(configuration);
    p2p network(configuration);
    const config::endpoint host(SEED1);
    BOOST_REQUIRE_EQUAL(start_result(network), error::success);
    BOOST_REQUIRE_EQUAL(subscribe_connect2_result(network, host), error::success);
}

BOOST_AUTO_TEST_CASE(p2p__broadcast__ping_two_distinct_hosts__two_sends_and_successful_completion)
{
    print_headers(TEST_NAME);
    SETTINGS_TESTNET_ONE_THREAD_NO_CONNECTIONS(configuration);
    p2p network(configuration);
    const config::endpoint host1(SEED1);
    const config::endpoint host2(SEED2);
    BOOST_REQUIRE_EQUAL(start_result(network), error::success);
    BOOST_REQUIRE_EQUAL(run_result(network), error::success);
    BOOST_REQUIRE_EQUAL(connect_result(network, host1), error::success);
    BOOST_REQUIRE_EQUAL(connect_result(network, host2), error::success);
    BOOST_REQUIRE_EQUAL(send_result(ping(0), network, 2), error::success);
}

////BOOST_AUTO_TEST_CASE(p2p__subscribe__seed_outbound__success)
////{
////    print_headers(TEST_NAME);
////    SETTINGS_TESTNET_THREE_THREADS_ONE_SEED_FIVE_OUTBOUND(configuration);
////    p2p network(configuration);
////    BOOST_REQUIRE_EQUAL(start_result(network), error::success);
////
////    std::promise<code> subscribe;
////    const auto subscribe_handler = [&subscribe, &network](code ec, channel::ptr)
////    {
////        // Fires on first connection.
////        subscribe.set_value(ec);
////        return false;
////    };
////    network.subscribe_connection(subscribe_handler);
////
////    std::promise<code> run;
////    const auto run_handler = [&run, &network](code ec)
////    {
////        // Fires once the session is started.
////        run.set_value(ec);
////    };
////    network.run(run_handler);
////
////    BOOST_REQUIRE_EQUAL(run.get_future().get().value(), error::success);
////    BOOST_REQUIRE_EQUAL(subscribe.get_future().get().value(), error::success);
////
////    // ~network blocks on stopping all channels.
////    // during channel.stop each channel removes itself from the collection.
////}

BOOST_AUTO_TEST_SUITE_END()
