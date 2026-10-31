/**
 * Copyright (c) 2011-2026 libbitcoin developers
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

BC_AUTO_TEST_SUITE(deadline_tests)

BC_AUTO_TEST_CASE(deadline__construct1__one_thread_start_zero_delay__success)
{
    const logger log{};
    threadpool pool(1);
    io::strand strand(pool.service().get_executor());
    const auto timer = std::make_shared<deadline>(log, strand);
    timer->start([timer](code ec)
    {
        BC_REQUIRE(timer && !ec);
    });
}

BC_AUTO_TEST_CASE(deadline__construct1__two_threads_start_delay__success)
{
    const logger log{};
    threadpool pool(2);
    io::strand strand(pool.service().get_executor());
    const auto timer = std::make_shared<deadline>(log, strand);
    timer->start([timer](code ec)
    {
        BC_REQUIRE(timer && !ec);
    }, milliseconds(1));
}

BC_AUTO_TEST_CASE(deadline__construct2__three_threads_start_zero_delay__success)
{
    const logger log{};
    threadpool pool(3);
    io::strand strand(pool.service().get_executor());
    const auto timer = std::make_shared<deadline>(log, strand, seconds(42));
    timer->start([timer](code ec)
    {
        BC_REQUIRE(timer && !ec);
    }, seconds(0));
}

BC_AUTO_TEST_CASE(deadline__stop__thread_starved__not_invoked)
{
    // Thread starved timer.
    const logger log{};
    threadpool pool(0);
    io::strand strand(pool.service().get_executor());
    const auto timer = std::make_shared<deadline>(log, strand);
    timer->start([timer](code)
    {
        // This should never be invoked (no threads).
        BC_REQUIRE(!timer);
    });

    // Stop timer.
    threadpool stop_pool(1);
    io::strand stop_strand(stop_pool.service().get_executor());
    const auto stopper = std::make_shared<deadline>(log, stop_strand, milliseconds(1));
    stopper->start([timer, stopper](code ec)
    {
        BC_REQUIRE(stopper && !ec);
        timer->stop();
    });
}

BC_AUTO_TEST_CASE(deadline__stop__race__success)
{
    // Slow timer.
    const logger log{};
    threadpool pool(1);
    io::strand strand(pool.service().get_executor());
    const auto timer = std::make_shared<deadline>(log, strand, seconds(10));
    timer->start([timer](code ec)
    {
        // In the case of a race won by the slow timer, this catches success.
        // In the case of a race won by the stop timer, this catches canceled.
        // A 10s delay indicates the slow timer has won the race (unexpected).
        BC_REQUIRE(timer && ec == error::operation_canceled);
    });

    // Stop timer.
    threadpool stop_pool(1);
    io::strand stop_strand(stop_pool.service().get_executor());
    const auto stopper = std::make_shared<deadline>(log, stop_strand);
    stopper->start([timer, stopper](code ec)
    {
        BC_REQUIRE(stopper && !ec);
        timer->stop();
    }, milliseconds(1));
}

BC_AUTO_TEST_CASE(deadline__remaining__started__within_timeout)
{
    const logger log{};
    threadpool pool(1);
    io::strand strand(pool.service().get_executor());
    const auto timer = std::make_shared<deadline>(log, strand, seconds(100));
    std::promise<code> fired{};
    std::promise<seconds> remaining{};
    io::post(strand, [&]() NOEXCEPT
    {
        timer->start([&](code ec) NOEXCEPT
        {
            fired.set_value(ec);
        });

        remaining.set_value(timer->remaining());
        timer->stop();
    });

    const auto left = remaining.get_future().get();
    BC_REQUIRE_EQUAL(fired.get_future().get(), error::operation_canceled);
    BC_REQUIRE_LE(left, seconds(100));
    BC_REQUIRE_GT(left, seconds(90));
    pool.stop();
    BC_REQUIRE(pool.join());
}

BC_AUTO_TEST_SUITE_END()
