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

BOOST_AUTO_TEST_SUITE(distributor_peer_tests)

using namespace rpc;
using namespace messages::peer;

struct mock_methods
{
    static constexpr std::tuple methods
    {
        method<"ping", ping::cptr>{ "message" }
    };

    using ping = at<0, decltype(methods)>;
};

using mock = interface<mock_methods>;
using distributor_mock = dispatcher<mock>;

BOOST_AUTO_TEST_CASE(distributor_peer__notify__ping_positional__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_mock instance(strand);

    bool called{};
    ping::cptr result{};
    std::promise<code> promise1{};
    std::promise<code> promise2{};
    constexpr auto expected = 42u;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe(
            [&](code ec, ping::cptr ptr) NOEXCEPT
            {
                // Avoid stop notification (unavoidable test condition).
                if (called)
                    return false;

                result = ptr;
                called = true;
                promise2.set_value(ec);
                return true;
            });

        promise1.set_value(instance.notify(
        {
            .method = "ping",
            .params = { array_t{ system::to_shared<ping>(expected) } }
        }));
    });

    BOOST_REQUIRE(!promise1.get_future().get());
    BOOST_REQUIRE(!promise2.get_future().get());
    BOOST_REQUIRE(result);
    BOOST_REQUIRE(result->id == identifier::ping);
    BOOST_REQUIRE_EQUAL(result->nonce, expected);

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::service_stopped);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(distributor_peer__notify__ping_named__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_mock instance(strand);

    bool called{};
    ping::cptr result{};
    std::promise<code> promise1{};
    std::promise<code> promise2{};
    constexpr auto expected = 42u;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe(
            [&](const code& ec, const ping::cptr& ptr) NOEXCEPT
            {
                // Avoid stop notification (unavoidable test condition).
                if (called)
                    return false;

                result = ptr;
                called = true;
                promise2.set_value(ec);
                return true;
            });

        promise1.set_value(instance.notify(
        {
            .method = "ping",
            .params = { object_t{ { "message", system::to_shared<ping>(expected) } } }
        }));
    });

    BOOST_REQUIRE(!promise1.get_future().get());
    BOOST_REQUIRE(!promise2.get_future().get());
    BOOST_REQUIRE(result);
    BOOST_REQUIRE(result->id == identifier::ping);
    BOOST_REQUIRE_EQUAL(result->nonce, expected);

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::service_stopped);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_SUITE_END()
