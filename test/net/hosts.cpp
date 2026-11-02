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

BC_FIXTURE_TEST_SUITE(hosts_tests, hosts_tests_setup_fixture)

using namespace messages::peer;

class mock_settings final
  : public settings
{
public:
    mock_settings(const settings& base) NOEXCEPT
      : settings(base)
    {
    }

    // Override derivative name, using directory as file.
    std::filesystem::path file() const NOEXCEPT override
    {
        return path;
    }
};

constexpr address_item host1{ 0, 0, ipv6_t{ loopback_ip_address }, 1 };
constexpr address_item host2{ 0, 0, ipv6_t{ loopback_ip_address }, 2 };
constexpr address_item host3{ 0, 0, ipv6_t{ loopback_ip_address }, 3 };
constexpr address_item loopback00{ 0, 0, ipv6_t{ loopback_ip_address }, 0 };
constexpr address_item loopback42{ 0, 0, ipv6_t{ loopback_ip_address }, 42 };
constexpr address_item onion42{ 0, 0, torv3_t{ { 0x01 } }, 42 };
constexpr address_item unspecified00{ 0, 0, ipv6_t{ unspecified_ip_address }, 0 };
////constexpr address_item unspecified42{ 0, 0, ipv6_t{ unspecified_ip_address }, 42 };

// start

BC_AUTO_TEST_CASE(hosts__start__disabled__success)
{
    const logger log{};
    mock_settings set(test::network_settings());
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(set.outbound.host_pool_capacity, 0u);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
}

BC_AUTO_TEST_CASE(hosts__start__enabled__success)
{
    // Non-empty capacity causes file open/load.
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE(!test::exists(TEST_NAME));

    instance.stop();
}

BC_AUTO_TEST_CASE(hosts__start__disabled_start__success)
{
    const logger log{};
    mock_settings set(test::network_settings());
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
}

BC_AUTO_TEST_CASE(hosts__start__enabled_started__success)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);

    // Not idempotent start.
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.start(), error::operation_failed);
    BC_REQUIRE(!test::exists(TEST_NAME));

    instance.stop();
}

BC_AUTO_TEST_CASE(hosts__start__populated_file__expected)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance1(set, log);

    // File is deleted if empty on open.
    BC_REQUIRE(test::create(TEST_NAME));
    BC_REQUIRE(test::exists(TEST_NAME));
    BC_REQUIRE_EQUAL(instance1.start(), error::success);
    BC_REQUIRE_EQUAL(instance1.count(), 0u);
    BC_REQUIRE(!test::exists(TEST_NAME));

    const auto message = system::to_shared(address{ { host1, host2, host3 } });
    std::promise<size_t> promise_count{};
    instance1.save(message, [&](code, size_t accepted)
    {
        promise_count.set_value(accepted);
    });
    BC_REQUIRE_EQUAL(promise_count.get_future().get(), 3u);
    BC_REQUIRE_EQUAL(instance1.count(), 3u);

    // File is not created until stop.
    BC_REQUIRE(!test::exists(TEST_NAME));

    // File is created with three entries.
    instance1.stop();
    BC_REQUIRE(test::exists(TEST_NAME));

    // Start with existing file and read entries.
    hosts instance2(set, log);
    set.gossip_ipv6 = true;
    BC_REQUIRE_EQUAL(instance2.start(), error::success);
    BC_REQUIRE_EQUAL(instance2.count(), 3u);

    instance2.stop();
    BC_REQUIRE(test::exists(TEST_NAME));
}

// stop

BC_AUTO_TEST_CASE(hosts__stop__disabled__success)
{
    const logger log{};
    mock_settings set(test::network_settings());
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE(!test::exists(TEST_NAME));

    // Idempotent stop.
    BC_REQUIRE_EQUAL(instance.stop(), error::success);
    BC_REQUIRE_EQUAL(instance.stop(), error::success);
}

BC_AUTO_TEST_CASE(hosts__stop__enabled_stopped__success)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);

    // Idempotent stop.
    BC_REQUIRE_EQUAL(instance.stop(), error::success);
    BC_REQUIRE_EQUAL(instance.stop(), error::success);
}

BC_AUTO_TEST_CASE(hosts__stop__enabled_started__success)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);

    // Idempotent stop.
    BC_REQUIRE_EQUAL(instance.stop(), error::success);
    BC_REQUIRE_EQUAL(instance.stop(), error::success);
}

// count

BC_AUTO_TEST_CASE(hosts__count__empty__zero)
{
    const logger log{};
    mock_settings set(test::network_settings());
    const hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.count(), 0u);
}

// counts

constexpr auto ipv4_index = ipv4_t::id;
constexpr auto ipv6_index = ipv6_t::id;
constexpr ip_address mapped_ip_address =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x01
};
constexpr address_item mapped42{ 0, 0, ipv4_t{ mapped_ip_address }, 42 };

BC_AUTO_TEST_CASE(hosts__counts__empty__zeros)
{
    const logger log{};
    mock_settings set(test::network_settings());
    const hosts instance(set, log);
    BC_REQUIRE(instance.counts() == config::address_counts{});
}

BC_AUTO_TEST_CASE(hosts__counts__mixed_take_stop__expected)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);

    std::promise<code> promise_v6{};
    instance.restore(system::to_shared(loopback42), [&](const code& ec) NOEXCEPT
    {
        promise_v6.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise_v6.get_future().get(), error::success);

    std::promise<code> promise_v4{};
    instance.restore(system::to_shared(mapped42), [&](const code& ec) NOEXCEPT
    {
        promise_v4.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise_v4.get_future().get(), error::success);

    auto counts = instance.counts();
    BC_REQUIRE_EQUAL(counts.at(ipv4_index), 1u);
    BC_REQUIRE_EQUAL(counts.at(ipv6_index), 1u);
    BC_REQUIRE_EQUAL(instance.count(), 2u);

    std::promise<std::pair<code, address_item_cptr>> promise_take{};
    instance.take(hosts::family::any, [&](const code& ec, const address_item_cptr& item) NOEXCEPT
    {
        promise_take.set_value({ ec, item });
    });

    BC_REQUIRE_EQUAL(promise_take.get_future().get().first, error::success);

    counts = instance.counts();
    BC_REQUIRE_EQUAL(counts.at(ipv4_index) + counts.at(ipv6_index), 1u);
    BC_REQUIRE_EQUAL(instance.count(), 1u);

    instance.stop();
    counts = instance.counts();
    BC_REQUIRE_EQUAL(counts.at(ipv4_index), 0u);
    BC_REQUIRE_EQUAL(counts.at(ipv6_index), 0u);
}

BC_AUTO_TEST_CASE(hosts__take__family__expected)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);

    std::promise<code> promise_v6{};
    instance.restore(system::to_shared(loopback42), [&](const code& ec) NOEXCEPT
    {
        promise_v6.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise_v6.get_future().get(), error::success);

    std::promise<code> promise_v4{};
    instance.restore(system::to_shared(mapped42), [&](const code& ec) NOEXCEPT
    {
        promise_v4.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise_v4.get_future().get(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 2u);

    std::promise<std::pair<code, address_item_cptr>> promise_take_v4{};
    instance.take(hosts::family::ipv4, [&](const code& ec, const address_item_cptr& item) NOEXCEPT
    {
        promise_take_v4.set_value({ ec, item });
    });

    const auto taken_v4 = promise_take_v4.get_future().get();
    BC_REQUIRE_EQUAL(taken_v4.first, error::success);
    BC_REQUIRE(*taken_v4.second == mapped42);
    BC_REQUIRE_EQUAL(instance.count(), 1u);

    std::promise<std::pair<code, address_item_cptr>> promise_take_v4_again{};
    instance.take(hosts::family::ipv4, [&](const code& ec, const address_item_cptr& item) NOEXCEPT
    {
        promise_take_v4_again.set_value({ ec, item });
    });

    BC_REQUIRE_EQUAL(promise_take_v4_again.get_future().get().first, error::address_not_found);
    BC_REQUIRE_EQUAL(instance.count(), 1u);

    std::promise<std::pair<code, address_item_cptr>> promise_take_v6{};
    instance.take(hosts::family::ipv6, [&](const code& ec, const address_item_cptr& item) NOEXCEPT
    {
        promise_take_v6.set_value({ ec, item });
    });

    const auto taken_v6 = promise_take_v6.get_future().get();
    BC_REQUIRE_EQUAL(taken_v6.first, error::success);
    BC_REQUIRE(*taken_v6.second == loopback42);
    BC_REQUIRE_EQUAL(instance.count(), 0u);

    instance.stop();
}

BC_AUTO_TEST_CASE(hosts__counts__eviction__expected)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 2;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);

    const auto message = system::to_shared(address{ { host1, host2, host3 } });
    std::promise<code> promise_save{};
    instance.save(message, [&](const code& ec, size_t) NOEXCEPT
    {
        promise_save.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise_save.get_future().get(), error::success);

    // Push into the full buffer evicts the oldest, count remains capacity.
    const auto counts = instance.counts();
    BC_REQUIRE_EQUAL(counts.at(ipv6_index), 2u);
    BC_REQUIRE_EQUAL(instance.count(), 2u);

    instance.stop();
}

// take

BC_AUTO_TEST_CASE(hosts__take__unconnectable__address_not_found_and_retained)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);

    std::promise<code> promise_restore{};
    instance.restore(system::to_shared(onion42), [&](const code& ec) NOEXCEPT
    {
        promise_restore.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise_restore.get_future().get(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 1u);

    std::promise<std::pair<code, address_item_cptr>> promise_take{};
    instance.take(hosts::family::any, [&](const code& ec, const address_item_cptr& item) NOEXCEPT
    {
        promise_take.set_value({ ec, item });
    });

    const auto result = promise_take.get_future().get();
    BC_REQUIRE_EQUAL(result.first, error::address_not_found);
    BC_REQUIRE(!result.second);
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    instance.stop();
}

BC_AUTO_TEST_CASE(hosts__take__mixed__connectable_taken_other_retained)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);

    std::promise<code> promise_onion{};
    instance.restore(system::to_shared(onion42), [&](const code& ec) NOEXCEPT
    {
        promise_onion.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise_onion.get_future().get(), error::success);

    std::promise<code> promise_loopback{};
    instance.restore(system::to_shared(loopback42), [&](const code& ec) NOEXCEPT
    {
        promise_loopback.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise_loopback.get_future().get(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 2u);

    std::promise<std::pair<code, address_item_cptr>> promise_take{};
    instance.take(hosts::family::any, [&](const code& ec, const address_item_cptr& item) NOEXCEPT
    {
        promise_take.set_value({ ec, item });
    });

    const auto result = promise_take.get_future().get();
    BC_REQUIRE_EQUAL(result.first, error::success);
    BC_REQUIRE(*result.second == loopback42);
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    BC_REQUIRE_EQUAL(instance.counts().at(torv3_t::id), 1u);
    instance.stop();
}

BC_AUTO_TEST_CASE(hosts__take__empty__address_not_found)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 0u);

    std::promise<std::pair<code, address_item_cptr>> promise{};
    instance.take(hosts::family::any, [&](const code& ec, const address_item_cptr& item) NOEXCEPT
    {
        promise.set_value({ ec, item });
    });

    instance.stop();
    const auto result = promise.get_future().get();
    BC_REQUIRE_EQUAL(result.first, error::address_not_found);
    BC_REQUIRE(!result.second);
    BC_REQUIRE_EQUAL(instance.count(), 0u);
    BC_REQUIRE(!test::exists(TEST_NAME));
}

BC_AUTO_TEST_CASE(hosts__take__only__expected)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 0u);

    std::promise<code> promise_restore{};
    instance.restore(system::to_shared(loopback42), [&](const code& ec) NOEXCEPT
    {
        promise_restore.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise_restore.get_future().get(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 1u);

    std::promise<std::pair<code, address_item_cptr>> promise_take{};
    instance.take(hosts::family::any, [&](const code& ec, const address_item_cptr& item) NOEXCEPT
    {
        promise_take.set_value({ ec, item });
    });

    instance.stop();
    const auto result = promise_take.get_future().get();
    BC_REQUIRE_EQUAL(result.first, error::success);
    BC_REQUIRE(*result.second == loopback42);
    BC_REQUIRE_EQUAL(instance.count(), 0u);
    BC_REQUIRE(!test::exists(TEST_NAME));
}

// restore

BC_AUTO_TEST_CASE(hosts__restore__disabled_stopped__service_stopped_empty)
{
    const logger log{};
    mock_settings set(test::network_settings());
    hosts instance(set, log);
    std::promise<code> promise{};
    instance.restore(system::to_shared(loopback00), [&](const code& ec) NOEXCEPT
    {
        promise.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise.get_future().get(), error::service_stopped);
    BC_REQUIRE_EQUAL(instance.count(), 0u);
}

BC_AUTO_TEST_CASE(hosts__restore__stopped__service_stopped_empty)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    std::promise<code> promise{};
    instance.restore(system::to_shared(unspecified00), [&](const code& ec) NOEXCEPT
    {
        promise.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise.get_future().get(), error::service_stopped);
    BC_REQUIRE_EQUAL(instance.count(), 0u);

    instance.stop();
    BC_REQUIRE(!test::exists(TEST_NAME));
}

BC_AUTO_TEST_CASE(hosts__restore__unique__accepted)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 0u);

    std::promise<code> promise{};
    instance.restore(system::to_shared(loopback42), [&](const code& ec) NOEXCEPT
    {
        promise.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise.get_future().get(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 1u);

    instance.stop();
    BC_REQUIRE(test::exists(TEST_NAME));
}

BC_AUTO_TEST_CASE(hosts__restore__duplicate_authority__updated)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 0u);

    constexpr address_item loopback42a{ 1, 2, ipv6_t{ loopback_ip_address }, 42 };
    constexpr address_item loopback42b{ 3, 4, ipv6_t{ loopback_ip_address }, 42 };

    std::promise<code> promise1{};
    instance.restore(system::to_shared(loopback42a), [&](const code& ec) NOEXCEPT
    {
        promise1.set_value(ec);
    });
    BC_REQUIRE_EQUAL(promise1.get_future().get(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 1u);

    std::promise<code> promise2{};
    instance.restore(system::to_shared(loopback42b), [&](const code& ec) NOEXCEPT
    {
        promise2.set_value(ec);
    });
    BC_REQUIRE_EQUAL(promise2.get_future().get(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 1u);

    instance.stop();
    BC_REQUIRE(test::exists(TEST_NAME));
}

// fetch

// file round trip

BC_AUTO_TEST_CASE(hosts__stop__named_address__round_trips)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    set.gossip_tor = true;

    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);

    std::promise<code> promise_restore{};
    instance.restore(system::to_shared(onion42), [&](const code& ec) NOEXCEPT
    {
        promise_restore.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise_restore.get_future().get(), error::success);
    BC_REQUIRE_EQUAL(instance.counts().at(torv3_t::id), 1u);
    BC_REQUIRE_EQUAL(instance.stop(), error::success);
    BC_REQUIRE(test::exists(TEST_NAME));

    hosts reloaded(set, log);
    BC_REQUIRE_EQUAL(reloaded.start(), error::success);
    BC_REQUIRE_EQUAL(reloaded.count(), 1u);
    BC_REQUIRE_EQUAL(reloaded.counts().at(torv3_t::id), 1u);
    BC_REQUIRE_EQUAL(reloaded.stop(), error::success);
}

BC_AUTO_TEST_CASE(hosts__start__named_address_not_gossiped__purged)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    set.gossip_tor = true;

    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);

    std::promise<code> promise_restore{};
    instance.restore(system::to_shared(onion42), [&](const code& ec) NOEXCEPT
    {
        promise_restore.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise_restore.get_future().get(), error::success);
    BC_REQUIRE_EQUAL(instance.stop(), error::success);

    set.gossip_tor = false;
    hosts reloaded(set, log);
    BC_REQUIRE_EQUAL(reloaded.start(), error::success);
    BC_REQUIRE_EQUAL(reloaded.count(), 0u);
    BC_REQUIRE_EQUAL(reloaded.stop(), error::success);
}

BC_AUTO_TEST_CASE(hosts__fetch__empty__address_not_found)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 0u);

    std::promise<std::pair<code, address::cptr>> promise_fetch{};
    instance.fetch([&](const code& ec, const address::cptr& message) NOEXCEPT
    {
        promise_fetch.set_value({ ec, message });
    });

    const auto result = promise_fetch.get_future().get();
    BC_REQUIRE_EQUAL(result.first, error::address_not_found);
    BC_REQUIRE(!result.second);

    instance.stop();
    BC_REQUIRE(!test::exists(TEST_NAME));
}

BC_AUTO_TEST_CASE(hosts__fetch__single_undivided__expected)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    set.address_lower = 1;
    set.address_upper = 1;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);

    std::promise<code> promise_restore{};
    instance.restore(system::to_shared(loopback42), [&](const code& ec) NOEXCEPT
    {
        promise_restore.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise_restore.get_future().get(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 1u);

    std::promise<std::pair<code, address::cptr>> promise_fetch{};
    instance.fetch([&](const code& ec, const address::cptr& message) NOEXCEPT
    {
        promise_fetch.set_value({ ec, message });
    });

    const auto result = promise_fetch.get_future().get();
    BC_REQUIRE_EQUAL(result.first, error::success);
    BC_REQUIRE_EQUAL(result.second->addresses.size(), 1u);
    BC_REQUIRE(result.second->addresses.front() == loopback42);
    instance.stop();
}

BC_AUTO_TEST_CASE(hosts__fetch__three__success_empty)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 0u);

    const auto message = system::to_shared(address{ { host1, host2, host3 } });
    std::promise<size_t> promise_save{};
    instance.save(message, [&](code, size_t accepted) NOEXCEPT
    {
        promise_save.set_value(accepted);
    });
    BC_REQUIRE_EQUAL(promise_save.get_future().get(), 3u);

    std::promise<std::pair<code, address::cptr>> promise_fetch{};
    instance.fetch([&](const code& ec, const address::cptr& message) NOEXCEPT
    {
        promise_fetch.set_value({ ec, message });
    });

    const auto result = promise_fetch.get_future().get();
    BC_REQUIRE_EQUAL(result.first, error::success);
    BC_REQUIRE(result.second->addresses.empty());

    instance.stop();
    BC_REQUIRE(test::exists(TEST_NAME));
}

// store

BC_AUTO_TEST_CASE(hosts__save__three_unique__three)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 0u);
    
    const auto message = system::to_shared(address{ { host1, host2, host3 } });
    std::promise<size_t> promise_count{};
    instance.save(message, [&](code, size_t accepted) NOEXCEPT
    {
        promise_count.set_value(accepted);
    });
    BC_REQUIRE_EQUAL(promise_count.get_future().get(), 3u);
    BC_REQUIRE_EQUAL(instance.count(), 3u);

    instance.stop();
    BC_REQUIRE(test::exists(TEST_NAME));
}

BC_AUTO_TEST_CASE(hosts__save__redundant__expected)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 0u);

    const auto message = system::to_shared(address{ {host1, host2, host3, host3, host2, host1 } });
    std::promise<size_t> promise_count{};
    instance.save(message, [&](code, size_t accepted) NOEXCEPT
    {
        promise_count.set_value(accepted);
    });
    BC_REQUIRE_EQUAL(promise_count.get_future().get(), 3u);
    BC_REQUIRE_EQUAL(instance.count(), 3u);

    instance.stop();
    BC_REQUIRE(test::exists(TEST_NAME));
}

BC_AUTO_TEST_CASE(hosts__save__stopped__service_stopped)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);

    const auto message = system::to_shared(address{ { host1 } });
    std::promise<std::pair<code, size_t>> promise{};
    instance.save(message, [&](const code& ec, size_t accepted) NOEXCEPT
    {
        promise.set_value({ ec, accepted });
    });

    const auto result = promise.get_future().get();
    BC_REQUIRE_EQUAL(result.first, error::service_stopped);
    BC_REQUIRE_EQUAL(result.second, 0u);
}

BC_AUTO_TEST_CASE(hosts__save__empty__address_not_found)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);

    const auto message = system::to_shared(address{});
    std::promise<std::pair<code, size_t>> promise{};
    instance.save(message, [&](const code& ec, size_t accepted) NOEXCEPT
    {
        promise.set_value({ ec, accepted });
    });

    const auto result = promise.get_future().get();
    BC_REQUIRE_EQUAL(result.first, error::address_not_found);
    BC_REQUIRE_EQUAL(result.second, 0u);
    BC_REQUIRE_EQUAL(instance.count(), 0u);
    instance.stop();
}

BC_AUTO_TEST_CASE(hosts__save__insufficient_services__not_accepted)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log, service::node_network);
    BC_REQUIRE_EQUAL(instance.start(), error::success);

    constexpr address_item network42{ 0, service::node_network, ipv6_t{ loopback_ip_address }, 42 };
    const auto message = system::to_shared(address{ { host1, network42 } });
    std::promise<size_t> promise{};
    instance.save(message, [&](code, size_t accepted) NOEXCEPT
    {
        promise.set_value(accepted);
    });

    BC_REQUIRE_EQUAL(promise.get_future().get(), 1u);
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    instance.stop();
}

// reserve

BC_AUTO_TEST_CASE(hosts__reserve__unreserve__expected_reserved)
{
    const logger log{};
    mock_settings set(test::network_settings());
    hosts instance(set, log);
    const config::endpoint host{ "[::1]:42" };

    BC_REQUIRE_EQUAL(instance.reserved(), 0u);
    BC_REQUIRE(instance.reserve(host));
    BC_REQUIRE(!instance.reserve(host));
    BC_REQUIRE_EQUAL(instance.reserved(), 1u);
    BC_REQUIRE(instance.unreserve(host));
    BC_REQUIRE(!instance.unreserve(host));
    BC_REQUIRE_EQUAL(instance.reserved(), 0u);
}

BC_AUTO_TEST_CASE(hosts__take__reserved__address_not_found_and_removed)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);

    std::promise<code> promise_restore{};
    instance.restore(system::to_shared(loopback42), [&](const code& ec) NOEXCEPT
    {
        promise_restore.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise_restore.get_future().get(), error::success);
    BC_REQUIRE(instance.reserve({ loopback42 }));

    std::promise<code> promise_take{};
    instance.take(hosts::family::any, [&](const code& ec, const address_item_cptr&) NOEXCEPT
    {
        promise_take.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise_take.get_future().get(), error::address_not_found);
    BC_REQUIRE_EQUAL(instance.count(), 0u);
    instance.stop();
}

// load

static bool write_file(const std::string& name, const std::string& text)
{
    std::ofstream file{ name };
    file << text;
    return file.good();
}

BC_AUTO_TEST_CASE(hosts__start__unspecified_line__purged)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    set.gossip_ipv6 = true;
    BC_REQUIRE(write_file(TEST_NAME, "[::]\n[::1]:42/0/0\n"));

    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    instance.stop();
}

BC_AUTO_TEST_CASE(hosts__start__malformed_line__purged)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    set.gossip_ipv6 = true;
    BC_REQUIRE(write_file(TEST_NAME, "[::1]:42/0/0\nnot an address\n"));

    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    instance.stop();
}

BC_AUTO_TEST_CASE(hosts__start__insufficient_line__purged)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    set.gossip_ipv6 = true;
    BC_REQUIRE(write_file(TEST_NAME, "[::1]:42/0/0\n[::1]:43/0/1\n"));

    hosts instance(set, log, service::node_network);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    instance.stop();
}

BC_AUTO_TEST_CASE(hosts__start__unsupported_line__purged)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    set.gossip_ipv6 = true;
    set.invalid_services = service::node_bloom;
    BC_REQUIRE(write_file(TEST_NAME, "[::1]:42/0/4\n[::1]:43/0/1\n"));

    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    instance.stop();
}

BC_AUTO_TEST_CASE(hosts__start__peered_line__purged)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    set.gossip_ipv6 = true;
    set.manual.peers.emplace_back("[::1]:42");
    set.manual.initialize();
    BC_REQUIRE(write_file(TEST_NAME, "[::1]:42/0/0\n[::1]:43/0/0\n"));

    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    instance.stop();
}

BC_AUTO_TEST_CASE(hosts__start__blacklisted_line__purged)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    set.gossip_ipv6 = true;
    set.blacklists.emplace_back("[::1]:42");
    BC_REQUIRE(write_file(TEST_NAME, "[::1]:42/0/0\n[::1]:43/0/0\n"));

    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    instance.stop();
}

BC_AUTO_TEST_CASE(hosts__start__not_whitelisted_line__purged)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    set.gossip_ipv6 = true;
    set.whitelists.emplace_back("[::1]:43");
    BC_REQUIRE(write_file(TEST_NAME, "[::1]:42/0/0\n[::1]:43/0/0\n"));

    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE_EQUAL(instance.count(), 1u);
    instance.stop();
}

BC_AUTO_TEST_CASE(hosts__stop__directory_path__file_save)
{
    const logger log{};
    mock_settings set(test::network_settings());
    set.path = TEST_NAME;
    set.outbound.host_pool_capacity = 42;
    hosts instance(set, log);
    BC_REQUIRE_EQUAL(instance.start(), error::success);
    BC_REQUIRE(std::filesystem::create_directory(TEST_NAME));

    std::promise<code> promise_restore{};
    instance.restore(system::to_shared(loopback42), [&](const code& ec) NOEXCEPT
    {
        promise_restore.set_value(ec);
    });

    BC_REQUIRE_EQUAL(promise_restore.get_future().get(), error::success);
    BC_REQUIRE_EQUAL(instance.stop(), error::file_save);
}

BC_AUTO_TEST_SUITE_END()
