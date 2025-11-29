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

BOOST_AUTO_TEST_SUITE(dispatcher_tests)

using namespace rpc;

// uses unsubscriber<> (bool handler returns).
struct mock_methods
{
    static constexpr std::tuple methods
    {
        method<"empty_method">{},
        method<"all_required", bool, double, std::string>{ "a", "b", "c" },
        method<"with_options", std::string, optional<4.2>, optional<true>>{ "a", "b", "c" },
        method<"with_nullify", std::string, nullable<double>, nullable<bool>>{ "a", "b", "c" },
        method<"with_combine", std::string, nullable<bool>, optional<4.2>>{ "a", "b", "c" },
        method<"not_required", nullable<bool>, optional<4.2>>{ "a", "b" },
        method<"ping", messages::peer::ping::cptr>{ "message" }
    };

    template <typename... Args>
    using subscriber = network::unsubscriber<Args...>;

    template <size_t Index>
    using at = method_at<methods, Index>;

    // Derive this from above in c++26 using reflection.
    using empty_method = at<0>;
    using all_required = at<1>;
    using with_options = at<2>;
    using with_nullify = at<3>;
    using with_combine = at<4>;
    using not_required = at<5>;
    using ping = at<6>;
};

using mock_interface = publish<mock_methods>;
using distributor_mock = dispatcher<mock_interface>;

BOOST_AUTO_TEST_CASE(dispatcher__construct__stop__stops)
{
    distributor_mock instance{};
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__no_subscriber__success)
{
    distributor_mock instance{};
    request_t request{};
    request.method = "empty_method";
    const auto ec = instance.notify(request);
    instance.stop(error::service_stopped);
    BOOST_REQUIRE_EQUAL(ec, error::success);
}

BOOST_AUTO_TEST_CASE(dispatcher__subscribe__stopped__subscriber_stopped)
{
    distributor_mock instance{};
    auto result = true;
    instance.stop(error::invalid_magic);
    const auto subscribe_ec = instance.subscribe(
        [&](const code&, mock_interface::all_required, bool a, double b, std::string c)
        {
            static_assert(mock_interface::all_required::name == "all_required");
            static_assert(mock_interface::all_required::size == 3u);

            // Stop notification sets defaults and specified code.
            result &= is_zero(a);
            result &= is_zero(b);
            result &= c.empty();
            return false;
        });

    BOOST_REQUIRE_EQUAL(error::subscriber_stopped, subscribe_ec);
    BOOST_REQUIRE(result);
}

BOOST_AUTO_TEST_CASE(dispatcher__subscribe__stop__service_stopped)
{
    distributor_mock instance{};
    code result_ec{};
    auto result = true;
    const auto subscribe_ec = instance.subscribe(
        [&](const code& ec, mock_interface::all_required, bool a, double b, std::string c)
        {
            // Stop notification sets defaults and specified code.
            result_ec = ec;
            result &= is_zero(a);
            result &= is_zero(b);
            result &= c.empty();
            return true;
        });

    instance.stop(error::invalid_magic);
    BOOST_REQUIRE_EQUAL(result_ec, error::invalid_magic);
    BOOST_REQUIRE(!subscribe_ec);
    BOOST_REQUIRE(result);
}

BOOST_AUTO_TEST_CASE(dispatcher__subscribe__multiple__expected)
{
    distributor_mock instance{};

    const auto ec1 = instance.subscribe(
        [&](const code&, mock_interface::all_required, bool, double, std::string)
        {
            return true;
        });

    const auto ec2 = instance.subscribe(
        [&](const code&, mock_interface::all_required, bool, double, std::string)
        {
            return true;
        });

    BOOST_REQUIRE(!ec1);
    BOOST_REQUIRE(!ec2);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__unknown_method__unexpected_method)
{
    distributor_mock instance{};
    request_t request{};
    request.method = "unknown_method";
    BOOST_REQUIRE_EQUAL(instance.notify(request), error::unexpected_method);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__multiple_decayable_subscribers__invokes_both)
{
    distributor_mock instance{};
    bool first_called{};
    bool second_called{};
    bool first_result_a{};
    bool second_result_a{};
    double first_result_b{};
    double second_result_b{};
    std::string first_result_c{};
    std::string second_result_c{};

    instance.subscribe(
        [&](const code&, mock_interface::all_required, const bool a, double b, const std::string& c)
        {
            first_called = true;
            first_result_a = a;
            first_result_b = b;
            first_result_c = c;
            return true;
        });

    instance.subscribe(
        [&](const code&, mock_interface::all_required, bool a, double&& b, std::string c)
        {
            second_called = true;
            second_result_a = a;
            second_result_b = b;
            second_result_c = c;
            return true;
        });

    request_t request{};
    request.method = "all_required";
    request.params = params_t{ array_t{ boolean_t{ true }, number_t{ 24.0 }, string_t{ "42" } } };
    BOOST_REQUIRE(!instance.notify(request));
    BOOST_CHECK_EQUAL(first_result_a, true);
    BOOST_CHECK_EQUAL(second_result_a, true);
    BOOST_CHECK_EQUAL(first_result_b, 24.0);
    BOOST_CHECK_EQUAL(second_result_b, 24.0);
    BOOST_CHECK_EQUAL(first_result_c, "42");
    BOOST_CHECK_EQUAL(second_result_c, "42");
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__empty_method_no_params__success)
{
    distributor_mock instance{};
    bool called{};

    instance.subscribe([&](const code&, mock_interface::empty_method)
    {
        if (called) return false;
        called = true;
        return true;
    });

    request_t request{};
    request.method = "empty_method";
    BOOST_REQUIRE(!instance.notify(request));
    BOOST_REQUIRE(called);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__empty_method_empty_array__success)
{
    distributor_mock instance{};
    bool called{};

    instance.subscribe([&](const code&, mock_interface::empty_method)
    {
        if (called) return false;
        called = true;
        return true;
    });

    const request_t request
    {
        .method = "empty_method",
        .params = { array_t{} }
    };

    BOOST_REQUIRE(!instance.notify(request));
    BOOST_REQUIRE(called);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__empty_method_array_params__extra_positional)
{
    distributor_mock instance{};

    const request_t request
    {
        .method = "empty_method",
        .params = { array_t{ value_t{ 1.0 } } }
    };

    BOOST_REQUIRE_EQUAL(instance.notify(request), error::extra_positional);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__all_required_positional_params__expected)
{
    distributor_mock instance{};
    bool called{};
    bool result_a{};
    double result_b{};
    std::string result_c{};

    instance.subscribe(
        [&](const code&, mock_interface::all_required, bool a, double b, std::string c)
        {
            if (called) return false;
            called = true;
            result_a = a;
            result_b = b;
            result_c = c;
            return true;
        });

    const auto ec1 = instance.notify(
    {
        .method = "all_required",
        .params = { array_t{ boolean_t{ true }, value_t{ 24.0 } } }
    });

    const auto ec2 = instance.notify(
    {
        .method = "all_required",
        .params = { array_t{ string_t{ "42" }, number_t{ 24.0 }, boolean_t{ true } } }
    });

    const auto ec3 = instance.notify(
    {
        .method = "all_required",
        .params = { array_t{ boolean_t{ true }, number_t{ 24.0 }, string_t{ "42" }, string_t{ "42" } } }
    });

    const auto ec4 = instance.notify(
    {
        .method = "all_required",
        .params = { array_t{ boolean_t{ true }, number_t{ 24.0 }, string_t{ "42" } } }
    });

    BOOST_REQUIRE_EQUAL(ec1, error::missing_parameter);
    BOOST_REQUIRE_EQUAL(ec2, error::unexpected_type);
    BOOST_REQUIRE_EQUAL(ec3, error::extra_positional);
    BOOST_REQUIRE(!ec4);
    BOOST_REQUIRE_EQUAL(result_a, true);
    BOOST_REQUIRE_EQUAL(result_b, 24.0);
    BOOST_REQUIRE_EQUAL(result_c, "42");
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__all_required_named_params__expected)
{
    distributor_mock instance{};
    bool called{};
    bool result_a{};
    double result_b{};
    std::string result_c{};

    instance.subscribe(
        [&](const code&, mock_interface::all_required, bool a, double b, std::string c)
        {
            if (called) return false;
            called = true;
            result_a = a;
            result_b = b;
            result_c = c;
            return true;
        });

    const auto ec1 = instance.notify(
    {
        // missing_parameter (absent)
        .method = "all_required",
        .params = { object_t{ { "a", boolean_t{ true } }, { "b", number_t{ 24.0 } } } }
    });

    const auto ec2 = instance.notify(
    {
        // missing_parameter (misnamed/absent)
        .method = "all_required",
        .params = { object_t{ { "fu", boolean_t{ true } }, { "ga", number_t{ 24.0 } }, { "zi", string_t{ "42" } } } }
    });

    const auto ec3 = instance.notify(
    {
        // unexpected_type (named but wrong type)
        .method = "all_required",
        .params = { object_t{ { "a", number_t{ 24.0 } }, { "b", number_t{ 24.0 } }, { "c", string_t{ "42" } } } }
    });

    const auto ec4 = instance.notify(
    {
        // extra_named
        .method = "all_required",
        .params = { object_t{ { "a", boolean_t{ true } }, { "b", number_t{ 24.0 } }, { "c", string_t{ "42" } }, { "d", string_t{ "42" } } } }
    });

    const auto ec5 = instance.notify(
    {
        // success – duplicate keys are allowed (real JSON input)
        // Boost.JSON parser resolves duplicates using last-writer-wins,
        // before object_t conversion occurs, so map never sees duplicates.
        // Test construction uses initializer_list -> first-writer-wins.
        .method = "all_required",
        .params = { object_t{ { "a", boolean_t{ false } }, { "b", number_t{ 42.0 } }, { "c", string_t{ "24" } }, { "c", string_t{ "42" } } } }
    });

    const auto ec6 = instance.notify(
    {
        // success, in order
        .method = "all_required",
        .params = { object_t{ { "a", boolean_t{ true } }, { "b", number_t{ 24.0 } }, { "c", string_t{ "42" } } } }
    });

    const auto ec7 = instance.notify(
    {
        // success, out of order
        .method = "all_required",
        .params = { object_t{ { "b", number_t{ 24.0 } }, { "c", string_t{ "24" } }, { "a", boolean_t{ false } } } }
    });

    BOOST_REQUIRE_EQUAL(ec1, error::missing_parameter);
    BOOST_REQUIRE_EQUAL(ec2, error::missing_parameter);
    BOOST_REQUIRE_EQUAL(ec3, error::unexpected_type);
    BOOST_REQUIRE_EQUAL(ec4, error::extra_named);
    BOOST_REQUIRE(!ec5);
    BOOST_REQUIRE(!ec6);
    BOOST_REQUIRE(!ec7);
    BOOST_REQUIRE_EQUAL(result_a, false);
    BOOST_REQUIRE_EQUAL(result_b, 42.0);
    BOOST_REQUIRE_EQUAL(result_c, "24");
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__with_options_positional_params__expected)
{
    distributor_mock instance{};
    bool called{};
    string_t result_a{};
    number_t result_b{};
    boolean_t result_c{};

    instance.subscribe(
        [&](const code&, mock_interface::with_options, std::string a, double b, bool c)
        {
            if (called) return false;
            called = true;
            result_a = a;
            result_b = b;
            result_c = c;
            return true;
        });

    const auto ec1 = instance.notify(
    {
        .method = "with_options",
        .params = { array_t{} }
    });

    const auto ec2 = instance.notify(
    {
        .method = "with_options",
        .params = { array_t{ string_t{ "42" } } }
    });

    const auto ec3 = instance.notify(
    {
        .method = "with_options",
        .params = { array_t{ string_t{ "42" }, number_t{ 42.0 } } }
    });

    const auto ec4 = instance.notify(
    {
        .method = "with_options",
        .params = { array_t{ string_t{ "42" }, number_t{ 42.0 }, boolean_t{ false } } }
    });

    BOOST_REQUIRE_EQUAL(ec1, error::missing_parameter);
    BOOST_REQUIRE(!ec2);
    BOOST_REQUIRE(!ec3);
    BOOST_REQUIRE(!ec4);
    BOOST_REQUIRE_EQUAL(result_a, "42");
    BOOST_REQUIRE_EQUAL(result_b, 4.2);
    BOOST_REQUIRE_EQUAL(result_c, true);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__with_options_named_params__expected)
{
    distributor_mock instance{};
    bool called{};
    string_t result_a{};
    number_t result_b{};
    boolean_t result_c{};

    instance.subscribe(
        [&](const code&, mock_interface::with_options, std::string a, double b, bool c)
        {
            if (called) return false;
            called = true;
            result_a = a;
            result_b = b;
            result_c = c;
            return true;
        });

    const auto ec1 = instance.notify(
    {
        .method = "with_options",
        .params = { object_t{} }
    });

    const auto ec2 = instance.notify(
    {
        .method = "with_options",
        .params = { object_t{ { "a", string_t{ "42" } } } }
    });

    const auto ec3 = instance.notify(
    {
        .method = "with_options",
        .params = { object_t{ { "a", string_t{ "42" } }, { "b", number_t{ 42.0 } } } }
    });

    const auto ec4 = instance.notify(
    {
        .method = "with_options",
        .params = { object_t{ { "a", string_t{ "42" } }, { "b", number_t{ 42.0 } }, { "c", boolean_t{ false } } } }
    });

    BOOST_REQUIRE_EQUAL(ec1, error::missing_parameter);
    BOOST_REQUIRE(!ec2);
    BOOST_REQUIRE(!ec3);
    BOOST_REQUIRE(!ec4);
    BOOST_REQUIRE_EQUAL(result_a, "42");
    BOOST_REQUIRE_EQUAL(result_b, 4.2);
    BOOST_REQUIRE_EQUAL(result_c, true);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__with_nullify_positional_params__expected)
{
    distributor_mock instance{};
    bool called{};
    string_t result_a{};
    number_t result_b{};
    boolean_t result_c{};

    instance.subscribe(
        [&](const code&, mock_interface::with_nullify, std::string a, std::optional<double> b, std::optional<bool> c)
        {
            if (called) return false;
            called = true;
            result_a = a;
            result_b = b.has_value() ? b.value() : 4.2;
            result_c = c.has_value() ? c.value() : true;
            return true;
        });

    const auto ec1 = instance.notify(
    {
        .method = "with_nullify",
        .params = { array_t{ null_t{}, null_t{}, null_t{} } }
    });

    const auto ec2 = instance.notify(
    {
        .method = "with_nullify",
        .params = { array_t{ string_t{ "42" }, null_t{}, null_t{} } }
    });

    const auto ec3 = instance.notify(
    {
        .method = "with_nullify",
        .params = { array_t{ string_t{ "42" }, null_t{}, boolean_t{ false } } }
    });

    const auto ec4 = instance.notify(
    {
        .method = "with_nullify",
        .params = { array_t{ string_t{ "42" }, number_t{ 42.0 }, null_t{} } }
    });

    BOOST_REQUIRE_EQUAL(ec1, error::missing_parameter);
    BOOST_REQUIRE(!ec2);
    BOOST_REQUIRE(!ec3);
    BOOST_REQUIRE(!ec4);
    BOOST_REQUIRE_EQUAL(result_a, "42");
    BOOST_REQUIRE_EQUAL(result_b, 4.2);
    BOOST_REQUIRE_EQUAL(result_c, true);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__with_nullify_named_params__expected)
{
    distributor_mock instance{};
    bool called{};
    string_t result_a{};
    number_t result_b{};
    boolean_t result_c{};

    instance.subscribe(
        [&](const code&, mock_interface::with_nullify, std::string a, std::optional<double> b, std::optional<bool> c)
        {
            if (called) return false;
            called = true;
            result_a = a;
            result_b = b.has_value() ? b.value() : 4.2;
            result_c = c.has_value() ? c.value() : true;
            return true;
        });

    const auto ec1 = instance.notify(
    {
        .method = "with_nullify",
        .params = { object_t{ { "a", null_t{} }, { "b", null_t{} }, { "c", null_t{} } } }
    });

    const auto ec2 = instance.notify(
    {
        .method = "with_nullify",
        .params = { object_t{ { "a", string_t{ "42" } }, { "b", null_t{} }, { "c", null_t{} } } }
    });

    const auto ec3 = instance.notify(
    {
        .method = "with_nullify",
        .params = { object_t{ { "a", string_t{ "42" } }, { "b", null_t{} }, { "c", boolean_t{ false } } } }
    });

    const auto ec4 = instance.notify(
    {
        .method = "with_nullify",
        .params = { object_t{ { "a", string_t{ "42" } }, { "b", number_t{ 42.0 } }, { "c", null_t{} } } }
    });

    BOOST_REQUIRE_EQUAL(ec1, error::missing_parameter);
    BOOST_REQUIRE(!ec2);
    BOOST_REQUIRE(!ec3);
    BOOST_REQUIRE(!ec4);
    BOOST_REQUIRE_EQUAL(result_a, "42");
    BOOST_REQUIRE_EQUAL(result_b, 4.2);
    BOOST_REQUIRE_EQUAL(result_c, true);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__with_combine_positional_params__expected)
{
    distributor_mock instance{};
    bool called{};
    string_t result_a{};
    boolean_t result_b{};
    number_t result_c{};

    instance.subscribe(
        [&](const code&, mock_interface::with_combine, std::string a, std::optional<bool> b, double c)
        {
            if (called) return false;
            called = true;
            result_a = a;
            result_b = b.has_value() ? b.value() : true;
            result_c = c;
            return true;
        });

    const auto ec1 = instance.notify(
    {
        .method = "with_combine",
        .params = { array_t{ null_t{}, null_t{}, null_t{} } }
    });

    const auto ec2 = instance.notify(
    {
        .method = "with_combine",
        .params = { array_t{ string_t{ "42" }, null_t{} } }
    });

    const auto ec3 = instance.notify(
    {
        .method = "with_combine",
        .params = { array_t{ string_t{ "42" }, null_t{}, number_t{ 42.0 } } }
    });

    const auto ec4 = instance.notify(
    {
        .method = "with_combine",
        .params = { array_t{ string_t{ "42" }, boolean_t{ false }, number_t{ 42.0 } } }
    });

    BOOST_REQUIRE_EQUAL(ec1, error::missing_parameter);
    BOOST_REQUIRE(!ec2);
    BOOST_REQUIRE(!ec3);
    BOOST_REQUIRE(!ec4);
    BOOST_REQUIRE_EQUAL(result_a, "42");
    BOOST_REQUIRE_EQUAL(result_b, true);
    BOOST_REQUIRE_EQUAL(result_c, 4.2);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__with_combine_named_params__expected)
{
    distributor_mock instance{};
    bool called{};
    string_t result_a{};
    boolean_t result_b{};
    number_t result_c{};

    instance.subscribe(
        [&](const code&, mock_interface::with_combine, std::string a, std::optional<bool> b, double c)
        {
            if (called) return false;
            called = true;
            result_a = a;
            result_b = b.has_value() ? b.value() : true;
            result_c = c;
            return true;
        });

    const auto ec1 = instance.notify(
    {
        .method = "with_combine",
        .params = { object_t{ { "a", null_t{} }, { "b", null_t{} }, { "c", null_t{} } } }
    });

    const auto ec2 = instance.notify(
    {
        .method = "with_combine",
        .params = { object_t{ { "a", string_t{ "42" } }, { "b", null_t{} } } }
    });

    const auto ec3 = instance.notify(
    {
        .method = "with_combine",
        .params = { object_t{ { "a", string_t{ "42" } }, { "b", null_t{} }, { "c", number_t{ 42.0 } } } }
    });

    const auto ec4 = instance.notify(
    {
        .method = "with_combine",
        .params = { object_t{ { "a", string_t{ "42" } }, { "b", boolean_t{ false } }, { "c", number_t{ 42.0 } } } }
    });

    BOOST_REQUIRE_EQUAL(ec1, error::missing_parameter);
    BOOST_REQUIRE(!ec2);
    BOOST_REQUIRE(!ec3);
    BOOST_REQUIRE(!ec4);
    BOOST_REQUIRE_EQUAL(result_a, "42");
    BOOST_REQUIRE_EQUAL(result_b, true);
    BOOST_REQUIRE_EQUAL(result_c, 4.2);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__not_required_positional_params__expected)
{
    distributor_mock instance{};
    bool called{};
    boolean_t result_a{};
    number_t result_b{};

    instance.subscribe(
        [&](const code&, mock_interface::not_required, std::optional<bool> a, double b)
        {
            if (called) return false;
            called = true;
            result_a = a.has_value() ? a.value() : true;
            result_b = b;
            return true;
        });

    const auto ec1 = instance.notify(
    {
        .method = "not_required",
        .params = { array_t{ null_t{}, null_t{} } }
    });

    const auto ec2 = instance.notify(
    {
        .method = "not_required",
        .params = { array_t{} }
    });

    const auto ec3 = instance.notify(
    {
        .method = "not_required",
        .params = { array_t{ boolean_t{ false } } }
    });

    const auto ec4 = instance.notify(
    {
        .method = "not_required",
        .params = { array_t{ boolean_t{ false }, number_t{ 42.0 } } }
    });

    BOOST_REQUIRE_EQUAL(ec1, error::missing_parameter);
    BOOST_REQUIRE(!ec2);
    BOOST_REQUIRE(!ec3);
    BOOST_REQUIRE(!ec4);
    BOOST_REQUIRE_EQUAL(result_a, true);
    BOOST_REQUIRE_EQUAL(result_b, 4.2);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__not_required_named_params__expected)
{
    distributor_mock instance{};
    bool called{};
    boolean_t result_a{};
    number_t result_b{};

    instance.subscribe(
        [&](const code&, mock_interface::not_required, std::optional<bool> a, double b)
        {
            if (called) return false;
            called = true;
            result_a = a.has_value() ? a.value() : true;
            result_b = b;
            return true;
        });

    const auto ec1 = instance.notify(
    {
        .method = "not_required",
        .params = { object_t{ { "a", null_t{} }, { "b", null_t{} } } }
    });

    const auto ec2 = instance.notify(
    {
        .method = "not_required",
        .params = { object_t{} }
    });

    const auto ec3 = instance.notify(
    {
        .method = "not_required",
        .params = { object_t{ { "a", boolean_t{ false } } } }
    });

    const auto ec4 = instance.notify(
    {
        .method = "not_required",
        .params = { object_t{ { "a", boolean_t{ false } }, { "b", number_t{ 42.0 } } } }
    });

    BOOST_REQUIRE_EQUAL(ec1, error::missing_parameter);
    BOOST_REQUIRE(!ec2);
    BOOST_REQUIRE(!ec3);
    BOOST_REQUIRE(!ec4);
    BOOST_REQUIRE_EQUAL(result_a, true);
    BOOST_REQUIRE_EQUAL(result_b, 4.2);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(distributor__notify__ping_positional__expected)
{
    distributor_mock instance{};
    bool called{};
    constexpr auto expected = 42u;
    messages::peer::ping::cptr result{};
    const auto pointer = system::to_shared<messages::peer::ping>(expected);

    instance.subscribe(
        [&](code, messages::peer::ping::cptr ptr)
        {
            if (called) return false;
            called = true;
            result = ptr;
            return true;
        });

    const auto ec = instance.notify(
    {
        .method = "ping",
        .params = { array_t{ any_t{ pointer } } }
    });

    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(result);
    BOOST_REQUIRE(result->id == messages::peer::identifier::ping);
    BOOST_REQUIRE_EQUAL(result->nonce, expected);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(distributor__notify__ping_named__expected)
{
    distributor_mock instance{};
    bool called{};
    constexpr auto expected = 42u;
    messages::peer::ping::cptr result{};
    const auto pointer = system::to_shared<messages::peer::ping>(expected);

    instance.subscribe(
        [&](const code&, const messages::peer::ping::cptr& ptr)
        {
            if (called) return false;
            called = true;
            result = ptr;
            return true;
        });

    const auto ec = instance.notify(
    {
        .method = "ping",
        .params = { object_t{ { "message", any_t{ pointer } } } }
    });

    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(result);
    BOOST_REQUIRE(result->id == messages::peer::identifier::ping);
    BOOST_REQUIRE_EQUAL(result->nonce, expected);
    instance.stop(error::service_stopped);
}

// uses subscriber<> (void handler returns).
struct mock_missing_nullable
{
    static constexpr std::tuple methods
    {
        method<"missing_nullable", double, nullable<bool>>{ "a", "b" },
        method<"missing_nullable_pointer", double, nullable<messages::peer::ping::cptr>>{ "a", "b" },
    };

    template <typename... Args>
    using subscriber = network::subscriber<Args...>;

    template <size_t Index>
    using at = method_at<methods, Index>;

    using missing_nullable = at<0>;
    using missing_nullable_pointer = at<1>;
};

using missing_nullable_interface = publish<mock_missing_nullable>;
using distributor_missing_nullable = dispatcher<missing_nullable_interface>;

BOOST_AUTO_TEST_CASE(dispatcher__notify__missing_nullable__expected)
{
    distributor_missing_nullable instance{};
    double result_a{};
    bool result_b{};
    using method = missing_nullable_interface::missing_nullable;

    instance.subscribe([&](const code&, method::tag, double a, std::optional<bool> b)
    {
        result_a = a;
        result_b = b.value_or(true);
    });
    
    const auto ec1 = instance.notify(
    {
        .method = string_t{ method::name },
        .params = { array_t{ { 42.0 }, { false } } }
    });
    
    const auto ec2 = instance.notify(
    {
        .method = string_t{ method::name },
        .params = { object_t{ { "a", 42.0 }, { "b", false } } }
    });
    
    const auto ec3 = instance.notify(
    {
        .method = string_t{ method::name },
        .params = { object_t{ { "b", false }, { "a", 42.0 } } }
    });
    
    const auto ec4 = instance.notify(
    {
        .method = string_t{ method::name },
        .params = { object_t{ { "a", 42.0 }, { "b", null_t{} } } }
    });
    
    const auto ec5 = instance.notify(
    {
        .method = string_t{ method::name },
        .params = { object_t{ { "a", 24.0 } } }
    });

    BOOST_REQUIRE(!ec1);
    BOOST_REQUIRE(!ec2);
    BOOST_REQUIRE(!ec3);
    BOOST_REQUIRE(!ec4);
    BOOST_REQUIRE(!ec5);
    BOOST_REQUIRE(result_b);
    BOOST_REQUIRE_EQUAL(result_a, 24.0);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_CASE(dispatcher__notify__missing_nullable_pointer__expected)
{
    distributor_missing_nullable instance{};
    double result_a{};
    messages::peer::ping::cptr result_b{};
    using method = missing_nullable_interface::missing_nullable_pointer;
    const auto ping42 = system::emplace_shared<const messages::peer::ping>(42);

    instance.subscribe([&](const code&, method::tag, double a, std::optional<messages::peer::ping::cptr> b)
    {
        result_a = a;
        result_b = b.value_or(nullptr);
    });
    
    const auto ec1 = instance.notify(
    {
        .method = string_t{ method::name },
        .params = { array_t{ 42.0 } }
    });
    
    const auto ec2 = instance.notify(
    {
        .method = string_t{ method::name },
        .params = { array_t{ 42.0, null_t{} } }
    });
    
    const auto ec3 = instance.notify(
    {
        .method = string_t{ method::name },
        .params = { array_t{ null_t{}, 42.0 } }
    });

    // any_t{ ping42 } or { ping42 } must be specified.
    const auto ec4 = instance.notify(
    {
        .method = string_t{ method::name },
        .params = { array_t{ 42.0, any_t{ ping42 } } }
    });
    
    const auto ec5 = instance.notify(
    {
        .method = string_t{ method::name },
        .params = { object_t{ { "a", 42.0 } } }
    });
    
    const auto ec6 = instance.notify(
    {
        .method = string_t{ method::name },
        .params = { object_t{ { "a", 42.0 }, { "a", null_t{} } } }
    });

    // any_t{ ping42 } or { ping42 } must be specified.
    const auto ec7 = instance.notify(
    {
        .method = string_t{ method::name },
        .params = { object_t{ { "a", 42.0 }, { "b", any_t{ ping42 } } } }
    });

    BOOST_REQUIRE(!ec1);
    BOOST_REQUIRE(!ec2);
    BOOST_REQUIRE_EQUAL(ec3, error::missing_parameter);
    BOOST_REQUIRE(!ec4);
    BOOST_REQUIRE(!ec5);
    BOOST_REQUIRE(!ec6);
    BOOST_REQUIRE(!ec7);
    BOOST_REQUIRE_EQUAL(result_a, 42.0);
    BOOST_REQUIRE_EQUAL(result_b->nonce, ping42->nonce);
    instance.stop(error::service_stopped);
}

BOOST_AUTO_TEST_SUITE_END()
