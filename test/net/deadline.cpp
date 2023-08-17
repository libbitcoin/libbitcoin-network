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

BOOST_AUTO_TEST_SUITE(deadline_tests)

BOOST_AUTO_TEST_CASE(deadline__construct1__one_thread_start_zero_delay__success)
{
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const auto timer = std::make_shared<deadline>(log, strand);
    timer->start([timer](code ec)
    {
        BOOST_REQUIRE(timer && !ec);
    });
}

BOOST_AUTO_TEST_CASE(deadline__construct1__two_threads_start_delay__success)
{
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    const auto timer = std::make_shared<deadline>(log, strand);
    timer->start([timer](code ec)
    {
        BOOST_REQUIRE(timer && !ec);
    }, milliseconds(1));
}

BOOST_AUTO_TEST_CASE(deadline__construct2__three_threads_start_zero_delay__success)
{
    const logger log{};
    threadpool pool(3);
    asio::strand strand(pool.service().get_executor());
    const auto timer = std::make_shared<deadline>(log, strand, seconds(42));
    timer->start([timer](code ec)
    {
        BOOST_REQUIRE(timer && !ec);
    }, seconds(0));
}

BOOST_AUTO_TEST_CASE(deadline__stop__thread_starved__not_invoked)
{
    // Thread starved timer.
    const logger log{};
    threadpool pool(0);
    asio::strand strand(pool.service().get_executor());
    const auto timer = std::make_shared<deadline>(log, strand);
    timer->start([timer](code)
    {
        // This should never be invoked (no threads).
        BOOST_REQUIRE(!timer);
    });

    // Stop timer.
    threadpool stop_pool(1);
    asio::strand stop_strand(stop_pool.service().get_executor());
    const auto stopper = std::make_shared<deadline>(log, stop_strand, milliseconds(1));
    stopper->start([timer, stopper](code ec)
    {
        BOOST_REQUIRE(stopper && !ec);
        timer->stop();
    });
}

BOOST_AUTO_TEST_CASE(deadline__stop__race__success)
{
    // Slow timer.
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const auto timer = std::make_shared<deadline>(log, strand, seconds(10));
    timer->start([timer](code ec)
    {
        // In the case of a race won by the slow timer, this catches success.
        // In the case of a race won by the stop timer, this catches canceled.
        // A 10s delay indicates the slow timer has won the race (unexpected).
        BOOST_REQUIRE(timer && ec == error::operation_canceled);
    });

    // Stop timer.
    threadpool stop_pool(1);
    asio::strand stop_strand(stop_pool.service().get_executor());
    const auto stopper = std::make_shared<deadline>(log, stop_strand);
    stopper->start([timer, stopper](code ec)
    {
        BOOST_REQUIRE(stopper && !ec);
        timer->stop();
    }, milliseconds(1));
}

BOOST_AUTO_TEST_SUITE_END()
