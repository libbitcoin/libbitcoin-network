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

BOOST_AUTO_TEST_SUITE(distributor_rpc_tests)

using namespace json;

static_assert( is_same_type<rpc::method<"test2">, rpc::method<"test2">>);
static_assert(!is_same_type<rpc::method<"test1">, rpc::method<"test2">>);
static_assert(!is_same_type<rpc::method<"test1", bool>, rpc::method<"test1", int>>);
static_assert(!is_same_type<rpc::method<"test1", bool>, rpc::method<"test2", bool>>);
static_assert( is_same_type<rpc::method<"test1", bool>, rpc::method<"test1", bool>>);

using get_version = std::tuple_element_t<0, decltype(bitcoind::methods)>::tag;
using add_element = std::tuple_element_t<1, decltype(bitcoind::methods)>::tag;

BOOST_AUTO_TEST_CASE(distributor_rpc__construct__stop__stops)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_rpc<bitcoind> instance(strand);

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

BOOST_AUTO_TEST_CASE(distributor_rpc__notify__no_subscriber__success)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_rpc<bitcoind> instance(strand);

    std::promise<code> promise{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        request_t request{};
        request.method = "get_version";
        const auto ec = instance.notify(request);
        promise.set_value(ec);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(distributor_rpc__notify__unknown_method__returns_not_found)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_rpc<bitcoind> instance(strand);

    std::promise<code> promise{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        request_t request{};
        request.method = "unknown_method";
        const auto ec = instance.notify(request);
        promise.set_value(ec);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::not_found);
}

BOOST_AUTO_TEST_CASE(distributor_rpc__subscribe__stopped__subscriber_stopped)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_rpc<bitcoind> instance(strand);
    constexpr auto expected_ec = error::subscriber_stopped;
    code subscribe_ec{};
    auto result = true;

    std::promise<code> promise{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::invalid_magic);
        subscribe_ec = instance.subscribe([&](const code& ec, add_element, int a, int b) NOEXCEPT
        {
            // Stop notification sets defaults and specified code.
            result &= is_zero(a);
            result &= is_zero(b);
            promise.set_value(ec);
            return false;
        });
    });

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::invalid_magic);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), expected_ec);
    BOOST_REQUIRE_EQUAL(subscribe_ec, expected_ec);
    BOOST_REQUIRE(result);
}

BOOST_AUTO_TEST_CASE(distributor_rpc__subscribe__stop__service_stopped)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_rpc<bitcoind> instance(strand);
    constexpr auto expected_ec = error::service_stopped;
    code subscribe_ec{};
    auto result = true;

    std::promise<code> promise;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        subscribe_ec = instance.subscribe([&](const code& ec, add_element, int a, int b) NOEXCEPT
        {
            // Stop notification sets defaults and specified code.
            result &= is_zero(a);
            result &= is_zero(b);
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
    BOOST_REQUIRE(!subscribe_ec);
    BOOST_REQUIRE(result);
}

////BOOST_AUTO_TEST_CASE(distributor_rpc__subscribe__multiple__both_return_success_no_invoke)
////{
////    threadpool pool(2);
////    asio::strand strand(pool.service().get_executor());
////    distributor_rpc<bitcoind> instance(strand);
////
////    bool first_called{};
////    bool second_called{};
////    std::promise<std::pair<code, code>> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        const auto ec1 = instance.subscribe(
////            [&](const code&, add_element, int, int) NOEXCEPT
////            {
////                first_called = true;
////                return true;
////            });
////
////        const auto ec2 = instance.subscribe(
////            [&](const code&, add_element, int, int) NOEXCEPT
////            {
////                second_called = true;
////                return true;
////            });
////
////        promise.set_value({ec1, ec2});
////    });
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////
////    const auto [ec1, ec2] = promise.get_future().get();
////    BOOST_REQUIRE(!ec1);
////    BOOST_REQUIRE(!ec2);
////    BOOST_REQUIRE(!first_called);
////    BOOST_REQUIRE(!second_called);
////}
////
////BOOST_AUTO_TEST_CASE(distributor_rpc__notify__multiple_subscribers__invokes_both)
////{
////    threadpool pool(2);
////    asio::strand strand(pool.service().get_executor());
////    distributor_rpc<bitcoind> instance(strand);
////
////    bool first_called{};
////    bool second_called{};
////    int first_result_a{};
////    int first_result_b{};
////    int second_result_a{};
////    int second_result_b{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe(
////            [&](const code&, add_element, int a, int b) NOEXCEPT
////            {
////                first_called = true;
////                first_result_a = a;
////                first_result_b = b;
////                return true;
////            });
////
////        instance.subscribe(
////            [&](const code&, add_element, int a, int b) NOEXCEPT
////            {
////                second_called = true;
////                second_result_a = a;
////                second_result_b = b;
////                return true;
////            });
////
////        request_t request{};
////        request.method = "add_element";
////        array_t params_array{ value_t{ 42.0 }, value_t{ 24.0 } };
////        request.params = params_t{ params_array };
////        const auto ec = instance.notify(request);
////        promise.set_value(ec);
////    });
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////    BOOST_REQUIRE(!promise.get_future().get());
////    BOOST_REQUIRE(first_called);
////    BOOST_REQUIRE(second_called);
////    BOOST_REQUIRE_EQUAL(first_result_a, 42);
////    BOOST_REQUIRE_EQUAL(first_result_b, 24);
////    BOOST_REQUIRE_EQUAL(second_result_a, 42);
////    BOOST_REQUIRE_EQUAL(second_result_b, 24);
////}
////
////BOOST_AUTO_TEST_CASE(distributor_rpc__stop__subscribed_handler__notifies_with_error_code)
////{
////    threadpool pool(2);
////    asio::strand strand(pool.service().get_executor());
////    distributor_rpc<bitcoind> instance(strand);
////
////    bool called{};
////    code result_ec{};
////    std::promise<bool> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe([&](const code& ec, get_version) NOEXCEPT
////        {
////            called = true;
////            result_ec = ec;
////            return false;
////        });
////
////        instance.stop(error::service_stopped);
////        promise.set_value(true);
////    });
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////    BOOST_REQUIRE(promise.get_future().get());
////    BOOST_REQUIRE(called);
////    BOOST_REQUIRE_EQUAL(result_ec, error::service_stopped);
////}
////
////BOOST_AUTO_TEST_CASE(distributor_rpc__notify__get_version_no_params__success_and_notifies)
////{
////    threadpool pool(2);
////    asio::strand strand(pool.service().get_executor());
////    distributor_rpc<bitcoind> instance(strand);
////
////    bool called{};
////    code result_ec{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe([&](const code& ec, get_version) NOEXCEPT
////        {
////            called = true;
////            result_ec = ec;
////            return true;
////        });
////
////        request_t request{};
////        request.method = "get_version";
////        const auto ec = instance.notify(request);
////        promise.set_value(ec);
////    });
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////    const auto notify_ec = promise.get_future().get();
////    BOOST_REQUIRE(!notify_ec);
////    BOOST_REQUIRE(called);
////    BOOST_REQUIRE(!result_ec);
////}
////
////BOOST_AUTO_TEST_CASE(distributor_rpc__notify__get_version_empty_array_params__success_and_notifies)
////{
////    threadpool pool(2);
////    asio::strand strand(pool.service().get_executor());
////    distributor_rpc<bitcoind> instance(strand);
////
////    bool called{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe([&](const code&, get_version) NOEXCEPT
////        {
////            called = true;
////            return true;
////        });
////
////        request_t request{};
////        request.method = "get_version";
////        request.params = params_t{ array_t{} };
////        const auto ec = instance.notify(request);
////        promise.set_value(ec);
////    });
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////    BOOST_REQUIRE(!promise.get_future().get());
////    BOOST_REQUIRE(called);
////}
////
////BOOST_AUTO_TEST_CASE(distributor_rpc__notify__get_version_non_empty_params__not_found_no_notify)
////{
////    threadpool pool(2);
////    asio::strand strand(pool.service().get_executor());
////    distributor_rpc<bitcoind> instance(strand);
////
////    bool called{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe([&](const code&, get_version) NOEXCEPT
////        {
////            called = true;
////            return true;
////        });
////
////        request_t request{};
////        request.method = "get_version";
////        array_t params_array{ value_t{ 1.0 } };
////        request.params = params_t{ params_array };
////        const auto ec = instance.notify(request);
////        promise.set_value(ec);
////    });
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::not_found);
////    BOOST_REQUIRE(!called);
////}
////
////BOOST_AUTO_TEST_CASE(distributor_rpc__notify__add_element_positional_params__success_and_notifies_with_args)
////{
////    threadpool pool(2);
////    asio::strand strand(pool.service().get_executor());
////    distributor_rpc<bitcoind> instance(strand);
////
////    bool called{};
////    int result_a{};
////    int result_b{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe(
////            [&](const code&, add_element, int a, int b) NOEXCEPT
////            {
////                called = true;
////                result_a = a;
////                result_b = b;
////                return true;
////            });
////
////        request_t request{};
////        request.method = "add_element";
////        array_t params_array{ value_t{ 42.0 }, value_t{ 24.0 } };
////        request.params = params_t{ params_array };
////        const auto ec = instance.notify(request);
////        promise.set_value(ec);
////    });
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////    BOOST_REQUIRE(!promise.get_future().get());
////    BOOST_REQUIRE(called);
////    BOOST_REQUIRE_EQUAL(result_a, 42);
////    BOOST_REQUIRE_EQUAL(result_b, 24);
////}
////
////BOOST_AUTO_TEST_CASE(distributor_rpc__notify__add_element_named_params__success_and_notifies_with_args)
////{
////    threadpool pool(2);
////    asio::strand strand(pool.service().get_executor());
////    distributor_rpc<bitcoind> instance(strand);
////
////    bool called{};
////    int result_a{};
////    int result_b{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe(
////            [&](const code&, add_element, int a, int b) NOEXCEPT
////            {
////                called = true;
////                result_a = a;
////                result_b = b;
////                return true;
////            });
////
////        request_t request{};
////        request.method = "add_element";
////        object_t params_object{ { "a", value_t{ 42.0 } }, { "b", value_t{ 24.0 } } };
////        request.params = params_t{ params_object };
////        const auto ec = instance.notify(request);
////        promise.set_value(ec);
////    });
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////    BOOST_REQUIRE(!promise.get_future().get());
////    BOOST_REQUIRE(called);
////    BOOST_REQUIRE_EQUAL(result_a, 42);
////    BOOST_REQUIRE_EQUAL(result_b, 24);
////}
////
////BOOST_AUTO_TEST_CASE(distributor_rpc__notify__add_element_wrong_param_count__not_found_no_notify)
////{
////    threadpool pool(2);
////    asio::strand strand(pool.service().get_executor());
////    distributor_rpc<bitcoind> instance(strand);
////
////    bool called{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe(
////            [&](const code&, add_element, int, int) NOEXCEPT
////            {
////                called = true;
////                return true;
////            });
////
////        request_t request{};
////        request.method = "add_element";
////        array_t params_array{ value_t{ 42.0 } };
////        request.params = params_t{ params_array };
////        const auto ec = instance.notify(request);
////        promise.set_value(ec);
////    });
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::not_found);
////    BOOST_REQUIRE(!called);
////}
////
////BOOST_AUTO_TEST_CASE(distributor_rpc__notify__add_element_invalid_type__not_found_no_notify)
////{
////    threadpool pool(2);
////    asio::strand strand(pool.service().get_executor());
////    distributor_rpc<bitcoind> instance(strand);
////
////    bool called{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe([&](const code&, add_element, int, int) NOEXCEPT
////        {
////            called = true;
////            return true;
////        });
////
////        request_t request{};
////        request.method = "add_element";
////        array_t params_array{ value_t{ string_t{ "invalid" } }, value_t{ 24.0 } };
////        request.params = params_t{ params_array };
////        const auto ec = instance.notify(request);
////        promise.set_value(ec);
////    });
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::not_found);
////    BOOST_REQUIRE(!called);
////}
////
////BOOST_AUTO_TEST_CASE(distributor_rpc__notify__add_element_non_integral_number__not_found_no_notify)
////{
////    threadpool pool(2);
////    asio::strand strand(pool.service().get_executor());
////    distributor_rpc<bitcoind> instance(strand);
////
////    bool called{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe(
////            [&](const code&, add_element, int, int) NOEXCEPT
////            {
////                called = true;
////                return true;
////            });
////
////        request_t request{};
////        request.method = "add_element";
////        array_t params_array{ value_t{ 42.5 }, value_t{ 24.0 } };
////        request.params = params_t{ params_array };
////        const auto ec = instance.notify(request);
////        promise.set_value(ec);
////    });
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::not_found);
////    BOOST_REQUIRE(!called);
////}
////
////BOOST_AUTO_TEST_CASE(distributor_rpc__notify__add_element_missing_named_param__not_found_no_notify)
////{
////    threadpool pool(2);
////    asio::strand strand(pool.service().get_executor());
////    distributor_rpc<bitcoind> instance(strand);
////
////    bool called{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe(
////            [&](const code&, add_element, int, int) NOEXCEPT
////            {
////                called = true;
////                return true;
////            });
////
////        request_t request{};
////        request.method = "add_element";
////        object_t params_object{ { "a", value_t{ 42.0 } } };
////        request.params = params_t{ params_object };
////        const auto ec = instance.notify(request);
////        promise.set_value(ec);
////    });
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::not_found);
////    BOOST_REQUIRE(!called);
////}

BOOST_AUTO_TEST_SUITE_END()
