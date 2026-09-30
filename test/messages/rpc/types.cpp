/**
 * Copyright (c) 2011-2026 libbitcoin developers
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

// boolean_t defined in global namespace by:
// Applications/Xcode_16.4.app/Contents/Developer/Platforms/MacOSX.platform/
// Developer/SDKs/MacOSX.sdk/usr/include/mach/arm/boolean.h:70:25
using boolean_type = bc::network::rpc::boolean_t;

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
static_assert(is_same_type<optional<true>::type, boolean_type>);
static_assert(is_same_type<optional<true>::tag, optional_tag>);
static_assert(is_same_type<decltype(optional<true>{}.default_value()), boolean_type>);
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
static_assert(!is_optional<boolean_type>);

// nullable<Type>
// ----------------------------------------------------------------------------

static_assert(is_same_type<nullable<boolean_type>::type, boolean_type>);
static_assert(is_same_type<nullable<number_t>::type, number_t>);
static_assert(is_same_type<nullable<string_t>::type, string_t>);
static_assert(is_same_type<nullable<object_t>::type, object_t>);
static_assert(is_same_type<nullable<array_t>::type, array_t>);

static_assert(is_same_type<nullable<boolean_type>::tag, nullable_tag>);
static_assert(is_same_type<nullable<number_t>::tag, nullable_tag>);
static_assert(is_same_type<nullable<string_t>::tag, nullable_tag>);
static_assert(is_same_type<nullable<object_t>::tag, nullable_tag>);
static_assert(is_same_type<nullable<array_t>::tag, nullable_tag>);

static_assert(is_nullable<nullable<boolean_type>>);
static_assert(is_nullable<nullable<boolean_type>>);
static_assert(is_nullable<nullable<boolean_type>>);
static_assert(is_nullable<nullable<boolean_type>>);
static_assert(is_nullable<nullable<boolean_type>>);

static_assert(!is_nullable<boolean_type>);
static_assert(!is_nullable<number_t>);
static_assert(!is_nullable<string_t>);
static_assert(!is_nullable<object_t>);
static_assert(!is_nullable<array_t>);

// nullopt<Default>
// ----------------------------------------------------------------------------

static_assert(is_same_type<nullopt<empty::array>::type, array_t>);
static_assert(is_same_type<nullopt<empty::object>::type, object_t>);
static_assert(is_same_type<nullopt<empty::value>::type, value_t>);
static_assert(is_same_type<nullopt<"default"_t>::type, string_t>);
static_assert(is_same_type<nullopt<true>::type, boolean_type>);
static_assert(is_same_type<nullopt<42_i32>::type, int32_t>);
static_assert(is_same_type<nullopt<4.2>::type, number_t>);

static_assert(is_same_type<nullopt<empty::array>::tag, nullopt_tag>);
static_assert(is_same_type<nullopt<"default"_t>::tag, nullopt_tag>);
static_assert(is_same_type<nullopt<true>::tag, nullopt_tag>);
static_assert(is_same_type<nullopt<4.2>::tag, nullopt_tag>);

static_assert(nullopt<4.2>::default_value() == 4.2);
static_assert(nullopt<true>::default_value());
static_assert(nullopt<42_i32>::default_value() == 42);

static_assert(is_nullopt<nullopt<empty::array>>);
static_assert(is_nullopt<nullopt<"default"_t>>);
static_assert(is_nullopt<nullopt<true>>);
static_assert(is_nullopt<nullopt<4.2>>);

static_assert(!is_nullopt<optional<4.2>>);
static_assert(!is_nullopt<nullable<number_t>>);
static_assert(!is_nullopt<number_t>);
static_assert(!is_optional<nullopt<4.2>>);
static_assert(!is_nullable<nullopt<4.2>>);

static_assert(is_defaulted<optional<4.2>>);
static_assert(is_defaulted<nullopt<4.2>>);
static_assert(!is_defaulted<nullable<number_t>>);
static_assert(!is_defaulted<number_t>);

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
static_assert(is_required<boolean_type>);

static_assert(!is_required<nullable<array_t>>);
static_assert(!is_required<nullable<object_t>>);
static_assert(!is_required<nullable<number_t>>);
static_assert(!is_required<nullable<string_t>>);
static_assert(!is_required<nullable<boolean_type>>);

static_assert(!is_required<nullopt<empty::array>>);
static_assert(!is_required<nullopt<"default"_t>>);
static_assert(!is_required<nullopt<true>>);
static_assert(!is_required<nullopt<4.2>>);

static_assert(is_required<array_t>);
static_assert(is_required<object_t>);
static_assert(is_required<number_t>);
static_assert(is_required<string_t>);
static_assert(is_required<boolean_type>);

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

static_assert(is_same_type<internal_t<boolean_type>, boolean_type>);
static_assert(is_same_type<internal_t<nullable<boolean_type>>, boolean_type>);
static_assert(is_same_type<internal_t<optional<true>>, boolean_type>);

static_assert(is_same_type<internal_t<nullopt<empty::array>>, array_t>);
static_assert(is_same_type<internal_t<nullopt<4.2>>, number_t>);
static_assert(is_same_type<internal_t<nullopt<"42"_t>>, string_t>);
static_assert(is_same_type<internal_t<nullopt<true>>, boolean_type>);

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

static_assert(is_same_type<external_t<boolean_type>, boolean_type>);
static_assert(is_same_type<external_t<nullable<boolean_type>>, std::optional<boolean_type>>);
static_assert(is_same_type<external_t<optional<true>>, boolean_type>);

static_assert(is_same_type<external_t<nullopt<empty::array>>, array_t>);
static_assert(is_same_type<external_t<nullopt<4.2>>, number_t>);
static_assert(is_same_type<external_t<nullopt<"42"_t>>, string_t>);
static_assert(is_same_type<external_t<nullopt<true>>, boolean_type>);

// externals_t<Arguments>
// ----------------------------------------------------------------------------

static_assert(is_same_tuple<externals_t<std::tuple<>>, std::tuple<>>);
static_assert(is_same_tuple<externals_t<std::tuple<bool>>, std::tuple<bool>>);
static_assert(is_same_tuple<externals_t<std::tuple<array_t, object_t>>, std::tuple<array_t, object_t>>);
static_assert(is_same_tuple<externals_t<std::tuple<optional<true>, optional<42>>>, std::tuple<bool, int>>);
static_assert(is_same_tuple<externals_t<std::tuple<nullable<bool>, nullable<double>>>, std::tuple<std::optional<bool>, std::optional<double>>>);
static_assert(is_same_tuple<externals_t<std::tuple<optional<true>, nullable<double>, optional<empty::array>>>, std::tuple<bool, std::optional<double>, array_t>>);
static_assert(is_same_tuple<externals_t<std::tuple<nullopt<true>, nullable<double>, nullopt<empty::array>>>, std::tuple<bool, std::optional<double>, array_t>>);

// externals_t does not decay
static_assert(is_same_tuple<externals_t<std::tuple<const bool>>, std::tuple<bool>>);
static_assert(!is_same_tuple<externals_t<std::tuple<>>, std::tuple<bool>>);

// only_trailing_unrequireds<Args...>
// ----------------------------------------------------------------------------

static_assert( only_trailing_unrequireds<>);
static_assert( only_trailing_unrequireds<bool>);

static_assert( only_trailing_unrequireds<optional<true>>);
static_assert( only_trailing_unrequireds<bool, optional<true>>);
static_assert( only_trailing_unrequireds<int, bool, optional<true>>);
static_assert( only_trailing_unrequireds<optional<true>, optional<42>>);
static_assert( only_trailing_unrequireds<int, optional<true>, optional<4.2>>);
static_assert(!only_trailing_unrequireds<optional<true>, bool>);
static_assert(!only_trailing_unrequireds<bool, optional<true>, bool>);
static_assert(!only_trailing_unrequireds<optional<true>, optional<42u>, bool>);

static_assert( only_trailing_unrequireds<nullable<bool>>);
static_assert( only_trailing_unrequireds<nullopt<true>>);
static_assert( only_trailing_unrequireds<bool, nullopt<true>, optional<4.2>>);
static_assert( only_trailing_unrequireds<bool, optional<true>, nullopt<4.2>>);
static_assert(!only_trailing_unrequireds<nullopt<true>, bool>);
static_assert( only_trailing_unrequireds<bool, nullable<bool>>);
static_assert( only_trailing_unrequireds<int, bool, nullable<bool>>);
static_assert( only_trailing_unrequireds<nullable<bool>, nullable<int32_t>>);
static_assert( only_trailing_unrequireds<int, nullable<bool>, nullable<double>>);
static_assert(!only_trailing_unrequireds<nullable<bool>, bool>);
static_assert(!only_trailing_unrequireds<bool, nullable<bool>, bool>);
static_assert(!only_trailing_unrequireds<nullable<bool>, nullable<uint32_t>, bool>);

static_assert( only_trailing_unrequireds<bool, optional<true>, nullable<uint32_t>>);
static_assert( only_trailing_unrequireds<bool, nullable<uint32_t>, optional<true>>);
static_assert(!only_trailing_unrequireds<optional<true>, nullable<uint32_t>, bool>);
static_assert(!only_trailing_unrequireds<nullable<uint32_t>, optional<true>, bool>);

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

// model
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(rpc_model_tests)

BOOST_AUTO_TEST_CASE(rpc_model__version_to__strings__expected)
{
    BOOST_REQUIRE(boost::json::value_to<version>(boost::json::value("1.0")) == version::v1);
    BOOST_REQUIRE(boost::json::value_to<version>(boost::json::value("2.0")) == version::v2);
    BOOST_REQUIRE(boost::json::value_to<version>(boost::json::value("3.0")) == version::invalid);
    BOOST_REQUIRE(boost::json::value_to<version>(boost::json::value(2.0)) == version::invalid);
}

BOOST_AUTO_TEST_CASE(rpc_model__version_from__versions__expected)
{
    BOOST_REQUIRE_EQUAL(boost::json::value_from(version::v1).as_string(), "1.0");
    BOOST_REQUIRE_EQUAL(boost::json::value_from(version::v2).as_string(), "2.0");
}

BOOST_AUTO_TEST_CASE(rpc_model__value_to__json_types__expected_alternatives)
{
    BOOST_REQUIRE(std::holds_alternative<null_t>(boost::json::value_to<value_t>(boost::json::parse("null")).value()));
    BOOST_REQUIRE(std::get<boolean_type>(boost::json::value_to<value_t>(boost::json::parse("true")).value()));
    BOOST_REQUIRE_EQUAL(std::get<number_t>(boost::json::value_to<value_t>(boost::json::parse("-7")).value()), -7.0);
    BOOST_REQUIRE_EQUAL(std::get<string_t>(boost::json::value_to<value_t>(boost::json::parse(R"("a")")).value()), "a");
    BOOST_REQUIRE_EQUAL(std::get<array_t>(boost::json::value_to<value_t>(boost::json::parse("[1,2]")).value()).size(), 2u);
    BOOST_REQUIRE_EQUAL(std::get<object_t>(boost::json::value_to<value_t>(boost::json::parse(R"({"a":1})")).value()).size(), 1u);
}

BOOST_AUTO_TEST_CASE(rpc_model__identity_to__json_types__expected_alternatives)
{
    BOOST_REQUIRE(std::holds_alternative<null_t>(boost::json::value_to<identity_t>(boost::json::parse("null"))));
    BOOST_REQUIRE_EQUAL(std::get<code_t>(boost::json::value_to<identity_t>(boost::json::parse("42"))), 42);
    BOOST_REQUIRE_EQUAL(std::get<string_t>(boost::json::value_to<identity_t>(boost::json::parse(R"("42")"))), "42");
}

BOOST_AUTO_TEST_CASE(rpc_model__identity_to__boolean__throws)
{
    BOOST_REQUIRE_THROW(boost::json::value_to<identity_t>(boost::json::parse("true")), ostream_exception);
}

BOOST_AUTO_TEST_CASE(rpc_model__identity_from__null__null)
{
    BOOST_REQUIRE(boost::json::value_from(identity_t{ null_t{} }).is_null());
}

BOOST_AUTO_TEST_CASE(rpc_model__response_to__v2_error_with_data__expected)
{
    const auto model = boost::json::parse(R"({"jsonrpc":"2.0","id":"1","error":{"code":-32600,"message":"Invalid Request","data":[1,2]}})");
    const auto response = boost::json::value_to<response_t>(model);
    BOOST_REQUIRE(response.jsonrpc == version::v2);
    BOOST_REQUIRE_EQUAL(std::get<string_t>(response.id.value()), "1");
    BOOST_REQUIRE(!response.result.has_value());
    BOOST_REQUIRE(response.error.has_value());
    BOOST_REQUIRE_EQUAL(response.error.value().code, -32600);
    BOOST_REQUIRE_EQUAL(response.error.value().message, "Invalid Request");
    BOOST_REQUIRE_EQUAL(std::get<array_t>(response.error.value().data.value().value()).size(), 2u);
}

BOOST_AUTO_TEST_CASE(rpc_model__response_to__v2_error_without_data_null_id__expected)
{
    const auto model = boost::json::parse(R"({"jsonrpc":"2.0","error":{"code":-32700,"message":"Parse error"},"id":null})");
    const auto response = boost::json::value_to<response_t>(model);
    BOOST_REQUIRE(response.jsonrpc == version::v2);
    BOOST_REQUIRE(std::holds_alternative<null_t>(response.id.value()));
    BOOST_REQUIRE_EQUAL(response.error.value().code, -32700);
    BOOST_REQUIRE_EQUAL(response.error.value().message, "Parse error");
    BOOST_REQUIRE(!response.error.value().data.has_value());
}

BOOST_AUTO_TEST_CASE(rpc_model__response_to__v2_result_numeric_id__expected)
{
    const auto model = boost::json::parse(R"({"jsonrpc":"2.0","result":19,"id":1})");
    const auto response = boost::json::value_to<response_t>(model);
    BOOST_REQUIRE(response.jsonrpc == version::v2);
    BOOST_REQUIRE_EQUAL(std::get<code_t>(response.id.value()), 1);
    BOOST_REQUIRE(!response.error.has_value());
    BOOST_REQUIRE_EQUAL(std::get<number_t>(response.result.value().value()), 19.0);
}

BOOST_AUTO_TEST_CASE(rpc_model__response_to__empty_object__defaults)
{
    const auto response = boost::json::value_to<response_t>(boost::json::parse("{}"));
    BOOST_REQUIRE(response.jsonrpc == version::undefined);
    BOOST_REQUIRE(!response.id.has_value());
    BOOST_REQUIRE(!response.error.has_value());
    BOOST_REQUIRE(!response.result.has_value());
}

BOOST_AUTO_TEST_CASE(rpc_model__response_from__v1_result__null_error)
{
    const auto model = boost::json::value_from(response_t{ version::v1, identity_t{ 1 }, {}, value_t{ true } });
    const auto& object = model.as_object();
    BOOST_REQUIRE(object.at("result").as_bool());
    BOOST_REQUIRE(object.at("error").is_null());
    BOOST_REQUIRE_EQUAL(object.at("id").as_int64(), 1);
}

BOOST_AUTO_TEST_CASE(rpc_model__response_from__v1_error__null_result)
{
    const auto model = boost::json::value_from(response_t{ version::v1, identity_t{ 1 }, result_t{ -32601, "Method not found", {} }, {} });
    const auto& object = model.as_object();
    BOOST_REQUIRE(object.at("result").is_null());
    BOOST_REQUIRE_EQUAL(object.at("error").as_object().at("code").as_int64(), -32601);
    BOOST_REQUIRE_EQUAL(object.at("error").as_object().at("message").as_string(), "Method not found");
}

BOOST_AUTO_TEST_CASE(rpc_model__response_from__v2_result__no_error)
{
    const auto model = boost::json::value_from(response_t{ version::v2, identity_t{ 1 }, {}, value_t{ true } });
    const auto& object = model.as_object();
    BOOST_REQUIRE(object.at("result").as_bool());
    BOOST_REQUIRE(!object.contains("error"));
    BOOST_REQUIRE_EQUAL(object.at("jsonrpc").as_string(), "2.0");
}

BOOST_AUTO_TEST_CASE(rpc_model__request_from__null_id_value_params__expected)
{
    const auto model = boost::json::value_from(request_t{ version::v2, identity_t{ null_t{} }, "m", params_t{ value_t{ string_t{ "x" } } } });
    BOOST_REQUIRE_EQUAL(boost::json::serialize(model), R"({"jsonrpc":"2.0","id":null,"method":"m","params":"x"})");
}

BOOST_AUTO_TEST_SUITE_END()
