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

BOOST_AUTO_TEST_SUITE(threadpool_tests)

BOOST_AUTO_TEST_CASE(threadpool__construct__default__unstopped)
{
    threadpool pool{};
    BOOST_REQUIRE(!pool.service().stopped());
}

BOOST_AUTO_TEST_CASE(threadpool__construct__empty__joins)
{
    threadpool pool{ 0, thread_priority::low };
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(!pool.service().stopped());
}

BOOST_AUTO_TEST_CASE(threadpool__service__always__defined)
{
    threadpool pool{ 2, thread_priority::lowest };
    pool.stop();
    BOOST_REQUIRE(pool.service().stopped());
}

BOOST_AUTO_TEST_CASE(threadpool__stop__always__stopped)
{
    threadpool pool{};
    pool.stop();
    BOOST_REQUIRE(pool.service().stopped());
}

BOOST_AUTO_TEST_CASE(threadpool__join__stopped__stopped)
{
    threadpool pool{};
    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(pool.service().stopped());
}

BOOST_AUTO_TEST_SUITE_END()
