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

BOOST_AUTO_TEST_SUITE(resubscriber_tests)

typedef resubscriber<uint64_t, size_t> test_resubscriber;

BOOST_AUTO_TEST_CASE(resubscriber__subscribe__subscribed__subscriber_stopped)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_resubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    std::pair<code, size_t> stop_result;
    std::pair<code, size_t> resubscribe_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            stop_result = { value, size };
            return true;
        }, 0);

        instance.stop(ec, expected);

        instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            resubscribe_result = { value, size };
            return true;
        }, 0);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());

    BOOST_REQUIRE_EQUAL(stop_result.first, ec);
    BOOST_REQUIRE_EQUAL(stop_result.second, expected);
    BOOST_REQUIRE_EQUAL(resubscribe_result.first, error::subscriber_stopped);
    BOOST_REQUIRE_EQUAL(resubscribe_result.second, size_t{});
}

BOOST_AUTO_TEST_CASE(resubscriber__stop_default__once__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_resubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = zero;

    std::pair<code, size_t> stop_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            stop_result = { value, size };
            return true;
        }, 0);

        instance.stop_default(ec);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());

    BOOST_REQUIRE_EQUAL(stop_result.first, ec);
    BOOST_REQUIRE_EQUAL(stop_result.second, expected);
}

BOOST_AUTO_TEST_CASE(resubscriber__stop__once__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_resubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    std::pair<code, size_t> stop_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            stop_result = { value, size };
            return true;
        }, 0);

        instance.stop(ec, expected);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());

    BOOST_REQUIRE_EQUAL(stop_result.first, ec);
    BOOST_REQUIRE_EQUAL(stop_result.second, expected);
}

BOOST_AUTO_TEST_CASE(resubscriber__stop__twice__second_dropped)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_resubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    std::pair<code, size_t> stop_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            stop_result = { value, size };
            return true;
        }, 0);

        instance.stop(ec, expected);
        instance.stop(error::address_blocked, {});
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());

    // Handler is not invoked on stop after stop (handlers cleared).
    BOOST_REQUIRE_EQUAL(stop_result.first, ec);
    BOOST_REQUIRE_EQUAL(stop_result.second, expected);
}

BOOST_AUTO_TEST_CASE(resubscriber__notify__stopped__dropped)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_resubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto count = 0u;
    std::pair<code, size_t> notify_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            // Allow first and possible second notify, ignore stop.
            if (++count != two) notify_result = { value, size };
            return true;
        }, 0);

        instance.notify(ec, expected);
        instance.stop_default(error::address_blocked);
        instance.notify(error::address_blocked, {});
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());

    BOOST_REQUIRE_EQUAL(notify_result.first, ec);
    BOOST_REQUIRE_EQUAL(notify_result.second, expected);
}

BOOST_AUTO_TEST_CASE(resubscriber__notify__once__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_resubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto count = 0u;
    std::pair<code, size_t> notify_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            if (is_one(++count)) notify_result = { value, size };
            return true;
        }, 0);

        instance.notify(ec, expected);

        // Prevents unstopped assertion.
        instance.stop_default(error::address_blocked);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());

    BOOST_REQUIRE_EQUAL(notify_result.first, ec);
    BOOST_REQUIRE_EQUAL(notify_result.second, expected);
}

BOOST_AUTO_TEST_CASE(resubscriber__notify__twice__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_resubscriber instance(strand);
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto count = 0u;
    std::pair<code, size_t> notify_result;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](code value, size_t size) NOEXCEPT
        {
            if (++count == two) notify_result = { value, size };
            return true;
        }, 0);

        instance.notify({}, {});
        instance.notify(ec, expected);

        // Prevents unstopped assertion.
        instance.stop_default(error::address_blocked);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());

    BOOST_REQUIRE_EQUAL(notify_result.first, ec);
    BOOST_REQUIRE_EQUAL(notify_result.second, expected);
}

BOOST_AUTO_TEST_SUITE_END()
