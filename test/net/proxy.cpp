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

BOOST_AUTO_TEST_SUITE(proxy_tests)

class mock_proxy
  : public proxy
{
public:
    // Call must be stranded.
    void subscribe_stop1(result_handler handler) NOEXCEPT
    {
        proxy::subscribe_stop(std::move(handler));
    }

    // Call must be stranded.
    steady_clock::duration unconsumed1(size_t bytes,
        const steady_clock::time_point& start) const NOEXCEPT
    {
        return proxy::unconsumed(bytes, start);
    }

    // Access protected constructor.
    mock_proxy(const socket::ptr& socket, uint32_t rate_limit=0) NOEXCEPT
      : proxy(socket, rate_limit)
    {
    }
};

// Obtain the deferral for a send of bytes that started at the given offset.
static milliseconds get_unconsumed(uint32_t rate_limit, size_t bytes,
    const steady_clock::duration& elapsed) NOEXCEPT
{
    const logger log{};
    threadpool pool(1);
    socket::parameters params{ .maximum_request = 42 };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr, rate_limit);

    std::promise<milliseconds> deferral;
    boost::asio::post(proxy_ptr->strand(), [=, &deferral]() NOEXCEPT
    {
        deferral.set_value(std::chrono::duration_cast<milliseconds>(
            proxy_ptr->unconsumed1(bytes, steady_clock::now() - elapsed)));
    });

    const auto result = deferral.get_future().get();
    proxy_ptr->stop(error::invalid_magic);
    pool.stop();
    return result;
}

BOOST_AUTO_TEST_CASE(proxy__unconsumed__unlimited__zero)
{
    BOOST_REQUIRE_EQUAL(get_unconsumed(0, 1000, seconds(0)), milliseconds(0));
}

BOOST_AUTO_TEST_CASE(proxy__unconsumed__untransmitted__full_allocation)
{
    // 1000 bytes at 1000 bytes/second is allocated one second, unconsumed.
    const auto deferral = get_unconsumed(1000, 1000, seconds(0));
    BOOST_REQUIRE_GT(deferral, milliseconds(900));
    BOOST_REQUIRE_LE(deferral, milliseconds(1000));
}

BOOST_AUTO_TEST_CASE(proxy__unconsumed__partly_transmitted__remainder)
{
    // Transmission consumed 250ms of the 1000ms allocation.
    const auto deferral = get_unconsumed(1000, 1000, milliseconds(250));
    BOOST_REQUIRE_GT(deferral, milliseconds(650));
    BOOST_REQUIRE_LE(deferral, milliseconds(750));
}

BOOST_AUTO_TEST_CASE(proxy__unconsumed__fully_transmitted__zero)
{
    // Transmission was slower than the rate limit, so there is nothing to add.
    BOOST_REQUIRE_EQUAL(get_unconsumed(1000, 1000, seconds(2)),
        milliseconds(0));
}

BOOST_AUTO_TEST_CASE(proxy__unconsumed__stopped__zero)
{
    const logger log{};
    threadpool pool(1);
    socket::parameters params{ .maximum_request = 42 };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr, 1000);
    proxy_ptr->stop(error::invalid_magic);

    std::promise<steady_clock::duration::rep> deferral;
    boost::asio::post(proxy_ptr->strand(), [=, &deferral]() NOEXCEPT
    {
        deferral.set_value(
            proxy_ptr->unconsumed1(1000, steady_clock::now()).count());
    });

    BOOST_REQUIRE_EQUAL(deferral.get_future().get(), 0);
}

BOOST_AUTO_TEST_CASE(proxy__paused__default__true)
{
    const logger log{};
    threadpool pool(1);
    socket::parameters params{ .maximum_request = 42 };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);

    std::promise<bool> paused;
    boost::asio::post(proxy_ptr->strand(), [=, &paused]() NOEXCEPT
    {
        paused.set_value(proxy_ptr->paused());
    });

    BOOST_REQUIRE(paused.get_future().get());
    proxy_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(proxy__paused__pause__true)
{
    const logger log{};
    threadpool pool(1);
    socket::parameters params{ .maximum_request = 42 };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);

    std::promise<bool> paused;
    boost::asio::post(proxy_ptr->strand(), [=, &paused]() NOEXCEPT
    {
        proxy_ptr->pause();
        paused.set_value(proxy_ptr->paused());
    });

    BOOST_REQUIRE(paused.get_future().get());
    proxy_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(proxy__paused__resume__false)
{
    const logger log{};
    threadpool pool(1);
    socket::parameters params{ .maximum_request = 42 };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);

    std::promise<bool> paused;
    boost::asio::post(proxy_ptr->strand(), [=, &paused]() NOEXCEPT
    {
        // Resume queues up a (failing) read that will not execute until after this.
        proxy_ptr->resume();
        paused.set_value(proxy_ptr->paused());
    });

    BOOST_REQUIRE(!paused.get_future().get());

    // Ensures stop is not executed concurrenty due to resume, guarding promise.
    std::promise<bool> stopped;
    boost::asio::post(proxy_ptr->strand(), [=, &stopped]() NOEXCEPT
    {
        proxy_ptr->stop(error::invalid_magic);
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
}

BOOST_AUTO_TEST_CASE(proxy__paused__resume_pause__true)
{
    const logger log{};
    threadpool pool(1);
    socket::parameters params{ .maximum_request = 42 };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);

    std::promise<bool> paused;
    boost::asio::post(proxy_ptr->strand(), [=, &paused]() NOEXCEPT
    {
        // Resume queues up a (failing) read that will not execute until after this. 
        proxy_ptr->resume();
        proxy_ptr->pause();
        paused.set_value(proxy_ptr->paused());
    });

    BOOST_REQUIRE(paused.get_future().get());

    // Ensures stop is not executed concurrenty due to resume, guarding promise.
    std::promise<bool> stopped;
    boost::asio::post(proxy_ptr->strand(), [=, &stopped]() NOEXCEPT
    {
        proxy_ptr->stop(error::invalid_magic);
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
}

BOOST_AUTO_TEST_CASE(proxy__stopped__default__false)
{
    const logger log{};
    threadpool pool(2);
    socket::parameters params{ .maximum_request = 42 };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    BOOST_REQUIRE(!proxy_ptr->stopped());

    proxy_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(proxy__stranded__default__false)
{
    const logger log{};
    threadpool pool(2);
    socket::parameters params{ .maximum_request = 42 };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    BOOST_REQUIRE(!proxy_ptr->stranded());

    proxy_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(proxy__authority__default__expected)
{
    const logger log{};
    threadpool pool(2);
    const config::endpoint default_endpoint{};
    socket::parameters params{ .maximum_request = 42 };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    BOOST_REQUIRE(proxy_ptr->endpoint() == default_endpoint);

    proxy_ptr->stop(error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(proxy__subscribe_stop__subscribed__expected)
{
    const logger log{};
    threadpool pool(2);
    socket::parameters params{ .maximum_request = 42 };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    constexpr auto expected_ec = error::invalid_magic;

    std::promise<code> stop2_stopped;
    std::promise<code> stop_subscribed;
    proxy_ptr->subscribe_stop(
        [=, &stop2_stopped](code ec) NOEXCEPT
        {
            stop2_stopped.set_value(ec);
        },
        [=, &stop_subscribed](code ec) NOEXCEPT
        {
            stop_subscribed.set_value(ec);
        });

    BOOST_REQUIRE(!proxy_ptr->stopped());
    BOOST_REQUIRE_EQUAL(stop_subscribed.get_future().get(), error::success);

    proxy_ptr->stop(expected_ec);
    BOOST_REQUIRE_EQUAL(stop2_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE(proxy_ptr->stopped());
}

BOOST_AUTO_TEST_CASE(proxy__do_subscribe_stop__subscribed__expected)
{
    const logger log{};
    threadpool pool(2);
    socket::parameters params{ .maximum_request = 42 };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto proxy_ptr = std::make_shared<mock_proxy>(socket_ptr);
    constexpr auto expected_ec = error::invalid_magic;

    std::promise<code> stop1_stopped;
    boost::asio::post(proxy_ptr->strand(), [&]() NOEXCEPT
    {
        proxy_ptr->subscribe_stop1([=, &stop1_stopped](code ec) NOEXCEPT
        {
            stop1_stopped.set_value(ec);
        });
    });

    BOOST_REQUIRE(!proxy_ptr->stopped());

    proxy_ptr->stop(expected_ec);
    BOOST_REQUIRE_EQUAL(stop1_stopped.get_future().get(), expected_ec);
    BOOST_REQUIRE(proxy_ptr->stopped());
}

BOOST_AUTO_TEST_SUITE_END()
