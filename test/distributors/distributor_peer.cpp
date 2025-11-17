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

BOOST_AUTO_TEST_SUITE(distributor_peer_tests)

using namespace rpc;
using namespace messages::peer;

struct mock_methods
{
    static constexpr std::tuple methods
    {
        method<"ping", ping::cptr>{ "message" }
    };

    using ping = at<0, decltype(methods)>;
};

using mock = interface<mock_methods>;
using distributor_mock = distributor_rpc<mock>;

BOOST_AUTO_TEST_CASE(distributor_peer__notify__ping_positional__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_mock instance(strand);

    bool called{};
    ping::cptr result{};
    std::promise<code> promise1{};
    std::promise<code> promise2{};
    constexpr auto expected = 42u;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe(
            [&](const code& ec, mock::ping, const ping::cptr& ptr) NOEXCEPT
            {
                // Avoid stop notification (unavoidable test condition).
                if (called)
                    return false;

                result = ptr;
                called = true;
                promise2.set_value(ec);
                return true;
            });

        promise1.set_value(instance.notify(
        {
            .method = "ping",
            .params = { array_t{ system::to_shared<ping>(expected) } }
        }));
    });

    BOOST_REQUIRE(!promise1.get_future().get());
    BOOST_REQUIRE(!promise2.get_future().get());
    BOOST_REQUIRE(result);
    BOOST_REQUIRE(result->id == identifier::ping);
    BOOST_REQUIRE_EQUAL(result->nonce, expected);

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::service_stopped);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(distributor_peer__notify__ping_named__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_mock instance(strand);

    bool called{};
    ping::cptr result{};
    std::promise<code> promise1{};
    std::promise<code> promise2{};
    constexpr auto expected = 42u;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe(
            [&](const code& ec, mock::ping, const ping::cptr& ptr) NOEXCEPT
            {
                // Avoid stop notification (unavoidable test condition).
                if (called)
                    return false;

                result = ptr;
                called = true;
                promise2.set_value(ec);
                return true;
            });

        promise1.set_value(instance.notify(
        {
            .method = "ping",
            .params = { object_t{ { "message", system::to_shared<ping>(expected) } } }
        }));
    });

    BOOST_REQUIRE(!promise1.get_future().get());
    BOOST_REQUIRE(!promise2.get_future().get());
    BOOST_REQUIRE(result);
    BOOST_REQUIRE(result->id == identifier::ping);
    BOOST_REQUIRE_EQUAL(result->nonce, expected);

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::service_stopped);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
}

// old school peer
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(distributor_peer__construct__stop__stops)
{
    default_memory memory{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_peer instance(memory, strand);

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

BOOST_AUTO_TEST_CASE(distributor_peer__subscribe__stop__expected_code)
{
    default_memory memory{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_peer instance(memory, strand);
    constexpr auto expected_ec = error::invalid_magic;
    auto result = true;

    std::promise<code> promise;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](const code& ec, const messages::peer::ping::cptr& ping) NOEXCEPT
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

BOOST_AUTO_TEST_CASE(distributor_peer__notify__invalid_message__no_notification)
{
    default_memory memory{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_peer instance(memory, strand);
    constexpr auto expected_ec = error::invalid_magic;
    auto result = true;

    // Subscription will capture only the stop notification.
    std::promise<code> promise;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](const code& ec, const messages::peer::ping::cptr& ping) NOEXCEPT
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
        constexpr auto nonced_ping_version = messages::peer::level::bip31;

        // This line throws and is caught internal to the low level stream.
        const auto ec = instance.notify(messages::peer::identifier::ping, nonced_ping_version, empty);
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

BOOST_AUTO_TEST_CASE(distributor_peer__notify__valid_message_invalid_version__no_notification)
{
    default_memory memory{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_peer instance(memory, strand);
    constexpr auto expected_ec = error::invalid_magic;
    auto result = true;

    // Subscription will capture only the stop notification.
    std::promise<code> promise;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](const code& ec, const messages::peer::ping::cptr& ping) NOEXCEPT
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
        const auto ec = instance.notify(messages::peer::identifier::ping, invalid_ping_version, ping);
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

BOOST_AUTO_TEST_CASE(distributor_peer__notify__valid_nonced_ping__expected_notification)
{
    default_memory memory{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_peer instance(memory, strand);
    constexpr uint64_t expected_nonce = 42;
    constexpr auto expected_ec = error::invalid_magic;
    auto result = true;

    // Subscription will capture message and stop notifications.
    std::promise<code> promise;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](const code& ec, const messages::peer::ping::cptr& ping) NOEXCEPT
        {
            // Avoid stop notification (unavoidable test condition).
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
        constexpr auto nonced_ping_version = messages::peer::level::bip31;
        const auto ec = instance.notify(messages::peer::identifier::ping, nonced_ping_version, ping);
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
