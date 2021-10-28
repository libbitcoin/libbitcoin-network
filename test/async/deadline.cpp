/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network.hpp>

using namespace bc::network;

BOOST_AUTO_TEST_SUITE(deadline_tests)

BOOST_AUTO_TEST_CASE(deadline__construct1__one_thread_start_zero_delay__success)
{
    const auto handler = [](code ec)
    {
        BOOST_REQUIRE(ec == error::success);
    };

    threadpool pool(1);
    std::make_shared<deadline>(pool.service())->start(handler);
}

BOOST_AUTO_TEST_CASE(deadline__construct1__two_threads_start_delay__success)
{
    const auto handler = [](code ec)
    {
        BOOST_REQUIRE(ec == error::success);
    };

    threadpool pool(2);
    std::make_shared<deadline>(pool.service())->start(handler, milliseconds(1));
}

BOOST_AUTO_TEST_CASE(deadline__construct2__three_threads_start_zero_delay__success)
{
    const auto handler = [](code ec)
    {
        BOOST_REQUIRE(ec == error::success);
    };

    threadpool pool(3);
    std::make_shared<deadline>(pool.service(), seconds(42))->start(handler, seconds(0));
}

BOOST_AUTO_TEST_CASE(deadline__stop__thread_starved__not_invoked)
{
    // Thread starved timer.
    // ------------------------------------------------------------------------

    const auto handler = [](code ec)
    {
        // This should never be invoked (no threads).
        BOOST_REQUIRE(false);
    };

    threadpool pool(0);
    auto timer = std::make_shared<deadline>(pool.service());
    timer->start(handler);

    // Stop timer.
    // ------------------------------------------------------------------------

    const auto stop_handler = [&timer](code ec)
    {
        BOOST_REQUIRE(ec == error::success);
        timer->stop();
    };

    threadpool stop_pool(1);
    auto stopper = std::make_shared<deadline>(stop_pool.service(), milliseconds(1));
    stopper->start(stop_handler);
}

BOOST_AUTO_TEST_CASE(deadline__stop__race__success)
{
    // Slow timer.
    // ------------------------------------------------------------------------

    const auto handler = [](code ec)
    {
        // In the case of a race won by the slow timer, this catches success.
        // In the case of a race won by the stop timer, this will not fire.
        // A 10s delay indicates the slow timer has won the race (unexpected).
        BOOST_REQUIRE(ec == error::success);
    };

    threadpool pool(1);
    auto timer = std::make_shared<deadline>(pool.service(), seconds(10));
    timer->start(handler);

    // Stop timer.
    // ------------------------------------------------------------------------

    const auto stop_handler = [&timer](code ec)
    {
        BOOST_REQUIRE(ec == error::success);
        timer->stop();
    };

    threadpool stop_pool(1);
    auto stopper = std::make_shared<deadline>(stop_pool.service());
    stopper->start(stop_handler, milliseconds(1));
}

BOOST_AUTO_TEST_SUITE_END()
