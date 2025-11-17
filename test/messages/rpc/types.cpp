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

BOOST_AUTO_TEST_SUITE(rpc_types_tests)

using namespace rpc;

// optional

static_assert(is_same_type<optional<empty::array>::type, array_t>);
static_assert(is_same_type<optional<empty::array>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<empty::array>{}.default_value()), array_t>);
////static_assert(optional<empty::array>{}.default_value() == array_t{});

static_assert(is_same_type<optional<empty::object>::type, object_t>);
static_assert(is_same_type<optional<empty::object>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<empty::object>{}.default_value()), object_t>);
////static_assert(optional<empty::object>{}.default_value() == object_t{});

static_assert(is_same_type<optional<"default"_t>::type, string_t>);
static_assert(is_same_type<optional<"default"_t>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<"default"_t>{}.default_value()), string_t>);
////static_assert(optional<"default"_t>{}.default_value() == "default");

static_assert(is_same_type<optional<true>::type, boolean_t>);
static_assert(is_same_type<optional<true>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<true>{}.default_value()), boolean_t>);
static_assert(optional<true>{}.default_value() == true);

static_assert(is_same_type<optional<4.2>::type, number_t>);
static_assert(is_same_type<optional<4.2>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<4.2>{}.default_value()), number_t>);
static_assert(optional<4.2>{}.default_value() == 4.2);

// nullable

static_assert(is_same_type<nullable<boolean_t>::type, boolean_t>);
static_assert(is_same_type<nullable<number_t>::type, number_t>);
static_assert(is_same_type<nullable<string_t>::type, string_t>);
static_assert(is_same_type<nullable<object_t>::type, object_t>);
static_assert(is_same_type<nullable<array_t>::type, array_t>);

static_assert(is_same_type<nullable<boolean_t>::tag, nullable_tag>);
static_assert(is_same_type<nullable<number_t>::tag, nullable_tag>);
static_assert(is_same_type<nullable<string_t>::tag, nullable_tag>);
static_assert(is_same_type<nullable<object_t>::tag, nullable_tag>);
static_assert(is_same_type<nullable<array_t>::tag, nullable_tag>);

BOOST_AUTO_TEST_SUITE_END()
