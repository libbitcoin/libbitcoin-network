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

struct mock_methods
{
    static constexpr std::tuple methods
    {
        method<"get_version">{},
        method<"add_element", bool, double, std::string>{ "a", "b", "c" }
        ////method<"foobar", std::string, optional<4.2>, nullable<bool>>{ "a", "b", "c" }
    };
};

using mock = interface<mock_methods>;

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

BOOST_AUTO_TEST_CASE(distributor_rpc__notify__unknown_method__unexpected_method)
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
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::unexpected_method);
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

BOOST_AUTO_TEST_CASE(distributor_rpc__subscribe__multiple_stop__expected)
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
        promise.set_value(instance.notify(request));
    });

    BOOST_REQUIRE(!promise.get_future().get());
    BOOST_REQUIRE(!first_promise.get_future().get());
    BOOST_REQUIRE(!second_promise.get_future().get());
    BOOST_CHECK_EQUAL(first_result_a, true);
    BOOST_CHECK_EQUAL(second_result_a, true);
    BOOST_CHECK_EQUAL(first_result_b, 24.0);
    BOOST_CHECK_EQUAL(second_result_b, 24.0);
    BOOST_CHECK_EQUAL(first_result_c, "42");
    BOOST_CHECK_EQUAL(second_result_c, "42");

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::service_stopped);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(distributor_rpc__notify__get_version_no_params__success_and_notifies)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_mock instance(strand);

    code ec{};
    bool called{};
    std::promise<code> promise{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](const code& ec, get_version) NOEXCEPT
        {
            // Avoid stop notification (unavoidable test condition).
            if (called)
                return false;

            called = true;
            promise.set_value(ec);
            return true;
        });

        request_t request{};
        request.method = "get_version";
        ec = instance.notify(request);
    });

    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(!promise.get_future().get());

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::service_stopped);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(distributor_rpc__notify__get_version_empty_array_params__success_and_notifies)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_mock instance(strand);

    bool called{};
    std::promise<code> promise{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](const code& ec, get_version) NOEXCEPT
        {
            // Avoid stop notification (unavoidable test condition).
            if (called)
                return false;

            called = true;
            promise.set_value(ec);
            return true;
        });

        const request_t request
        {
            .method = "get_version",
            .params = { array_t{} }
        };

        instance.notify(request);
    });

    BOOST_REQUIRE(!promise.get_future().get());

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::service_stopped);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(distributor_rpc__notify__get_version_with_params__extra_positional)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_mock instance(strand);

    code result{};
    std::promise<code> promise{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe([&](const code& ec, get_version) NOEXCEPT
        {
            result = ec;
            return true;
        });

        const request_t request
        {
            .method = "get_version",
            .params = { array_t{ value_t{ 1.0 } } }
        };

        promise.set_value(instance.notify(request));
    });

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::service_stopped);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE_EQUAL(promise.get_future().get(), error::extra_positional);
    BOOST_REQUIRE_EQUAL(result, error::service_stopped);
}

BOOST_AUTO_TEST_CASE(distributor_rpc__notify__add_element_positional_params__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_mock instance(strand);

    bool called{};
    bool result_a{};
    double result_b{};
    std::string result_c{};
    std::promise<code> promise1{};
    std::promise<code> promise2{};
    std::promise<code> promise3{};
    std::promise<code> promise4{};
    std::promise<code> promise5{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe(
            [&](const code& ec, add_element, bool a, double b, std::string c) NOEXCEPT
            {
                // Avoid stop notification (unavoidable test condition).
                if (called)
                    return false;

                called = true;
                result_a = a;
                result_b = b;
                result_c = c;
                promise5.set_value(ec);
                return true;
            });

        promise1.set_value(instance.notify(
        {
            .method = "add_element",
            .params = { array_t{ value_t{ true }, value_t{ 24.0 } } }
        }));

        promise2.set_value(instance.notify(
        {
            .method = "add_element",
            .params = { array_t{ string_t{ "42" }, number_t{ 24.0 }, boolean_t{ true } } }
        }));

        promise3.set_value(instance.notify(
        {
            .method = "add_element",
            .params = { array_t{ string_t{ "42" }, number_t{ 24.0 }, boolean_t{ true }, boolean_t{ true } } }
        }));

        promise4.set_value(instance.notify(
        {
            .method = "add_element",
            .params = { array_t{ boolean_t{ true }, number_t{ 24.0 }, string_t{ "42" } } }
        }));
    });

    BOOST_REQUIRE_EQUAL(promise1.get_future().get(), error::missing_parameter);
    BOOST_REQUIRE_EQUAL(promise2.get_future().get(), error::unexpected_type);
    BOOST_REQUIRE(!promise4.get_future().get());
    BOOST_REQUIRE(!promise5.get_future().get());
    BOOST_REQUIRE_EQUAL(result_a, true);
    BOOST_REQUIRE_EQUAL(result_b, 24.0);
    BOOST_REQUIRE_EQUAL(result_c, "42");

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::service_stopped);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(distributor_rpc__notify__add_element_named_params__expected)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    distributor_mock instance(strand);

    bool called{};
    bool result_a{};
    double result_b{};
    std::string result_c{};
    std::promise<code> promise1{};
    std::promise<code> promise2{};
    std::promise<code> promise3{};
    std::promise<code> promise4{};
    std::promise<code> promise5{};
    std::promise<code> promise6{};
    std::promise<code> promise7{};
    std::promise<code> promise8{};
    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.subscribe(
            [&](const code& ec, add_element, bool a, double b, std::string c) NOEXCEPT
            {
                // Avoid stop notification (unavoidable test condition).
                if (called)
                    return false;

                called = true;
                result_a = a;
                result_b = b;
                result_c = c;
                promise8.set_value(ec);
                return true;
            });

        promise1.set_value(instance.notify(
        {
            // missing_parameter (absent)
            .method = "add_element",
            .params = { object_t{ { "a", boolean_t{ true } }, { "b", number_t{ 24.0 } } } }
        }));

        promise2.set_value(instance.notify(
        {
            // missing_parameter (misnamed/absent)
            .method = "add_element",
            .params = { object_t{ { "fu", boolean_t{ true } }, { "ga", number_t{ 24.0 } }, { "zi", string_t{ "42" } } } }
        }));

        promise3.set_value(instance.notify(
        {
            // unexpected_type (named but wrong type)
            .method = "add_element",
            .params = { object_t{ { "a", number_t{ 24.0 } }, { "b", number_t{ 24.0 } }, { "c", string_t{ "42" } } } }
        }));

        promise4.set_value(instance.notify(
        {
            // extra_named
            .method = "add_element",
            .params = { object_t{ { "a", boolean_t{ true } }, { "b", number_t{ 24.0 } }, { "c", string_t{ "42" } }, { "d", string_t{ "42" } } } }
        }));

        promise5.set_value(instance.notify(
        {
            // success – duplicate keys are allowed (real JSON input)
            // Boost.JSON parser resolves duplicates using last-writer-wins,
            // before object_t conversion occurs, so map never sees duplicates.
            // Test construction uses initializer_list -> first-writer-wins.
            .method = "add_element",
            .params = { object_t{ { "a", boolean_t{ false } }, { "b", number_t{ 42.0 } }, { "c", string_t{ "24" } }, { "c", string_t{ "42" } } } }
        }));

        promise6.set_value(instance.notify(
        {
            // success, in order
            .method = "add_element",
            .params = { object_t{ { "a", boolean_t{ true } }, { "b", number_t{ 24.0 } }, { "c", string_t{ "42" } } } }
        }));

        promise7.set_value(instance.notify(
        {
            // success, out of order
            .method = "add_element",
            .params = { object_t{ { "b", number_t{ 24.0 } }, { "c", string_t{ "24" } }, { "a", boolean_t{ false } } } }
        }));
    });

    BOOST_REQUIRE_EQUAL(promise1.get_future().get(), error::missing_parameter);
    BOOST_REQUIRE_EQUAL(promise2.get_future().get(), error::missing_parameter);
    BOOST_REQUIRE_EQUAL(promise3.get_future().get(), error::unexpected_type);
    BOOST_REQUIRE_EQUAL(promise4.get_future().get(), error::extra_named);
    BOOST_REQUIRE(!promise5.get_future().get());
    BOOST_REQUIRE(!promise6.get_future().get());
    BOOST_REQUIRE(!promise7.get_future().get());
    BOOST_REQUIRE(!promise8.get_future().get());
    BOOST_REQUIRE_EQUAL(result_a, false);
    BOOST_REQUIRE_EQUAL(result_b, 42.0);
    BOOST_REQUIRE_EQUAL(result_c, "24");

    boost::asio::post(strand, [&]() NOEXCEPT
    {
        instance.stop(error::service_stopped);
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_SUITE_END()
