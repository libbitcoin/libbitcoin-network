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

BOOST_AUTO_TEST_SUITE(json_rpc_tests)

using namespace rpc;

template <typename Type>
using names_t = typename parameter_names<Type>::type;

static_assert(is_same_type<names_t<method<"foo", bool, double>>, std::array<std::string_view, 2>>);
static_assert(is_same_type<names_t<method<"bar">>, std::array<std::string_view, 0>>);
static_assert(is_same_type<names_t<std::tuple<bool, double>>, std::array<std::string_view, 2>>);
static_assert(is_same_type<names_t<std::tuple<>>, std::array<std::string_view, 0>>);

static_assert( is_same_type<method<"test2">, method<"test2">>);
static_assert(!is_same_type<method<"test1">, method<"test2">>);
static_assert(!is_same_type<method<"test1", bool>, method<"test1", int>>);
static_assert(!is_same_type<method<"test1", bool>, method<"test2", bool>>);
static_assert( is_same_type<method<"test1", bool>, method<"test1", bool>>);

using truth = option<true>;
////using only = option<42>;
using every = option<4.2>;
using hello = option<"hello"_t>;
using world = option<"world!"_t>;

static_assert(truth::value == true);
////static_assert(only::value == 42);
static_assert(every::value == 4.2);
static_assert(hello::value == "hello");
static_assert(world::value == "world!");
static_assert(hello::value != world::value);

static_assert(is_same_type<truth::type, boolean_t>);
static_assert(is_same_type<every::type, number_t>);
static_assert(is_same_type<hello::type, string_t>);
static_assert(is_same_type<world::type, string_t>);

static_assert(is_same_type<option<true>::type, boolean_t>);
static_assert(is_same_type<option<false>::type, boolean_t>);
////static_assert(is_same_type<option<42>::type, number_t>);
static_assert(is_same_type<option<4.2>::type, number_t>);
static_assert(is_same_type<option<-4.2>::type, number_t>);
static_assert(is_same_type<option<"hello"_t>::type, string_t>);
static_assert(is_same_type<option<"world!"_t>::type, string_t>);

BOOST_AUTO_TEST_SUITE_END()
