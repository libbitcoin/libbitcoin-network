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

#include <tuple>

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
