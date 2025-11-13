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

BOOST_AUTO_TEST_SUITE_END()
