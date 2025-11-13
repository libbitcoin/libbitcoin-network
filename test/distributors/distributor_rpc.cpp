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

using namespace rpc;

// TODO: move to messages/json/rpc tests.
template <typename Type>
using names_t = typename parameter_names<Type>::type;
static_assert(is_same_type<names_t<method<"foo", bool, double>>, std::array<std::string, 2>>);
static_assert(is_same_type<names_t<method<"bar">>, std::array<std::string, 0>>);
static_assert(is_same_type<names_t<std::tuple<bool, double>>, std::array<std::string, 2>>);
static_assert(is_same_type<names_t<std::tuple<>>, std::array<std::string, 0>>);

// TODO: move to messages/json/rpc tests.
static_assert( is_same_type<method<"test2">, method<"test2">>);
static_assert(!is_same_type<method<"test1">, method<"test2">>);
static_assert(!is_same_type<method<"test1", bool>, method<"test1", int>>);
static_assert(!is_same_type<method<"test1", bool>, method<"test2", bool>>);
static_assert( is_same_type<method<"test1", bool>, method<"test1", bool>>);

// interface requires `type` (type) and `methods`, `size`, `mode` (value).
struct mock
{
    static constexpr std::tuple methods
    {
        method<"get_version">{},
        method<"add_element", bool, double, std::string>{ "a", "b", "c" }
    };

    using type = decltype(methods);
    static constexpr auto size = std::tuple_size_v<type>;
    static constexpr group mode = group::either;
};

using get_version = std::tuple_element_t<0, decltype(mock::methods)>::tag;
using add_element = std::tuple_element_t<1, decltype(mock::methods)>::tag;
using distributor_mock = distributor_rpc<mock>;

BOOST_AUTO_TEST_CASE(distributor_rpc__construct__stop__stops)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_mock instance(strand);

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
    distributor_mock instance(strand);

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
    distributor_mock instance(strand);

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
    distributor_mock instance(strand);
    constexpr auto expected_ec = error::subscriber_stopped;
    code subscribe_ec{};
    auto result = true;

    std::promise<code> promise{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::invalid_magic);
        subscribe_ec = instance.subscribe(
            [&](const code& ec, add_element, bool a, double b, std::string c) NOEXCEPT
            {
                // Stop notification sets defaults and specified code.
                result &= is_zero(a);
                result &= is_zero(b);
                result &= c.empty();
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
    distributor_mock instance(strand);
    constexpr auto expected_ec = error::service_stopped;
    code subscribe_ec{};
    auto result = true;

    std::promise<code> promise;
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        subscribe_ec = instance.subscribe(
            [&](const code& ec, add_element, bool a, double b, std::string c) NOEXCEPT
            {
                // Stop notification sets defaults and specified code.
                result &= is_zero(a);
                result &= is_zero(b);
                result &= c.empty();
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

BOOST_AUTO_TEST_CASE(distributor_rpc__subscribe__multiple__both_return_success_no_invoke)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_mock instance(strand);

    bool first_called{};
    bool second_called{};
    std::promise<std::pair<code, code>> promise{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        const auto ec1 = instance.subscribe(
            [&](const code&, add_element, bool, double, std::string) NOEXCEPT
            {
                first_called = true;
                return true;
            });

        const auto ec2 = instance.subscribe(
            [&](const code&, add_element, bool, double, std::string) NOEXCEPT
            {
                second_called = true;
                return true;
            });

        promise.set_value({ec1, ec2});
    });

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::service_stopped);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());

    const auto [ec1, ec2] = promise.get_future().get();
    BOOST_REQUIRE(!ec1);
    BOOST_REQUIRE(!ec2);
    BOOST_REQUIRE(first_called);
    BOOST_REQUIRE(second_called);
}

BOOST_AUTO_TEST_CASE(distributor_rpc__notify__multiple_decayable_subscribers__invokes_both)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_mock instance(strand);

    bool first_called{};
    bool second_called{};
    bool first_result_a{};
    bool second_result_a{};
    double first_result_b{};
    double second_result_b{};
    std::string first_result_c{};
    std::string second_result_c{};
    std::promise<code> promise{};
    std::promise<code> first_promise{};
    std::promise<code> second_promise{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe(
            [&](const code& ec, add_element, const bool a, double b, const std::string& c) NOEXCEPT
            {
                // Avoid stop notification (unavoidable test condition).
                if (first_called)
                    return false;

                first_called = true;
                first_result_a = a;
                first_result_b = b;
                first_result_c = c;
                first_promise.set_value(ec);
                return true;
            });

        instance.subscribe(
            [&](const code& ec, add_element, bool a, double&& b, std::string c) NOEXCEPT
            {
                // Avoid stop notification (unavoidable test condition).
                if (second_called)
                    return false;

                second_called = true;
                second_result_a = a;
                second_result_b = b;
                second_result_c = c;
                second_promise.set_value(ec);
                return true;
            });

        request_t request{};
        request.method = "add_element";
        request.params = params_t{ array_t{ boolean_t{ true }, number_t{ 24.0 }, string_t{ "42" } } };
        const auto ec = instance.notify(request);
        promise.set_value(ec);
    });

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::service_stopped);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(!promise.get_future().get());
    BOOST_REQUIRE(!first_promise.get_future().get());
    BOOST_REQUIRE(!second_promise.get_future().get());
    BOOST_REQUIRE(first_called);
    BOOST_REQUIRE(second_called);
    BOOST_CHECK_EQUAL(first_result_a, true);
    BOOST_CHECK_EQUAL(second_result_a, true);
    BOOST_CHECK_EQUAL(first_result_b, 24.0);
    BOOST_CHECK_EQUAL(second_result_b, 24.0);
    BOOST_CHECK_EQUAL(first_result_c, "42");
    BOOST_CHECK_EQUAL(second_result_c, "42");
}

////BOOST_AUTO_TEST_CASE(distributor_rpc__stop__subscribed_handler__notifies_with_error_code)
////{
////    threadpool pool(2);
////    asio::strand strand(pool.service().get_executor());
////    distributor_mock instance(strand);
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
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.stop(error::service_stopped);
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
////    distributor_mock instance(strand);
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
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.stop(error::service_stopped);
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
////    distributor_mock instance(strand);
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
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.stop(error::service_stopped);
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
////    distributor_mock instance(strand);
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
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.stop(error::service_stopped);
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
////    distributor_mock instance(strand);
////
////    bool called{};
////    bool result_a{};
////    double result_b{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe(
////            [&](const code& ec, add_element, bool a, double b, std::string c) NOEXCEPT
////            {
////                called = true;
////                result_a = a;
////                result_b = b;
////                return true;
////            });
////
////        request_t request{};
////        request.method = "add_element";
////        array_t params_array{ value_t{ true }, value_t{ 24.0 } };
////        request.params = params_t{ params_array };
////        const auto ec = instance.notify(request);
////        promise.set_value(ec);
////    });
////
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.stop(error::service_stopped);
////    });
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////    BOOST_REQUIRE(!promise.get_future().get());
////    BOOST_REQUIRE(called);
////    BOOST_REQUIRE_EQUAL(result_a, true);
////    BOOST_REQUIRE_EQUAL(result_b, 24.0);
////}
////
////BOOST_AUTO_TEST_CASE(distributor_rpc__notify__add_element_named_params__success_and_notifies_with_args)
////{
////    threadpool pool(2);
////    asio::strand strand(pool.service().get_executor());
////    distributor_mock instance(strand);
////
////    bool called{};
////    int result_a{};
////    int result_b{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe(
////            [&](const code& ec, add_element, bool a, double b, std::string c) NOEXCEPT
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
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.stop(error::service_stopped);
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
////    distributor_mock instance(strand);
////
////    bool called{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe(
////            [&](const code& ec, add_element, bool a, double b, std::string c) NOEXCEPT
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
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.stop(error::service_stopped);
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
////    distributor_mock instance(strand);
////
////    bool called{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe(
////            [&](const code& ec, add_element, bool a, double b, std::string c) NOEXCEPT
////            {
////                called = true;
////                return true;
////            });
////
////        request_t request{};
////        request.method = "add_element";
////        array_t params_array{ value_t{ string_t{ "invalid" } }, value_t{ 24.0 } };
////        request.params = params_t{ params_array };
////        const auto ec = instance.notify(request);
////        promise.set_value(ec);
////    });
////
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.stop(error::service_stopped);
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
////    distributor_mock instance(strand);
////
////    bool called{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe(
////            [&](const code& ec, add_element, bool a, double b, std::string c) NOEXCEPT
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
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.stop(error::service_stopped);
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
////    distributor_mock instance(strand);
////
////    bool called{};
////    std::promise<code> promise{};
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.subscribe(
////            [&](const code& ec, add_element, bool a, double b, std::string c) NOEXCEPT
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
////    boost::asio::post(strand, [&]() NOEXCEPT
////    {
////        instance.stop(error::service_stopped);
////    });
////
////    pool.stop();
////    BOOST_REQUIRE(pool.join());
////    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::not_found);
////    BOOST_REQUIRE(!called);
////}

BOOST_AUTO_TEST_SUITE_END()
