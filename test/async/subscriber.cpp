/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(subscriber_tests)

typedef subscriber<size_t> test_subscriber;

BOOST_AUTO_TEST_CASE(subscriber__subscribe__subscribed__subscriber_stopped)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_subscriber instance(strand);
    const auto ec = error::address_not_found;
    const auto expected = 42u;

    std::pair<code, size_t> stop_result;
    std::pair<code, size_t> resubscribe_result;
    boost::asio::post(strand, [&]()
    {
        instance.subscribe([&](code value, size_t size)
        {
            stop_result = { value, size };
        });

        instance.stop(ec, expected);

        instance.subscribe([&](code value, size_t size)
        {
            resubscribe_result = { value, size };
        });
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());

    BOOST_REQUIRE_EQUAL(stop_result.first, ec);
    BOOST_REQUIRE_EQUAL(stop_result.second, expected);
    BOOST_REQUIRE_EQUAL(resubscribe_result.first, error::subscriber_stopped);
    BOOST_REQUIRE_EQUAL(resubscribe_result.second, size_t{});
}

BOOST_AUTO_TEST_CASE(subscriber__stop_default__once__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_subscriber instance(strand);
    const auto ec = error::address_not_found;
    const auto expected = size_t{};

    std::pair<code, size_t> stop_result;
    boost::asio::post(strand, [&]()
    {
        instance.subscribe([&](code value, size_t size)
        {
            stop_result = { value, size };
        });

        instance.stop_default(ec);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());

    BOOST_REQUIRE_EQUAL(stop_result.first, ec);
    BOOST_REQUIRE_EQUAL(stop_result.second, expected);
}

BOOST_AUTO_TEST_CASE(subscriber__stop__once__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_subscriber instance(strand);
    const auto ec = error::address_not_found;
    const auto expected = 42u;

    std::pair<code, size_t> stop_result;
    boost::asio::post(strand, [&]()
    {
        instance.subscribe([&](code value, size_t size)
        {
            stop_result = { value, size };
        });

        instance.stop(ec, expected);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());

    BOOST_REQUIRE_EQUAL(stop_result.first, ec);
    BOOST_REQUIRE_EQUAL(stop_result.second, expected);
}

BOOST_AUTO_TEST_CASE(subscriber__stop__twice__second_dropped)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_subscriber instance(strand);
    const auto ec = error::address_not_found;
    const auto expected = 42u;

    std::pair<code, size_t> stop_result;
    boost::asio::post(strand, [&]()
    {
        instance.subscribe([&](code value, size_t size)
        {
            stop_result = { value, size };
        });

        instance.stop(ec, expected);
        instance.stop(error::address_blocked, {});
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());

    // Handler is not invoked on stop after stop (handlers cleared).
    BOOST_REQUIRE_EQUAL(stop_result.first, ec);
    BOOST_REQUIRE_EQUAL(stop_result.second, expected);
}

BOOST_AUTO_TEST_CASE(subscriber__notify__stopped__dropped)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_subscriber instance(strand);
    const auto ec = error::address_not_found;
    const auto expected = 42u;

    auto count = 0u;
    std::pair<code, size_t> notify_result;
    boost::asio::post(strand, [&]()
    {
        instance.subscribe([&](code value, size_t size)
        {
            // Allow first and possible second notify, ignore stop.
            if (++count != 2u)
                notify_result = { value, size };
        });

        instance.notify(ec, expected);
        instance.stop_default(error::address_blocked);
        instance.notify(error::address_blocked, {});
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());

    BOOST_REQUIRE_EQUAL(notify_result.first, ec);
    BOOST_REQUIRE_EQUAL(notify_result.second, expected);
}

BOOST_AUTO_TEST_CASE(subscriber__notify__once__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_subscriber instance(strand);
    const auto ec = error::address_not_found;
    const auto expected = 42u;

    auto count = 0u;
    std::pair<code, size_t> notify_result;
    boost::asio::post(strand, [&]()
    {
        instance.subscribe([&](code value, size_t size)
        {
            if (is_one(++count))
                notify_result = { value, size };
        });

        instance.notify(ec, expected);

        // Prevents unstopped assertion.
        instance.stop_default(error::address_blocked);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());

    BOOST_REQUIRE_EQUAL(notify_result.first, ec);
    BOOST_REQUIRE_EQUAL(notify_result.second, expected);
}

BOOST_AUTO_TEST_CASE(subscriber__notify__twice__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_subscriber instance(strand);
    const auto ec = error::address_not_found;
    const auto expected = 42u;

    auto count = 0u;
    std::pair<code, size_t> notify_result;
    boost::asio::post(strand, [&]()
    {
        instance.subscribe([&](code value, size_t size)
        {
            if (++count == 2u)
                notify_result = { value, size };
        });

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
