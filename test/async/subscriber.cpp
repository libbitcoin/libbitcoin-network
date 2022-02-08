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

typedef subscriber<bool, size_t> test_subscriber;

BOOST_AUTO_TEST_CASE(subscriber__stopped__not_stop__false)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_subscriber instance(strand);

    boost::asio::post(strand, [&]()
    {
        BOOST_REQUIRE(!instance.stopped());
        instance.stop(false, 0);
    });

    pool.stop();
    pool.join();
}

BOOST_AUTO_TEST_CASE(subscriber__stopped__stop__true)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_subscriber instance(strand);

    boost::asio::post(strand, [&]()
    {
            instance.stop(false, 0);
            BOOST_REQUIRE(instance.stopped());
    });

    pool.stop();
    pool.join();
}

BOOST_AUTO_TEST_CASE(subscriber__subscribe__stop__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_subscriber instance(strand);

    std::pair<bool, size_t> result;
    const auto handler = [&](bool value, size_t size)
    {
        result = { value, size };
    };

    const auto expected1 = true;
    const auto expected2 = 42u;

    boost::asio::post(strand, [&]()
    {
        instance.subscribe(handler);
        instance.stop(expected1, expected2);
    });

    pool.stop();
    pool.join();

    BOOST_REQUIRE_EQUAL(result.first, expected1);
    BOOST_REQUIRE_EQUAL(result.second, expected2);
}

BOOST_AUTO_TEST_CASE(subscriber__subscribe__notify__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    test_subscriber instance(strand);

    std::promise<std::pair<bool, size_t>> promise;
    const auto handler = [&](bool value, size_t size)
    {
        promise.set_value({ value, size });
    };

    const auto expected1 = true;
    const auto expected2 = 42u;

    // Subscribe and notify.
    boost::asio::post(strand, [&]()
    {
        instance.subscribe(handler);
        instance.notify(expected1, expected2);
    });

    // Wait until notification handler capture is invoked.
    const auto result = promise.get_future().get();

    BOOST_REQUIRE_EQUAL(result.first, expected1);
    BOOST_REQUIRE_EQUAL(result.second, expected2);

    // Reset promise to allow stop notification.
    promise = {};

    // Stop to prevent destructor assertion.
    boost::asio::post(strand, [&]()
    {
        instance.stop(false, 0);
    });

    pool.stop();
    pool.join();
}

BOOST_AUTO_TEST_SUITE_END()
