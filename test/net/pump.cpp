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

BOOST_AUTO_TEST_SUITE(pump_tests)

BOOST_AUTO_TEST_CASE(pump__construct__stop__stops)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    pump instance(strand);

    std::promise<bool> promise;
    boost::asio::post(strand, [&]()
    {
        instance.stop(error::service_stopped);
        promise.set_value(true);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(promise.get_future().get());
}

BOOST_AUTO_TEST_CASE(pump__subscribe__stop__expected_code)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    pump instance(strand);
    constexpr auto expected_ec = error::invalid_magic;

    std::promise<code> promise;
    boost::asio::post(strand, [&]()
    {
        instance.subscribe([&](const code& ec, messages::ping::cptr ping)
        {
            // Stop notification has nullptr message and specified code.
            BOOST_REQUIRE(!ping);
            promise.set_value(ec);
        });
    });

    boost::asio::post(strand, [&]()
    {
        instance.stop(expected_ec);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), expected_ec);
}

BOOST_AUTO_TEST_CASE(pump__notify__invalid_message__no_notification)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    pump instance(strand);
    constexpr auto expected_ec = error::invalid_magic;

    // Subscription will capture only the stop notification.
    std::promise<code> promise;
    boost::asio::post(strand, [&]()
    {
        instance.subscribe([&](const code& ec, messages::ping::cptr ping)
        {
            BOOST_REQUIRE(!ping);
            promise.set_value(ec);
        });
    });

    // Invalid object deserialization will not cause a notification.
    system::data_chunk empty;
    system::read::bytes::copy reader(empty);
    boost::asio::post(strand, [&]()
    {
        constexpr auto nonced_ping_version = messages::level::bip31;

        // This line throws and is caught internal to the low level stream.
        const auto ec = instance.notify(messages::identifier::ping, nonced_ping_version, reader);
        BOOST_REQUIRE_EQUAL(ec, error::invalid_message);
    });

    boost::asio::post(strand, [&]()
    {
        instance.stop(expected_ec);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), expected_ec);
    BOOST_REQUIRE(!reader);
}

BOOST_AUTO_TEST_CASE(pump__notify__valid_message_invalid_version__no_notification)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    pump instance(strand);
    constexpr auto expected_ec = error::invalid_magic;

    // Subscription will capture only the stop notification.
    std::promise<code> promise;
    boost::asio::post(strand, [&]()
    {
        instance.subscribe([&](const code& ec, messages::ping::cptr ping)
        {
            BOOST_REQUIRE(!ping);
            promise.set_value(ec);
        });
    });
    
    // Invalid object version will not cause a notification.
    const auto ping = system::to_little_endian<uint64_t>(42);
    system::read::bytes::copy reader(ping);
    boost::asio::post(strand, [&]()
    {
        constexpr uint32_t invalid_ping_version = 0;
        const auto ec = instance.notify(messages::identifier::ping, invalid_ping_version, reader);
        BOOST_REQUIRE_EQUAL(ec, error::invalid_message);
    });

    boost::asio::post(strand, [&]()
    {
        instance.stop(expected_ec);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), expected_ec);
    BOOST_REQUIRE(!reader);
}

BOOST_AUTO_TEST_CASE(pump__notify__valid_nonced_ping__expected_notification)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    pump instance(strand);
    constexpr uint64_t expected_nonce = 42;
    constexpr auto expected_ec = error::invalid_magic;

    // Subscription will capture message and stop notifications.
    std::promise<code> promise;
    boost::asio::post(strand, [&]()
    {
        instance.subscribe([&](const code& ec, messages::ping::cptr ping)
        {
            // Handle stop notification (unavoidable test condition).
            if (!ping)
            {
                promise.set_value(ec);
                return;
            }

            // Handle message notification.
            BOOST_REQUIRE_EQUAL(ping->nonce, expected_nonce);
            BOOST_REQUIRE_EQUAL(ec, error::success);
        });
    });

    const auto ping = system::to_little_endian<uint64_t>(expected_nonce);
    system::read::bytes::copy reader(ping);
    boost::asio::post(strand, [&]()
    {
        constexpr auto nonced_ping_version = messages::level::bip31;
        const auto ec = instance.notify(messages::identifier::ping, nonced_ping_version, reader);
        BOOST_REQUIRE_EQUAL(ec, error::success);
    });

    boost::asio::post(strand, [&]()
    {
        instance.stop(expected_ec);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), expected_ec);
    BOOST_REQUIRE(reader);
}

BOOST_AUTO_TEST_SUITE_END()
