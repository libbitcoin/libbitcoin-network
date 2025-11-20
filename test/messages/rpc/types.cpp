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

BOOST_AUTO_TEST_SUITE(rpc_types_tests)

using namespace rpc;

// optional<Default>
// ----------------------------------------------------------------------------

// optional<empty::array>
static_assert(is_optional<optional<empty::array>>);
static_assert(is_same_type<optional<empty::array>::type, array_t>);
static_assert(is_same_type<optional<empty::array>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<empty::array>{}.default_value()), array_t>);
////static_assert(optional<empty::array>{}.default_value() == array_t{});

// optional<empty::object>
static_assert(is_optional<optional<empty::object>>);
static_assert(is_same_type<optional<empty::object>::type, object_t>);
static_assert(is_same_type<optional<empty::object>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<empty::object>{}.default_value()), object_t>);
////static_assert(optional<empty::object>{}.default_value() == object_t{});

// optional<""_t>
static_assert(is_optional<optional<"default"_t>>);
static_assert(is_same_type<optional<"default"_t>::type, string_t>);
static_assert(is_same_type<optional<"default"_t>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<"default"_t>{}.default_value()), string_t>);
////static_assert(optional<"default"_t>{}.default_value() == "default");

// optional<true>
static_assert(is_optional<optional<true>>);
static_assert(is_same_type<optional<true>::type, boolean_t>);
static_assert(is_same_type<optional<true>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<true>{}.default_value()), boolean_t>);
static_assert(optional<true>{}.default_value() == true);

// optional<42_i8>
static_assert(is_optional<optional<42_i8>>);
static_assert(is_same_type<optional<42_i8>::type, int8_t>);
static_assert(is_same_type<optional<42_i8>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<42_i8>{}.default_value()), int8_t>);
static_assert(optional<42_i8>{}.default_value() == 42_i8);

// optional<42_i16>
static_assert(is_optional<optional<42_i16>>);
static_assert(is_same_type<optional<42_i16>::type, int16_t>);
static_assert(is_same_type<optional<42_i16>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<42_i16>{}.default_value()), int16_t>);
static_assert(optional<42_i16>{}.default_value() == 42_i16);

// optional<42_i32>
static_assert(is_optional<optional<42_i32>>);
static_assert(is_same_type<optional<42_i32>::type, int32_t>);
static_assert(is_same_type<optional<42_i32>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<42_i32>{}.default_value()), int32_t>);
static_assert(optional<42_i32>{}.default_value() == 42_i32);

// optional<42_i64>
static_assert(is_optional<optional<42_i64>>);
static_assert(is_same_type<optional<42_i64>::type, int64_t>);
static_assert(is_same_type<optional<42_i64>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<42_i64>{}.default_value()), int64_t>);
static_assert(optional<42_i64>{}.default_value() == 42_i64);

// optional<42_u8>
static_assert(is_optional<optional<42_u8>>);
static_assert(is_same_type<optional<42_u8>::type, uint8_t>);
static_assert(is_same_type<optional<42_u8>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<42_u8>{}.default_value()), uint8_t>);
static_assert(optional<42_u8>{}.default_value() == 42_u8);

// optional<42_u16>
static_assert(is_optional<optional<42_u16>>);
static_assert(is_same_type<optional<42_u16>::type, uint16_t>);
static_assert(is_same_type<optional<42_u16>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<42_u16>{}.default_value()), uint16_t>);
static_assert(optional<42_u16>{}.default_value() == 42_u16);

// optional<42_u32>
static_assert(is_optional<optional<42_u32>>);
static_assert(is_same_type<optional<42_u32>::type, uint32_t>);
static_assert(is_same_type<optional<42_u32>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<42_u32>{}.default_value()), uint32_t>);
static_assert(optional<42_u32>{}.default_value() == 42_u32);

// optional<42_u64>
static_assert(is_optional<optional<42_u64>>);
static_assert(is_same_type<optional<42_u64>::type, uint64_t>);
static_assert(is_same_type<optional<42_u64>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<42_u64>{}.default_value()), uint64_t>);
static_assert(optional<42_u64>{}.default_value() == 42_u64);

// optional<42>
static_assert(is_optional<optional<42>>);
static_assert(is_same_type<optional<42>::type, int>);
static_assert(is_same_type<optional<42>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<42>{}.default_value()), int> );
static_assert(optional<42>{}.default_value() == 42);

// optional<4.2> (double/float literals consolidated into number_t)
static_assert(is_optional<optional<4.2>>);
static_assert(is_optional<optional<4.2f>>);
static_assert(is_same_type<optional<4.2>::type, number_t>);
static_assert(is_same_type<optional<4.2>::tag, optional_tag>);
static_assert(is_same_type<optional<4.2f>::type, number_t>);
static_assert(is_same_type<optional<4.2f>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<4.2>{}.default_value()), number_t>);
static_assert(optional<4.2>{}.default_value() == 4.2);

static_assert(!is_optional<array_t>);
static_assert(!is_optional<object_t>);
static_assert(!is_optional<number_t>);
static_assert(!is_optional<string_t>);
static_assert(!is_optional<boolean_t>);

// nullable<Type>
// ----------------------------------------------------------------------------

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

static_assert(is_nullable<nullable<boolean_t>>);
static_assert(is_nullable<nullable<boolean_t>>);
static_assert(is_nullable<nullable<boolean_t>>);
static_assert(is_nullable<nullable<boolean_t>>);
static_assert(is_nullable<nullable<boolean_t>>);

static_assert(!is_nullable<boolean_t>);
static_assert(!is_nullable<number_t>);
static_assert(!is_nullable<string_t>);
static_assert(!is_nullable<object_t>);
static_assert(!is_nullable<array_t>);

// is_required<Tuple>
// ----------------------------------------------------------------------------

static_assert(!is_required<optional<empty::array>>);
static_assert(!is_required<optional<empty::object>>);
static_assert(!is_required<optional<4.2f>>);
static_assert(!is_required<optional<4.2>>);
static_assert(!is_required<optional<42>>);
static_assert(!is_required<optional<"default"_t>>);
static_assert(!is_required<optional<true>>);

static_assert(is_required<array_t>);
static_assert(is_required<object_t>);
static_assert(is_required<number_t>);
static_assert(is_required<string_t>);
static_assert(is_required<boolean_t>);

static_assert(!is_required<nullable<array_t>>);
static_assert(!is_required<nullable<object_t>>);
static_assert(!is_required<nullable<number_t>>);
static_assert(!is_required<nullable<string_t>>);
static_assert(!is_required<nullable<boolean_t>>);

static_assert(is_required<array_t>);
static_assert(is_required<object_t>);
static_assert(is_required<number_t>);
static_assert(is_required<string_t>);
static_assert(is_required<boolean_t>);

// internal_t<Argument>
// ----------------------------------------------------------------------------

static_assert(is_same_type<internal_t<array_t>, array_t>);
static_assert(is_same_type<internal_t<nullable<array_t>>, array_t>);
static_assert(is_same_type<internal_t<optional<empty::array>>, array_t>);

static_assert(is_same_type<internal_t<object_t>, object_t>);
static_assert(is_same_type<internal_t<nullable<object_t>>, object_t>);
static_assert(is_same_type<internal_t<optional<empty::object>>, object_t>);

static_assert(is_same_type<internal_t<number_t>, number_t>);
static_assert(is_same_type<internal_t<nullable<number_t>>, number_t>);
static_assert(is_same_type<internal_t<optional<4.2>>, number_t>);

static_assert(is_same_type<internal_t<string_t>, string_t>);
static_assert(is_same_type<internal_t<nullable<string_t>>, string_t>);
static_assert(is_same_type<internal_t<optional<"42"_t>>, string_t>);

static_assert(is_same_type<internal_t<boolean_t>, boolean_t>);
static_assert(is_same_type<internal_t<nullable<boolean_t>>, boolean_t>);
static_assert(is_same_type<internal_t<optional<true>>, boolean_t>);

// external_t<Argument>
// ----------------------------------------------------------------------------

static_assert(is_same_type<external_t<array_t>, array_t>);
static_assert(is_same_type<external_t<nullable<array_t>>, std::optional<array_t>>);
static_assert(is_same_type<external_t<optional<empty::array>>, array_t>);

static_assert(is_same_type<external_t<object_t>, object_t>);
static_assert(is_same_type<external_t<nullable<object_t>>, std::optional<object_t>>);
static_assert(is_same_type<external_t<optional<empty::object>>, object_t>);

static_assert(is_same_type<external_t<number_t>, number_t>);
static_assert(is_same_type<external_t<nullable<number_t>>, std::optional<number_t>>);
static_assert(is_same_type<external_t<optional<4.2>>, number_t>);

static_assert(is_same_type<external_t<string_t>, string_t>);
static_assert(is_same_type<external_t<nullable<string_t>>, std::optional<string_t>>);
static_assert(is_same_type<external_t<optional<"42"_t>>, string_t>);

static_assert(is_same_type<external_t<boolean_t>, boolean_t>);
static_assert(is_same_type<external_t<nullable<boolean_t>>, std::optional<boolean_t>>);
static_assert(is_same_type<external_t<optional<true>>, boolean_t>);

// externals_t<Arguments>
// ----------------------------------------------------------------------------

static_assert(is_same_tuple<externals_t<std::tuple<>>, std::tuple<>>);
static_assert(is_same_tuple<externals_t<std::tuple<bool>>, std::tuple<bool>>);
static_assert(is_same_tuple<externals_t<std::tuple<array_t, object_t>>, std::tuple<array_t, object_t>>);
static_assert(is_same_tuple<externals_t<std::tuple<optional<true>, optional<42>>>, std::tuple<bool, int>>);
static_assert(is_same_tuple<externals_t<std::tuple<nullable<bool>, nullable<double>>>, std::tuple<std::optional<bool>, std::optional<double>>>);
static_assert(is_same_tuple<externals_t<std::tuple<optional<true>, nullable<double>, optional<empty::array>>>, std::tuple<bool, std::optional<double>, array_t>>);

// externals_t does not decay
static_assert(is_same_tuple<externals_t<std::tuple<const bool>>, std::tuple<bool>>);
static_assert(!is_same_tuple<externals_t<std::tuple<>>, std::tuple<bool>>);

// only_trailing_optionals<Args...>
// ----------------------------------------------------------------------------

static_assert( only_trailing_optionals<>);
static_assert( only_trailing_optionals<bool>);
static_assert( only_trailing_optionals<optional<true>>);
static_assert( only_trailing_optionals<bool, optional<true>>);
static_assert( only_trailing_optionals<int, bool, optional<true>>);
static_assert( only_trailing_optionals<optional<true>, optional<42>>);
static_assert( only_trailing_optionals<int, optional<true>, optional<4.2>>);
static_assert(!only_trailing_optionals<optional<true>, bool>);
static_assert(!only_trailing_optionals<bool, optional<true>, bool>);
static_assert(!only_trailing_optionals<optional<true>, optional<42u>, bool>);

// is_tagged<Tuple>
// ----------------------------------------------------------------------------

static_assert(!is_tagged<std::tuple<>>);
static_assert(!is_tagged<std::tuple<bool>>);
static_assert(!is_tagged<std::tuple<bool, int>>);
static_assert(!is_tagged<std::tuple<bool, int, std::string>>);
static_assert(!is_tagged<std::tuple<bool, int, std::shared_ptr<int>>>);
static_assert( is_tagged<std::tuple<std::shared_ptr<int>>>);
static_assert( is_tagged<std::tuple<std::shared_ptr<const int>>>);
static_assert( is_tagged<std::tuple<const std::shared_ptr<const int>>>);
static_assert( is_tagged<std::tuple<const std::shared_ptr<const int>&>>);
static_assert( is_tagged<std::tuple<std::shared_ptr<const int>&&>>);
static_assert( is_tagged<std::tuple<std::shared_ptr<int>, std::string, bool>>);

// handler_args_t<Handler>
// ----------------------------------------------------------------------------

static_assert(is_same_tuple<handler_args_t<std::function<void(code, bool)>>, std::tuple<bool>>);
static_assert(is_same_tuple<handler_args_t<std::function<void(const code&, bool&&, const std::string&)>>, std::tuple<bool, std::string>>);

BOOST_AUTO_TEST_SUITE_END()
