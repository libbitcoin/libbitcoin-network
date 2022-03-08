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
#include "test.hpp"
#include <cstdio>
#include <future>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <bitcoin/network.hpp>

using namespace bc::network;
using namespace bc::system::chain;
using namespace bc::network::messages;

struct p2p_tests_setup_fixture
{
    p2p_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }

    ~p2p_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }
};

BOOST_FIXTURE_TEST_SUITE(p2p_tests, p2p_tests_setup_fixture)

BOOST_AUTO_TEST_CASE(p2p__network_settings__unstarted__expected)
{
    settings set(selection::mainnet);
    BOOST_REQUIRE_EQUAL(set.threads, 1u);

    p2p net(set);
    BOOST_REQUIRE_EQUAL(net.network_settings().threads, 1u);
}

BOOST_AUTO_TEST_CASE(p2p__address_count__unstarted__zero)
{
    const settings set(selection::mainnet);
    p2p net(set);
    BOOST_REQUIRE_EQUAL(net.address_count(), 0u);
}

BOOST_AUTO_TEST_CASE(p2p__channel_count__unstarted__zero)
{
    const settings set(selection::mainnet);
    p2p net(set);
    BOOST_REQUIRE_EQUAL(net.channel_count(), 0u);
}

BOOST_AUTO_TEST_CASE(p2p__connect__unstarted__service_stopped)
{
    const settings set(selection::mainnet);
    p2p net(set);

    std::promise<bool> promise;
    const auto handler = [&](const code& ec, channel::ptr channel)
    {
        BOOST_REQUIRE(!channel);
        BOOST_REQUIRE_EQUAL(ec, error::service_stopped);
        promise.set_value(true);
    };

    net.connect({ "truckers.ca" });
    net.connect("truckers.ca", 42);
    net.connect("truckers.ca", 42, handler);
    BOOST_REQUIRE(promise.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__subscribe_connect__unstarted__success)
{
    const settings set(selection::mainnet);
    p2p net(set);

    std::promise<bool> promise_handler;
    const auto handler = [&](const code& ec, channel::ptr channel)
    {
        BOOST_REQUIRE(!channel);
        BOOST_REQUIRE_EQUAL(ec, error::service_stopped);
        promise_handler.set_value(true);
    };

    std::promise<bool> promise_complete;
    const auto complete = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        promise_complete.set_value(true);
    };

    net.subscribe_connect(handler, complete);
    BOOST_REQUIRE(promise_complete.get_future().get());

    // Close (or ~p2p) required to clear subscription.
    net.close();
    BOOST_REQUIRE(promise_handler.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__subscribe_close__unstarted__service_stopped)
{
    const settings set(selection::mainnet);
    p2p net(set);

    std::promise<bool> promise_handler;
    const auto handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::service_stopped);
        promise_handler.set_value(true);
    };

    std::promise<bool> promise_complete;
    const auto complete = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        promise_complete.set_value(true);
    };

    net.subscribe_close(handler, complete);
    BOOST_REQUIRE(promise_complete.get_future().get());

    // Close (or ~p2p) required to clear subscription.
    net.close();
    BOOST_REQUIRE(promise_handler.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__start__no__peers_no_seeds__success)
{
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.peers.empty());
    set.seeds.clear();

    p2p net(set);
    BOOST_REQUIRE(net.network_settings().peers.empty());
    BOOST_REQUIRE(net.network_settings().seeds.empty());

    std::promise<bool> promise;
    const auto handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        promise.set_value(true);
    };

    net.start(handler);
    BOOST_REQUIRE(promise.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__run__not_started__service_stopped)
{
    settings set(selection::mainnet);
    p2p net(set);

    std::promise<bool> promise;
    const auto handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::service_stopped);
        promise.set_value(true);
    };

    net.run(handler);
    BOOST_REQUIRE(promise.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__run__started_no_peers_no_seeds__success)
{
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.peers.empty());
    set.seeds.clear();

    p2p net(set);
    BOOST_REQUIRE(net.network_settings().peers.empty());
    BOOST_REQUIRE(net.network_settings().seeds.empty());

    std::promise<bool> promise_run;
    const auto run_handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        promise_run.set_value(true);
    };

    const auto start_handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        net.run(run_handler);
    };

    net.start(start_handler);
    BOOST_REQUIRE(promise_run.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__run__started_no_peers_no_seeds_one_connection_no_batch__success)
{
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.peers.empty());

    // This implies seeding would be required.
    set.host_pool_capacity = 1;

    // There are no seeds, so seeding would fail.
    set.seeds.clear();

    // Cache one address to preclude seeding.
    set.hosts_file = TEST_NAME;
    system::ofstream file(set.hosts_file);
    file << config::authority{ "1.2.3.4:42" } << std::endl;

    // Configure one connection with no batching.
    set.connect_batch_size = 1;
    set.outbound_connections = 1;

    p2p net(set);

    std::promise<bool> promise_run;
    const auto run_handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        promise_run.set_value(true);
    };

    const auto start_handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        net.run(run_handler);
    };

    net.start(start_handler);
    BOOST_REQUIRE(promise_run.get_future().get());
}

BOOST_AUTO_TEST_SUITE_END()
