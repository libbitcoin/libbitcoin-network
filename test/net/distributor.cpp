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

BOOST_AUTO_TEST_SUITE(distributor_tests)

BOOST_AUTO_TEST_CASE(distributor__construct__stop__stops)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor instance(strand);

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

BOOST_AUTO_TEST_CASE(distributor__subscribe__stop__expected_code)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor instance(strand);
    constexpr auto expected_ec = error::invalid_magic;
    auto result = true;

    std::promise<code> promise;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](const code& ec, const messages::ping::cptr& ping) NOEXCEPT
        {
            // Stop notification has nullptr message and specified code.
            result &= is_null(ping);
            promise.set_value(ec);
            return true;
        });
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

BOOST_AUTO_TEST_CASE(distributor__notify__invalid_message__no_notification)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor instance(strand);
    constexpr auto expected_ec = error::invalid_magic;
    auto result = true;

    // Subscription will capture only the stop notification.
    std::promise<code> promise;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](const code& ec, const messages::ping::cptr& ping) NOEXCEPT
        {
            result &= is_null(ping);
            promise.set_value(ec);
            return true;
        });
    });

    // Invalid object deserialization will not cause a notification.
    system::data_chunk empty{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        constexpr auto nonced_ping_version = messages::level::bip31;

        // This line throws and is caught internal to the low level stream.
        const auto ec = instance.notify(messages::identifier::ping, nonced_ping_version, empty);
        result &= (ec == error::invalid_message);
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

BOOST_AUTO_TEST_CASE(distributor__notify__valid_message_invalid_version__no_notification)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor instance(strand);
    constexpr auto expected_ec = error::invalid_magic;
    auto result = true;

    // Subscription will capture only the stop notification.
    std::promise<code> promise;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](const code& ec, const messages::ping::cptr& ping) NOEXCEPT
        {
            result &= is_null(ping);
            promise.set_value(ec);
            return true;
        });
    });
    
    // Invalid object version will not cause a notification.
    const auto ping = system::to_chunk(system::to_little_endian(42));
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        constexpr uint32_t invalid_ping_version = 0;
        const auto ec = instance.notify(messages::identifier::ping, invalid_ping_version, ping);
        result &= (ec == error::invalid_message);
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

BOOST_AUTO_TEST_CASE(distributor__notify__valid_nonced_ping__expected_notification)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor instance(strand);
    constexpr uint64_t expected_nonce = 42;
    constexpr auto expected_ec = error::invalid_magic;
    auto result = true;

    // Subscription will capture message and stop notifications.
    std::promise<code> promise;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](const code& ec, const messages::ping::cptr& ping) NOEXCEPT
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
            return true;
        });
    });

    const auto ping = system::to_chunk(system::to_little_endian(expected_nonce));
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        constexpr auto nonced_ping_version = messages::level::bip31;
        const auto ec = instance.notify(messages::identifier::ping, nonced_ping_version, ping);
        result &= (ec == error::success);
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
