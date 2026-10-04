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

BOOST_AUTO_TEST_SUITE(channel_tests)

// channel is abstract non-virtual base class.
struct accessor
  : public channel
{
    ////using channel::channel;
    accessor(const logger& log, const socket::ptr& socket, uint64_t identifier,
        const network::settings& settings,
        const channel::options_t& options) NOEXCEPT
      : channel(log, socket, identifier, settings, options)
    {
    }

    static uint32_t rate_limited1(const network::settings& settings,
        const channel::options_t& options) NOEXCEPT
    {
        return channel::rate_limited(settings, options);
    }
};

struct timed_options
  : channel::options_t
{
    timed_options(steady_clock::duration inactive,
        steady_clock::duration expire) NOEXCEPT
      : channel::options_t("test"), inactive_(inactive), expire_(expire)
    {
    }

    steady_clock::duration inactivity() const NOEXCEPT override
    {
        return inactive_;
    }

    steady_clock::duration expiration() const NOEXCEPT override
    {
        return expire_;
    }

    const steady_clock::duration inactive_;
    const steady_clock::duration expire_;
};

static const timed_options untimed{ {}, {} };
static const timed_options expiring{ {}, std::chrono::milliseconds(1) };
static const timed_options inactive{ std::chrono::milliseconds(1), {} };
static const channel::options_t default_options{ "test" };

// rate_limited

BOOST_AUTO_TEST_CASE(channel__rate_limited__both_zero__zero)
{
    settings set(bc::system::chain::selection::mainnet);
    set.rate_limit = 0;
    set.outbound.rate_limit = 0;
    BOOST_REQUIRE_EQUAL(accessor::rate_limited1(set, set.outbound), 0u);
}

BOOST_AUTO_TEST_CASE(channel__rate_limited__network_zero__service)
{
    settings set(bc::system::chain::selection::mainnet);
    set.rate_limit = 0;
    set.outbound.rate_limit = 42;
    BOOST_REQUIRE_EQUAL(accessor::rate_limited1(set, set.outbound), 42u);
}

BOOST_AUTO_TEST_CASE(channel__rate_limited__service_zero__network)
{
    settings set(bc::system::chain::selection::mainnet);
    set.rate_limit = 42;
    set.outbound.rate_limit = 0;
    BOOST_REQUIRE_EQUAL(accessor::rate_limited1(set, set.outbound), 42u);
}

BOOST_AUTO_TEST_CASE(channel__rate_limited__network_lesser__network)
{
    settings set(bc::system::chain::selection::mainnet);
    set.rate_limit = 24;
    set.outbound.rate_limit = 42;
    BOOST_REQUIRE_EQUAL(accessor::rate_limited1(set, set.outbound), 24u);
}

BOOST_AUTO_TEST_CASE(channel__rate_limited__service_lesser__service)
{
    settings set(bc::system::chain::selection::mainnet);
    set.rate_limit = 42;
    set.outbound.rate_limit = 24;
    BOOST_REQUIRE_EQUAL(accessor::rate_limited1(set, set.outbound), 24u);
}

BOOST_AUTO_TEST_CASE(channel__stopped__default__false)
{
    constexpr auto expected = 42u;
    const logger log{};
    threadpool pool{ one };
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<accessor>(log, socket_ptr, expected, set, set.outbound);
    BOOST_REQUIRE(!channel_ptr->stopped());
    BOOST_REQUIRE_NE(channel_ptr->nonce(), zero);
    BOOST_REQUIRE_EQUAL(channel_ptr->identifier(), expected);

    // Stop completion is asynchronous.
    channel_ptr->stop(error::invalid_magic);
    channel_ptr.reset();
}

BOOST_AUTO_TEST_CASE(channel__created__default__read_and_write_times_equal)
{
    const logger log{};
    threadpool pool(2);
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42, .maximum_buffer = default_options.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<accessor>(log, socket_ptr, 42, set, default_options);

    std::promise<bool> equal{};
    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        equal.set_value(channel_ptr->created() == channel_ptr->last_read() && channel_ptr->created() == channel_ptr->last_write());
    });

    BOOST_REQUIRE(equal.get_future().get());
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel__remaining__zero_inactivity__zero)
{
    const logger log{};
    threadpool pool(2);
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42, .maximum_buffer = default_options.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<accessor>(log, socket_ptr, 42, set, untimed);

    std::promise<size_t> remaining{};
    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        remaining.set_value(channel_ptr->remaining());
    });

    BOOST_REQUIRE(is_zero(remaining.get_future().get()));
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel__remaining__default_inactivity__within_inactivity)
{
    const logger log{};
    threadpool pool(2);
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42, .maximum_buffer = default_options.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<accessor>(log, socket_ptr, 42, set, default_options);

    std::promise<size_t> remaining{};
    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        remaining.set_value(channel_ptr->remaining());
    });

    const auto seconds = remaining.get_future().get();
    BOOST_REQUIRE(!is_zero(seconds));
    BOOST_REQUIRE_LE(seconds, default_options.inactivity_minutes * 60u);
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel__pause__resume__held_then_released)
{
    const logger log{};
    threadpool pool(2);
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42, .maximum_buffer = default_options.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<accessor>(log, socket_ptr, 42, set, untimed);

    std::promise<std::string> states{};
    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        const auto initial = channel_ptr->held();
        channel_ptr->pause();
        const auto paused = channel_ptr->held();
        channel_ptr->resume();
        const auto resumed = channel_ptr->held();
        channel_ptr->resume();
        states.set_value(std::to_string(initial) + std::to_string(paused) + std::to_string(resumed) + std::to_string(channel_ptr->held()) + std::to_string(!channel_ptr->gate()));
    });

    BOOST_REQUIRE_EQUAL(states.get_future().get(), "01001");
    BOOST_REQUIRE(!channel_ptr->stopped());
    channel_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(channel__stop__paused__released)
{
    const logger log{};
    threadpool pool(2);
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42, .maximum_buffer = default_options.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<accessor>(log, socket_ptr, 42, set, untimed);
    const std::weak_ptr<accessor> weak{ channel_ptr };

    std::promise<bool> paused{};
    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        channel_ptr->pause();
        const auto held = channel_ptr->held();
        channel_ptr->stop(error::invalid_magic);
        paused.set_value(held);
    });

    auto future = paused.get_future();
    BOOST_REQUIRE(future.wait_for(std::chrono::seconds(10)) == std::future_status::ready);
    BOOST_REQUIRE(future.get());

    channel_ptr.reset();
    socket_ptr.reset();
    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(weak.expired());
}

BOOST_AUTO_TEST_CASE(channel__resume__expiration_elapsed__channel_expired)
{
    const logger log{};
    threadpool pool(2);
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42, .maximum_buffer = default_options.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<accessor>(log, socket_ptr, 42, set, expiring);

    std::promise<code> stopped{};
    channel_ptr->subscribe_stop([&](const code& ec) NOEXCEPT
    {
        stopped.set_value(ec);
    }, [](const code&) NOEXCEPT {});

    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        channel_ptr->resume();
    });

    BOOST_REQUIRE_EQUAL(stopped.get_future().get(), error::channel_expired);
}

BOOST_AUTO_TEST_CASE(channel__resume__inactivity_elapsed__channel_inactive)
{
    const logger log{};
    threadpool pool(2);
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42, .maximum_buffer = default_options.maximum_buffer };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<accessor>(log, socket_ptr, 42, set, inactive);

    std::promise<code> stopped{};
    channel_ptr->subscribe_stop([&](const code& ec) NOEXCEPT
    {
        stopped.set_value(ec);
    }, [](const code&) NOEXCEPT {});

    boost::asio::post(channel_ptr->strand(), [&]() NOEXCEPT
    {
        channel_ptr->resume();
    });

    BOOST_REQUIRE_EQUAL(stopped.get_future().get(), error::channel_inactive);
}

BOOST_AUTO_TEST_SUITE_END()
