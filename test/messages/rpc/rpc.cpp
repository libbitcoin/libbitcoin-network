/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your optional) any later version.
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

// names_t

template <typename Type>
using names_t = typename parameter_names<Type>::type;
static_assert(is_same_type<names_t<method<"foo", bool, double>>, std::array<std::string_view, 2>>);
static_assert(is_same_type<names_t<method<"bar">>, std::array<std::string_view, 0>>);
static_assert(is_same_type<names_t<std::tuple<bool, double>>, std::array<std::string_view, 2>>);
static_assert(is_same_type<names_t<std::tuple<>>, std::array<std::string_view, 0>>);

// method

static_assert( is_same_type<method<"test2">, method<"test2">>);
static_assert(!is_same_type<method<"test1">, method<"test2">>);
static_assert(!is_same_type<method<"test1", bool>, method<"test1", int>>);
static_assert(!is_same_type<method<"test1", bool>, method<"test2", bool>>);
static_assert( is_same_type<method<"test1", bool>, method<"test1", bool>>);

// optional

using optional_truth = optional<true>;
using optional_every = optional<4.2>;
using optional_hello = optional<"hello"_t>;
using optional_world = optional<"world!"_t>;

static_assert(optional_truth::value == true);
static_assert(optional_every::value == 4.2);
static_assert(optional_hello::value == "hello");
static_assert(optional_world::value == "world!");
static_assert(optional_hello::value != optional_world::value);

static_assert(is_same_type<optional_truth::tag, optional_tag>);
static_assert(is_same_type<optional_every::tag, optional_tag>);
static_assert(is_same_type<optional_hello::tag, optional_tag>);
static_assert(is_same_type<optional_world::tag, optional_tag>);

static_assert(is_same_type<optional<true>::type, boolean_t>);
static_assert(is_same_type<optional<false>::type, boolean_t>);
static_assert(is_same_type<optional<4.2>::type, number_t>);
static_assert(is_same_type<optional<-4.2>::type, number_t>);
static_assert(is_same_type<optional<"hello"_t>::type, string_t>);
static_assert(is_same_type<optional<"world!"_t>::type, string_t>);

// nullable

using nullable_boolean = nullable<boolean_t>;
using nullable_number = nullable<number_t>;
using nullable_string = nullable<string_t>;
using nullable_object = nullable<object_t>;
using nullable_array = nullable<array_t>;

static_assert(is_same_type<nullable_boolean::tag, nullable_tag>);
static_assert(is_same_type<nullable_number::tag, nullable_tag>);
static_assert(is_same_type<nullable_string::tag, nullable_tag>);
static_assert(is_same_type<nullable_object::tag, nullable_tag>);
static_assert(is_same_type<nullable_array::tag, nullable_tag>);

static_assert(is_same_type<nullable<boolean_t>::type, boolean_t>);
static_assert(is_same_type<nullable<number_t>::type, number_t>);
static_assert(is_same_type<nullable<string_t>::type, string_t>);
static_assert(is_same_type<nullable<object_t>::type, object_t>);
static_assert(is_same_type<nullable<array_t>::type, array_t>);

BOOST_AUTO_TEST_SUITE_END()
