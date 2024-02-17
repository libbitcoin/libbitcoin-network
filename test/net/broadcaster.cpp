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

BOOST_AUTO_TEST_SUITE(broadcaster_tests)

BOOST_AUTO_TEST_CASE(broadcaster__construct__stop__stops)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    broadcaster instance(strand);

    std::promise<bool> promise;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::service_stopped);
        promise.set_value(true);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(promise.get_future().get());
}

BOOST_AUTO_TEST_CASE(broadcaster__subscribe__stop__expected_code)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    broadcaster instance(strand);
    constexpr uint64_t channel_id = 42;
    constexpr auto expected_ec = error::invalid_magic;
    auto result = true;

    std::promise<code> promise;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        BOOST_REQUIRE(!instance.subscribe(
            [&](const code& ec, const messages::ping::cptr& ping, broadcaster::channel_id id) NOEXCEPT
            {
                // Stop notification has nullptr message, zero id, and specified code.
                result &= is_null(ping);
                result &= is_zero(id);
                promise.set_value(ec);
                return true;
            }, channel_id));
    });

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(expected_ec);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), expected_ec);
    BOOST_REQUIRE(result);
}

BOOST_AUTO_TEST_CASE(broadcaster__notify__valid_nonced_ping__expected_notification)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    broadcaster instance(strand);
    constexpr uint64_t channel_id = 42;
    constexpr uint64_t expected_nonce = 42;
    constexpr auto expected_ec = error::invalid_magic;
    auto result = true;

    // Subscription will capture message and stop notifications.
    std::promise<code> promise;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        BOOST_REQUIRE(!instance.subscribe(
            [&](const code& ec, const messages::ping::cptr& ping, broadcaster::channel_id id) NOEXCEPT
            {
                // Handle stop notification (unavoidable test condition).
                if (!ping)
                {
                    promise.set_value(ec);
                    return true;
                }

                // Handle message notification.
                result &= (ping->nonce == expected_nonce);
                result &= (ec == error::success);
                result &= (id == channel_id);
                return true;
            }, channel_id));
    });

    // Move vs. emplace required on some platforms, possibly due to default constructor.
    const auto ping = std::make_shared<messages::ping>(messages::ping{ expected_nonce });
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.notify(ping, channel_id);
    });

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(expected_ec);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), expected_ec);
    BOOST_REQUIRE(result);
}

BOOST_AUTO_TEST_SUITE_END()
