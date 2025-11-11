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

#include <future>
#include <memory>

BOOST_AUTO_TEST_SUITE(distributor_rpc_tests)

using namespace json;

BOOST_AUTO_TEST_CASE(distributor_rpc__construct__stop__stops)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_rpc instance(strand);

    std::promise<bool> promise{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::service_stopped);
        promise.set_value(true);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(promise.get_future().get());
}

BOOST_AUTO_TEST_CASE(distributor_rpc__notify__unknown_method__returns_not_found)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_rpc instance(strand);

    std::promise<code> promise{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        request_t request{};
        request.method = "unknown_method";
        promise.set_value(instance.notify(request));
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(promise.get_future().get() == error::not_found);
}

BOOST_AUTO_TEST_SUITE_END()
