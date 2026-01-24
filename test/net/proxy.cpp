/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

    // Access protected constructor.
    mock_proxy(const socket::ptr& socket) NOEXCEPT
      : proxy(socket)
    {
    }
};

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
