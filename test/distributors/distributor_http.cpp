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

BOOST_AUTO_TEST_SUITE(distributor_http_tests)

using namespace http;

BOOST_AUTO_TEST_CASE(distributor_http__construct__stop__stops)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_http instance(strand);

    std::promise<bool> promise;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::service_stopped);
        promise.set_value(true);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(promise.get_future().get());
}

BOOST_AUTO_TEST_CASE(distributor_http__subscribe__stop__expected_code)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_http instance(strand);
    constexpr auto expected_ec = error::invalid_magic;
    auto result = true;

    std::promise<code> promise;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](const code& ec, const method::get& request) NOEXCEPT
        {
            // Stop notification has nullptr message and specified code.
            result = request;
            promise.set_value(ec);
            return true;
        });
    });

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(expected_ec);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), expected_ec);
    BOOST_REQUIRE(!result);
}

BOOST_AUTO_TEST_CASE(distributor_http__notify__null_message__null_unknown_with_operation_failed)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_http instance(strand);
    auto result = true;

    bool set{};
    std::promise<code> promise{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](const code& ec, const method::unknown& request) NOEXCEPT
        {
            // Skip stop notification (unavoidable test condition).
            if (!set)
            {
                result = request;
                promise.set_value(ec);
                set = true;
            }

            return true;
        });
    });

    // Notify with null request.
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.notify({});
    });

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::invalid_magic);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::operation_failed);
    BOOST_REQUIRE(!result);
}

BOOST_AUTO_TEST_CASE(distributor_http__notify__get_message__expected_method)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_http instance(strand);
    auto result = true;

    bool set{};
    std::promise<code> promise{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](const code& ec, const method::get& request) NOEXCEPT
        {
            // Skip stop notification (unavoidable test condition).
            if (!set)
            {
                result = (request->method() == verb::get);
                promise.set_value(ec);
                set = true;
            }

            return true;
        });
    });

    // Notify with get request.
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.notify(std::make_shared<request>(verb::get, "/", 11));
    });

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::invalid_magic);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::success);
    BOOST_REQUIRE(result);
}

BOOST_AUTO_TEST_SUITE_END()
