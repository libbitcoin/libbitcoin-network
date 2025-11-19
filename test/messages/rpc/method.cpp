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
#include "../../test.hpp"

#include <tuple>

BOOST_AUTO_TEST_SUITE(rpc_method_tests)

using namespace rpc;

// setup
// -----------------------------------------------------------------------------

struct tag_a {};
struct tag_b {};

struct method0
{
    using args_native = std::tuple<int, bool>;
    using args = std::tuple<int, bool>;
    using tag = tag_a;
};

struct method1
{
    using args_native = std::tuple<double>;
    using args = std::tuple<double>;
    using tag = tag_b;
};

struct method2
{
    using args_native = std::tuple<short>;
    using args = std::tuple<char const*, short>;
    using tag = void;
};

using test_methods0 = std::tuple<method0, method1, method2>;
static_assert(std::tuple_size_v<test_methods0> == 3u);

// args_t
// -----------------------------------------------------------------------------

static_assert(is_same_type<args_t<method0>, std::tuple<int, bool>>);
static_assert(is_same_type<args_t<method1>, std::tuple<double>>);
static_assert(is_same_type<args_t<method2>, std::tuple<char const*, short>>);

// through method_t indexing
static_assert(is_same_type<args_t<method_t<0, test_methods0>>, std::tuple<int, bool>>);
static_assert(is_same_type<args_t<method_t<1, test_methods0>>, std::tuple<double>>);
static_assert(is_same_type<tag_t<method_t<0, test_methods0>>, tag_a>);
static_assert(is_same_type<tag_t<method_t<2, test_methods0>>, void>);

// args_native_t
// -----------------------------------------------------------------------------

static_assert(is_same_type<args_native_t<method0>, std::tuple<int, bool>>);
static_assert(is_same_type<args_native_t<method1>, std::tuple<double>>);
static_assert(is_same_type<args_native_t<method2>, std::tuple<short>>);

// tag_t
// -----------------------------------------------------------------------------
static_assert(is_same_type<tag_t<method0>, tag_a>);
static_assert(is_same_type<tag_t<method1>, tag_b>);
static_assert(is_same_type<tag_t<method2>, void>);

// method_t
// -----------------------------------------------------------------------------
static_assert(is_same_type<method_t<0, test_methods0>, method0>);
static_assert(is_same_type<method_t<1, test_methods0>, method1>);
static_assert(is_same_type<method_t<2, test_methods0>, method2>);

// method<>
// -----------------------------------------------------------------------------

static_assert(is_same_type<method<"test2">, method<"test2">>);
static_assert(!is_same_type<method<"test1">, method<"test2">>);
static_assert(!is_same_type<method<"test1", bool>, method<"test1", int>>);
static_assert(!is_same_type<method<"test1", bool>, method<"test2", bool>>);
static_assert(is_same_type<method<"test1", bool>, method<"test1", bool>>);

// names_t<>
// -----------------------------------------------------------------------------

// method
static_assert(is_same_type<names_t<method<"foo", bool, double>>, std::array<std::string_view, 2>>);
static_assert(is_same_type<names_t<method<"bar">>, std::array<std::string_view, 0>>);

// tuple
static_assert(is_same_type<names_t<std::tuple<bool, double>>, std::array<std::string_view, 2>>);
static_assert(is_same_type<names_t<std::tuple<>>, std::array<std::string_view, 0>>);

// subscriber_t<Method>
// ----------------------------------------------------------------------------

// required

static constexpr std::tuple required_methods
{
    method<"foo", double>{ "i" },
    method<"bar", bool, std::string>{ "b", "s" }
};

using required_methods_t = decltype(required_methods);
using foo_method_t = std::tuple_element_t<0, required_methods_t>;
using bar_method_t = std::tuple_element_t<1, required_methods_t>;
static_assert(is_same_type<foo_method_t, method<"foo", double>>);
static_assert(is_same_type<bar_method_t, method<"bar", bool, std::string>>);

using foo_args_native = typename foo_method_t::args_native;
using bar_args_native = typename bar_method_t::args_native;
using foo_args = typename foo_method_t::args;
using bar_args = typename bar_method_t::args;
using foo_tag = typename foo_method_t::tag;
using bar_tag = typename bar_method_t::tag;

static_assert(foo_method_t::size == 1u);
static_assert(bar_method_t::size == 2u);
static_assert(!foo_method_t::native);
static_assert(!bar_method_t::native);

static_assert(is_same_type<foo_args_native, std::tuple<double>>);
static_assert(is_same_type<bar_args_native, std::tuple<bool, std::string>>);
static_assert(is_same_type<foo_args, std::tuple<foo_tag, double>>);
static_assert(is_same_type<bar_args, std::tuple<bar_tag, bool, std::string>>);

using foo_unsubscriber = subscriber_t<foo_method_t>;
using bar_unsubscriber = subscriber_t<bar_method_t>;
using foo_unsubscriber_handler = typename foo_unsubscriber::handler;
using bar_unsubscriber_handler = typename bar_unsubscriber::handler;
using foo_unsubscriber_handler_args = handler_args_t<foo_unsubscriber_handler>;
using bar_unsubscriber_handler_args = handler_args_t<bar_unsubscriber_handler>;

using foo_unsubscriber_handler_arg0 = std::tuple_element_t<0, foo_unsubscriber_handler_args>;
using foo_unsubscriber_handler_arg1 = std::tuple_element_t<1, foo_unsubscriber_handler_args>;
static_assert(is_same_type<foo_unsubscriber_handler_arg0, foo_tag>);
static_assert(is_same_type<foo_unsubscriber_handler_arg1, double>);

using bar_unsubscriber_handler_arg0 = std::tuple_element_t<0, bar_unsubscriber_handler_args>;
using bar_unsubscriber_handler_arg1 = std::tuple_element_t<1, bar_unsubscriber_handler_args>;
using bar_unsubscriber_handler_arg2 = std::tuple_element_t<2, bar_unsubscriber_handler_args>;
static_assert(is_same_type<bar_unsubscriber_handler_arg0, bar_tag>);
static_assert(is_same_type<bar_unsubscriber_handler_arg1, bool>);
static_assert(is_same_type<bar_unsubscriber_handler_arg2, std::string>);

static_assert(is_same_type<foo_unsubscriber, network::unsubscriber<foo_tag, double>>);
static_assert(is_same_type<bar_unsubscriber, network::unsubscriber<bar_tag, bool, std::string>>);

// optional

static constexpr std::tuple optional_methods
{
    method<"optional", optional<true>, optional<"default"_t>>{ "b", "s" }
};

using optional_methods_t = decltype(optional_methods);
using optional_method_t = std::tuple_element_t<0, optional_methods_t>;
static_assert(is_same_type<optional_method_t, method<"optional", optional<true>, optional<"default"_t>>>);

using optional_tag = typename optional_method_t::tag;
using optional_unsubscriber = subscriber_t<optional_method_t>;
static_assert(is_same_type<optional_unsubscriber, network::unsubscriber<optional_tag, bool, std::string>>);

// nullable

static constexpr std::tuple nullable_methods
{
    method<"nullable", nullable<bool>, nullable<std::string>>{ "b", "s" }
};

using nullable_methods_t = decltype(nullable_methods);
using nullable_method_t = std::tuple_element_t<0, nullable_methods_t>;
static_assert(is_same_type<nullable_method_t, method<"nullable", nullable<bool>, nullable<std::string>>>);

using nullable_tag = typename nullable_method_t::tag;
using nullable_unsubscriber = subscriber_t<nullable_method_t>;
static_assert(is_same_type<nullable_unsubscriber, network::unsubscriber<nullable_tag, std::optional<bool>, std::optional<std::string>>>);

// subscribers_t<Method...>
// ----------------------------------------------------------------------------

using foobar_subscribers_t = subscribers_t<required_methods_t>;
static_assert(std::tuple_size_v<foobar_subscribers_t> == 2u);
static_assert(is_same_type<std::tuple_element_t<0, foobar_subscribers_t>, foo_unsubscriber>);
static_assert(is_same_type<std::tuple_element_t<1, foobar_subscribers_t>, bar_unsubscriber>);

BOOST_AUTO_TEST_SUITE_END()
