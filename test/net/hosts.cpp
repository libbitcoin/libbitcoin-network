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

class mock_settings final
  : public settings
{
public:
    using settings::settings;

    // Override derivative name, using directory as file.
    std::filesystem::path file() const NOEXCEPT override
    {
        return path;
    }
};

constexpr address_item host1{ 0, 0, loopback_ip_address, 1 };
constexpr address_item host2{ 0, 0, loopback_ip_address, 2 };
constexpr address_item host3{ 0, 0, loopback_ip_address, 3 };
constexpr address_item loopback00{ 0, 0, loopback_ip_address, 0 };
constexpr address_item loopback42{ 0, 0, loopback_ip_address, 42 };
constexpr address_item unspecified00{ 0, 0, unspecified_ip_address, 0 };
////constexpr address_item unspecified42{ 0, 0, unspecified_ip_address, 42 };

// start

BOOST_AUTO_TEST_CASE(hosts__start__disabled__success)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    hosts instance(set, log);
    BOOST_REQUIRE_EQUAL(set.host_pool_capacity, 0u);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
}

BOOST_AUTO_TEST_CASE(hosts__start__enabled__success)
{
    // Non-empty capacity causes file open/load.
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(set, log);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE(!test::exists(TEST_NAME));

    instance.stop();
}

BOOST_AUTO_TEST_CASE(hosts__start__disabled_start__success)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    hosts instance(set, log);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
}

BOOST_AUTO_TEST_CASE(hosts__start__enabled_started__success)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(set, log);

    // Not idempotent start.
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.start(), error::operation_failed);
    BOOST_REQUIRE(!test::exists(TEST_NAME));

    instance.stop();
}

BOOST_AUTO_TEST_CASE(hosts__start__populated_file__expected)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance1(set, log);

    // File is deleted if empty on open.
    BOOST_REQUIRE(test::create(TEST_NAME));
    BOOST_REQUIRE(test::exists(TEST_NAME));
    BOOST_REQUIRE_EQUAL(instance1.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance1.count(), 0u);
    BOOST_REQUIRE(!test::exists(TEST_NAME));

    const auto message = system::to_shared(address{ { host1, host2, host3 } });
    std::promise<size_t> promise_count{};
    instance1.save(message, [&](code, size_t accepted)
    {
        promise_count.set_value(accepted);
    });
    BOOST_REQUIRE_EQUAL(promise_count.get_future().get(), 3u);
    BOOST_REQUIRE_EQUAL(instance1.count(), 3u);

    // File is not created until stop.
    BOOST_REQUIRE(!test::exists(TEST_NAME));

    // File is created with three entries.
    instance1.stop();
    BOOST_REQUIRE(test::exists(TEST_NAME));

    // Start with existing file and read entries.
    hosts instance2(set, log);
    set.enable_ipv6 = true;
    BOOST_REQUIRE_EQUAL(instance2.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance2.count(), 3u);

    instance2.stop();
    BOOST_REQUIRE(test::exists(TEST_NAME));
}

// stop

BOOST_AUTO_TEST_CASE(hosts__stop__disabled__success)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    hosts instance(set, log);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE(!test::exists(TEST_NAME));

    // Idempotent stop.
    BOOST_REQUIRE_EQUAL(instance.stop(), error::success);
    BOOST_REQUIRE_EQUAL(instance.stop(), error::success);
}

BOOST_AUTO_TEST_CASE(hosts__stop__enabled_stopped__success)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(set, log);

    // Idempotent stop.
    BOOST_REQUIRE_EQUAL(instance.stop(), error::success);
    BOOST_REQUIRE_EQUAL(instance.stop(), error::success);
}

BOOST_AUTO_TEST_CASE(hosts__stop__enabled_started__success)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(set, log);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);

    // Idempotent stop.
    BOOST_REQUIRE_EQUAL(instance.stop(), error::success);
    BOOST_REQUIRE_EQUAL(instance.stop(), error::success);
}

// count

BOOST_AUTO_TEST_CASE(hosts__count__empty__zero)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    const hosts instance(set, log);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
}

// take

BOOST_AUTO_TEST_CASE(hosts__take__empty__address_not_found)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(set, log);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    std::promise<std::pair<code, address_item_cptr>> promise{};
    instance.take([&](const code& ec, const address_item_cptr& item) NOEXCEPT
    {
        promise.set_value({ ec, item });
    });

    instance.stop();
    const auto result = promise.get_future().get();
    BOOST_REQUIRE_EQUAL(result.first, error::address_not_found);
    BOOST_REQUIRE(!result.second);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
    BOOST_REQUIRE(!test::exists(TEST_NAME));
}

BOOST_AUTO_TEST_CASE(hosts__take__only__expected)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(set, log);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    std::promise<code> promise_restore{};
    instance.restore(system::to_shared(loopback42), [&](const code& ec) NOEXCEPT
    {
        promise_restore.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(promise_restore.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);

    std::promise<std::pair<code, address_item_cptr>> promise_take{};
    instance.take([&](const code& ec, const address_item_cptr& item) NOEXCEPT
    {
        promise_take.set_value({ ec, item });
    });

    instance.stop();
    const auto result = promise_take.get_future().get();
    BOOST_REQUIRE_EQUAL(result.first, error::success);
    BOOST_REQUIRE(*result.second == loopback42);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
    BOOST_REQUIRE(!test::exists(TEST_NAME));
}

// restore

BOOST_AUTO_TEST_CASE(hosts__restore__disabled_stopped__service_stopped_empty)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    hosts instance(set, log);
    std::promise<code> promise{};
    instance.restore(system::to_shared(loopback00), [&](const code& ec) NOEXCEPT
    {
        promise.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::service_stopped);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
}

BOOST_AUTO_TEST_CASE(hosts__restore__stopped__service_stopped_empty)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(set, log);
    std::promise<code> promise{};
    instance.restore(system::to_shared(unspecified00), [&](const code& ec) NOEXCEPT
    {
        promise.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::service_stopped);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    instance.stop();
    BOOST_REQUIRE(!test::exists(TEST_NAME));
}

BOOST_AUTO_TEST_CASE(hosts__restore__unique__accepted)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(set, log);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    std::promise<code> promise{};
    instance.restore(system::to_shared(loopback42), [&](const code& ec) NOEXCEPT
    {
        promise.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);

    instance.stop();
    BOOST_REQUIRE(test::exists(TEST_NAME));
}

BOOST_AUTO_TEST_CASE(hosts__restore__duplicate_authority__updated)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(set, log);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    constexpr address_item loopback42a{ 1, 2, loopback_ip_address, 42 };
    constexpr address_item loopback42b{ 3, 4, loopback_ip_address, 42 };

    std::promise<code> promise1{};
    instance.restore(system::to_shared(loopback42a), [&](const code& ec) NOEXCEPT
    {
        promise1.set_value(ec);
    });
    BOOST_REQUIRE_EQUAL(promise1.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);

    std::promise<code> promise2{};
    instance.restore(system::to_shared(loopback42b), [&](const code& ec) NOEXCEPT
    {
        promise2.set_value(ec);
    });
    BOOST_REQUIRE_EQUAL(promise2.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 1u);

    instance.stop();
    BOOST_REQUIRE(test::exists(TEST_NAME));
}

// fetch

BOOST_AUTO_TEST_CASE(hosts__fetch__empty__address_not_found)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(set, log);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    std::promise<std::pair<code, address::cptr>> promise_fetch{};
    instance.fetch([&](const code& ec, const address::cptr& message) NOEXCEPT
    {
        promise_fetch.set_value({ ec, message });
    });

    const auto result = promise_fetch.get_future().get();
    BOOST_REQUIRE_EQUAL(result.first, error::address_not_found);
    BOOST_REQUIRE(!result.second);

    instance.stop();
    BOOST_REQUIRE(!test::exists(TEST_NAME));
}

BOOST_AUTO_TEST_CASE(hosts__fetch__three__success_empty)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(set, log);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    const auto message = system::to_shared(address{ { host1, host2, host3 } });
    std::promise<size_t> promise_save{};
    instance.save(message, [&](code, size_t accepted) NOEXCEPT
    {
        promise_save.set_value(accepted);
    });
    BOOST_REQUIRE_EQUAL(promise_save.get_future().get(), 3u);

    std::promise<std::pair<code, address::cptr>> promise_fetch{};
    instance.fetch([&](const code& ec, const address::cptr& message) NOEXCEPT
    {
        promise_fetch.set_value({ ec, message });
    });

    const auto result = promise_fetch.get_future().get();
    BOOST_REQUIRE_EQUAL(result.first, error::success);
    BOOST_REQUIRE(result.second->addresses.empty());

    instance.stop();
    BOOST_REQUIRE(test::exists(TEST_NAME));
}

// store

BOOST_AUTO_TEST_CASE(hosts__save__three_unique__three)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(set, log);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);
    
    const auto message = system::to_shared(address{ { host1, host2, host3 } });
    std::promise<size_t> promise_count{};
    instance.save(message, [&](code, size_t accepted) NOEXCEPT
    {
        promise_count.set_value(accepted);
    });
    BOOST_REQUIRE_EQUAL(promise_count.get_future().get(), 3u);
    BOOST_REQUIRE_EQUAL(instance.count(), 3u);

    instance.stop();
    BOOST_REQUIRE(test::exists(TEST_NAME));
}

BOOST_AUTO_TEST_CASE(hosts__save__redundant__expected)
{
    const logger log{};
    mock_settings set(bc::system::chain::selection::mainnet);
    set.path = TEST_NAME;
    set.host_pool_capacity = 42;
    hosts instance(set, log);
    BOOST_REQUIRE_EQUAL(instance.start(), error::success);
    BOOST_REQUIRE_EQUAL(instance.count(), 0u);

    const auto message = system::to_shared(address{ {host1, host2, host3, host3, host2, host1 } });
    std::promise<size_t> promise_count{};
    instance.save(message, [&](code, size_t accepted) NOEXCEPT
    {
        promise_count.set_value(accepted);
    });
    BOOST_REQUIRE_EQUAL(promise_count.get_future().get(), 3u);
    BOOST_REQUIRE_EQUAL(instance.count(), 3u);

    instance.stop();
    BOOST_REQUIRE(test::exists(TEST_NAME));
}

BOOST_AUTO_TEST_SUITE_END()
