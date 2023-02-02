/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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
#include "../test.hpp"

struct hosts_tests_setup_fixture
{
    hosts_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }

    ~hosts_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }
};

BOOST_FIXTURE_TEST_SUITE(hosts_tests, hosts_tests_setup_fixture)

using namespace messages;

// start

BOOST_AUTO_TEST_CASE(hosts__start__disabled__success)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(set.host_pool_capacity, 0u);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
}

BOOST_AUTO_TEST_CASE(hosts__start__enabled__success)
{
    // Non-empty pool causes file open/load.
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);

    instance.stop();
}

BOOST_AUTO_TEST_CASE(hosts__start__disabled_start__success)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
}

BOOST_AUTO_TEST_CASE(hosts__start__enabled_started__operation_failed)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.start(), error::operation_failed);

    instance.stop();
}

// stop

BOOST_AUTO_TEST_CASE(hosts__stop__disabled__success)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.stop(), error::success);
}

BOOST_AUTO_TEST_CASE(hosts__stop__enabled_stopped__success)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.host_pool_capacity = 42;
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.stop(), error::success);
}

// count

BOOST_AUTO_TEST_CASE(hosts__count__empty__zero)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    const hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
}

// store1

const address_item null_host{ 0, 0, null_ip_address, 0 };
const address_item host42{ 0, 0, unspecified_ip_address, 42 };

BOOST_AUTO_TEST_CASE(hosts__store1__disabled_stopped__empty)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    hosts instance(log, set);
    instance.store(null_host);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
}

BOOST_AUTO_TEST_CASE(hosts__store1__stopped__empty)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.host_pool_capacity = 42;
    hosts instance(log, set);
    instance.store(null_host);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
}

BOOST_AUTO_TEST_CASE(hosts__store1__invalid__empty)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    instance.store(null_host);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    instance.stop();
}

BOOST_AUTO_TEST_CASE(hosts__store1__valid__one)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    instance.store(host42);
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);

    instance.stop();
}

// store2

const address_item host1{ 0, 0, unspecified_ip_address, 1 };
const address_item host2{ 0, 0, unspecified_ip_address, 2 };
const address_item host3{ 0, 0, unspecified_ip_address, 3 };

BOOST_AUTO_TEST_CASE(hosts__store2__three_unique__three)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    instance.store({ host1, host2, host3 });
    BOOST_REQUIRE_EQUAL(instance.count(), 3u);

    instance.stop();
}

BOOST_AUTO_TEST_CASE(hosts__store2__redundant__expected)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    instance.store({ host1, host2, host3, host3, host2, host1 });
    BOOST_REQUIRE_EQUAL(instance.count(), 3u);

    instance.stop();
}

// remove

BOOST_AUTO_TEST_CASE(hosts__remove__only__empty)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    instance.store(host42);
    instance.remove(host42);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    instance.stop();
}

BOOST_AUTO_TEST_CASE(hosts__remove__single_not_found__one)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    instance.store(host1);
    instance.remove(host2);
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);

    instance.stop();
}

// fetch1

BOOST_AUTO_TEST_CASE(hosts__fetch1__stopped__service_stopped)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(log, set);

    instance.store(host42);
    instance.fetch([&](const code& ec, const messages::address_item&)
    {
        BOOST_REQUIRE_EQUAL(ec, error::service_stopped);
    });
}

BOOST_AUTO_TEST_CASE(hosts__fetch1__empty__address_not_found)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    instance.fetch([&](const code& ec, const messages::address_item&)
    {
        BOOST_REQUIRE_EQUAL(ec, error::address_not_found);
    });

    instance.stop();
}

BOOST_AUTO_TEST_CASE(hosts__fetch1__only__expected)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    instance.store(host42);
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);
    
    instance.fetch([&](const code& ec, const messages::address_item& item)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);

        // Message types do not have comparison operators.
        BOOST_REQUIRE_EQUAL(item.ip, host42.ip);
        BOOST_REQUIRE_EQUAL(item.port, host42.port);
        BOOST_REQUIRE_EQUAL(item.services, host42.services);
        BOOST_REQUIRE_EQUAL(item.timestamp, host42.timestamp);
    });

    instance.stop();
}

// fetch2

BOOST_AUTO_TEST_CASE(hosts__fetch2__stopped__service_stopped)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_PATH;
    set.host_pool_capacity = 42;
    hosts instance(log, set);

    instance.store(host42);
    instance.fetch([&](const code& ec, const messages::address_items& items)
    {
        BOOST_REQUIRE_EQUAL(ec, error::service_stopped);
        BOOST_REQUIRE(items.empty());
    });
}

BOOST_AUTO_TEST_CASE(hosts__fetch2__empty__address_not_found)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    instance.fetch([&](const code& ec, const messages::address_items& items)
    {
        BOOST_REQUIRE_EQUAL(ec, error::address_not_found);
        BOOST_REQUIRE(items.empty());
    });

    instance.stop();
}

BOOST_AUTO_TEST_CASE(hosts__fetch2__three__success_empty)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(log, set);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    instance.store({ host1, host2, host3 });
    BOOST_REQUIRE_EQUAL(instance.count(), 3u);

    instance.fetch([&](const code& ec, const messages::address_items& items)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        BOOST_REQUIRE(items.empty());
    });

    instance.stop();
}

BOOST_AUTO_TEST_CASE(hosts__fetch2__populated_file__expected)
{
    const logger log{};
    settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance1(log, set);
    BOOST_REQUIRE_EQUAL(instance1.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance1.count(), 0u);

    instance1.store({ host1, host2, host3 });
    BOOST_REQUIRE_EQUAL(instance1.count(), 3u);

    // File is created with three entries.
    instance1.stop();

    // Start with existing file and read entries.
    hosts instance2(log, set);
    BOOST_REQUIRE_EQUAL(instance2.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance2.count(), 3u);

    instance2.stop();
}

BOOST_AUTO_TEST_SUITE_END()
