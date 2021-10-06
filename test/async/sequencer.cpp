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
#include <chrono>
#include <thread>
#include <bitcoin/network.hpp>

using namespace bc::network;

BOOST_AUTO_TEST_SUITE(sequencer_tests)

BOOST_AUTO_TEST_CASE(sequencer__lock__once__invoked)
{
    threadpool pool(1);
    sequencer instance(pool.service());
    std::promise<code> promise;
    const auto handler = [&]()
    {
        instance.unlock();
        promise.set_value(error::unknown);
    };
    instance.lock(handler);
    BOOST_REQUIRE(promise.get_future().get().value() == error::unknown);
}

BOOST_AUTO_TEST_CASE(sequencer__lock__twice__both_invoked)
{
    auto order = 0;
    threadpool pool(2);
    sequencer instance(pool.service());

    std::promise<int> promise1;
    const auto handler1 = [&]()
    {
        // Delay on this pool thread while the other attempts to execute.
        std::this_thread::sleep_for(milliseconds(10));

        promise1.set_value(order++);
        instance.unlock();
    };

    std::promise<int> promise2;
    const auto handler2 = [&]()
    {
        promise2.set_value(order++);
        instance.unlock();
    };

    // Locked handler invocation is asynchronous.
    instance.lock(handler1);
    instance.lock(handler2);

    // If execution is not blocked for sleep, then these values would reverse.
    // This relies on a (reasonable) assumption about the race condition.
    BOOST_REQUIRE_EQUAL(promise1.get_future().get(), 0);
    BOOST_REQUIRE_EQUAL(promise2.get_future().get(), 1);
    BOOST_REQUIRE_EQUAL(order, 2);
}

BOOST_AUTO_TEST_SUITE_END()
