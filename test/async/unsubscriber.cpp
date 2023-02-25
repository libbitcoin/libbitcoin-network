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

BOOST_AUTO_TEST_SUITE(unsubscriber_tests)

typedef unsubscriber<size_t> test_unsubscriber;

BOOST_AUTO_TEST_CASE(unsubscriber__subscribe__stopped__subscriber_stopped)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_unsubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto result = true;
    std::pair<code, size_t> stop_result;
    std::pair<code, size_t> retry_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        result &= is_zero(instance.size());
        result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            stop_result = { value, size };
            return true;
        });
    
        result &= is_one(instance.size());
        instance.stop(ec, expected);

        result &= is_zero(instance.size());
        result &= (instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            retry_result = { value, size };
            return true;
        }) == error::subscriber_stopped);

        result &= is_zero(instance.size());
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(stop_result.first, ec);
    BOOST_REQUIRE_EQUAL(stop_result.second, expected);
    BOOST_REQUIRE_EQUAL(retry_result.first, error::subscriber_stopped);
    BOOST_REQUIRE_EQUAL(retry_result.second, size_t{});
}

BOOST_AUTO_TEST_CASE(unsubscriber__subscribe__exists__success)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_unsubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto result = true;
    std::pair<code, size_t> first_result;
    std::pair<code, size_t> second_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            first_result = { value, size };
            return true;
        });

        result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            second_result = { value, size };
            return true;
        });

        instance.stop(ec, expected);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(first_result.first, ec);
    BOOST_REQUIRE_EQUAL(first_result.second, expected);
    BOOST_REQUIRE_EQUAL(second_result.first, ec);
    BOOST_REQUIRE_EQUAL(second_result.second, expected);
}

BOOST_AUTO_TEST_CASE(unsubscriber__subscribe__removed__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_unsubscriber instance(strand);
    const auto ec1 = error::address_not_found;
    const auto ec2 = error::address_in_use;
    constexpr auto expected1 = 42u;
    constexpr auto expected2 = 24u;

    auto result = true;
    std::pair<code, size_t> first_result;
    std::pair<code, size_t> second_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        result &= is_zero(instance.size());
        result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            first_result = { value, size };
            return false;
        });

        instance.notify(ec1, expected1);

        result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            second_result = { value, size };
            return true;
        });

        result &= is_one(instance.size());
        instance.stop(ec2, expected2);
        result &= is_zero(instance.size());
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(first_result.first, ec1);
    BOOST_REQUIRE_EQUAL(first_result.second, expected1);
    BOOST_REQUIRE_EQUAL(second_result.first, ec2);
    BOOST_REQUIRE_EQUAL(second_result.second, expected2);
}

BOOST_AUTO_TEST_CASE(unsubscriber__subscribe__unique__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_unsubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto result = true;
    std::pair<code, size_t> first_result;
    std::pair<code, size_t> second_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        result &= is_zero(instance.size());
        result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            first_result = { value, size };
            return true;
        });

        result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            second_result = { value, size };
            return true;
        });

        result &= (instance.size() == two);
        instance.stop(ec, expected);
        result &= is_zero(instance.size());
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(first_result.first, ec);
    BOOST_REQUIRE_EQUAL(first_result.second, expected);
    BOOST_REQUIRE_EQUAL(second_result.first, ec);
    BOOST_REQUIRE_EQUAL(second_result.second, expected);
}

BOOST_AUTO_TEST_CASE(unsubscriber__stop_default__once__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_unsubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = zero;

    auto result = true;
    std::pair<code, size_t> stop_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            stop_result = { value, size };
            return true;
        });

        instance.stop_default(ec);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(stop_result.first, ec);
    BOOST_REQUIRE_EQUAL(stop_result.second, expected);
}

BOOST_AUTO_TEST_CASE(unsubscriber__stop__once__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_unsubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto result = true;
    std::pair<code, size_t> stop_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            stop_result = { value, size };
            return true;
        });

        instance.stop(ec, expected);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(stop_result.first, ec);
    BOOST_REQUIRE_EQUAL(stop_result.second, expected);
}

BOOST_AUTO_TEST_CASE(unsubscriber__stop__twice__second_dropped)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_unsubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto result = true;
    std::pair<code, size_t> stop_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            stop_result = { value, size };
            return true;
        });

        instance.stop(ec, expected);
        instance.stop(error::address_blocked, {});
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(result);

    // Handler is not invoked on stop after stop (handlers cleared).
    BOOST_REQUIRE_EQUAL(stop_result.first, ec);
    BOOST_REQUIRE_EQUAL(stop_result.second, expected);
}

BOOST_AUTO_TEST_CASE(unsubscriber__notify__stopped__dropped)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_unsubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto result = true;
    auto count = zero;
    std::pair<code, size_t> notify_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            // Allow first and possible second notify, ignore stop.
            if (++count != two) notify_result = { value, size };
            return true;
        });

        instance.notify(ec, expected);
        instance.stop_default(error::address_blocked);
        instance.notify(error::address_blocked, {});
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(notify_result.first, ec);
    BOOST_REQUIRE_EQUAL(notify_result.second, expected);
}

BOOST_AUTO_TEST_CASE(unsubscriber__notify__once__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_unsubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto result = true;
    auto count = 0u;
    std::pair<code, size_t> notify_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            if (is_one(++count)) notify_result = { value, size };
            return true;
        });

        instance.notify(ec, expected);

        // Prevents unstopped assertion (uncleared).
        instance.stop_default(error::address_blocked);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(notify_result.first, ec);
    BOOST_REQUIRE_EQUAL(notify_result.second, expected);
}

BOOST_AUTO_TEST_CASE(unsubscriber__notify__twice_true__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_unsubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto result = true;
    auto count = zero;
    std::pair<code, size_t> notify_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        result = !instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            // Exclude stop_default call.
            if (++count <= two) notify_result = { value, size };
            return true;
        });

        instance.notify({}, {});
        instance.notify(ec, expected);

        // Prevents unstopped assertion (uncleared).
        instance.stop_default(error::address_blocked);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(notify_result.first, ec);
    BOOST_REQUIRE_EQUAL(notify_result.second, expected);
}

BOOST_AUTO_TEST_CASE(unsubscriber__notify__twice_false__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_unsubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto result = true;
    std::pair<code, size_t> notify_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            notify_result = { value, size };
            return false;
        });

        instance.notify(ec, expected);
        instance.notify({}, {});

        // Cleared by false return.
        ////instance.stop_default(error::address_blocked);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(notify_result.first, ec);
    BOOST_REQUIRE_EQUAL(notify_result.second, expected);
}

BOOST_AUTO_TEST_SUITE_END()
