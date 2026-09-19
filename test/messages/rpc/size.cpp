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

BOOST_AUTO_TEST_SUITE(rpc_size_tests)

using namespace rpc;

constexpr auto element = element_size;
constexpr auto envelope = envelope_size;

// value_t
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_size__value__null__element)
{
    BOOST_REQUIRE_EQUAL(to_size(value_t{}), element);
}

BOOST_AUTO_TEST_CASE(rpc_size__value__boolean__element)
{
    BOOST_REQUIRE_EQUAL(to_size(value_t{ true }), element);
}

BOOST_AUTO_TEST_CASE(rpc_size__value__number__element)
{
    BOOST_REQUIRE_EQUAL(to_size(value_t{ 42_u64 }), element);
}

BOOST_AUTO_TEST_CASE(rpc_size__value__empty_text__element)
{
    BOOST_REQUIRE_EQUAL(to_size(value_t{ string_t{} }), element);
}

BOOST_AUTO_TEST_CASE(rpc_size__value__text__text_size_plus_element)
{
    BOOST_REQUIRE_EQUAL(to_size(value_t{ string_t{ "abcde" } }), 5u + element);
}

// array_t
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_size__array__empty__element)
{
    BOOST_REQUIRE_EQUAL(to_size(array_t{}), element);
}

BOOST_AUTO_TEST_CASE(rpc_size__array__two_numbers__three_elements)
{
    BOOST_REQUIRE_EQUAL(to_size(array_t{ value_t{ 1_u64 }, value_t{ 2_u64 } }), 3u * element);
}

BOOST_AUTO_TEST_CASE(rpc_size__array__text__text_size_plus_two_elements)
{
    BOOST_REQUIRE_EQUAL(to_size(array_t{ value_t{ string_t{ "abc" } } }), 3u + 2u * element);
}

BOOST_AUTO_TEST_CASE(rpc_size__array__nested_array__accumulates_nested)
{
    const array_t nested{ value_t{ string_t{ "abc" } } };
    BOOST_REQUIRE_EQUAL(to_size(array_t{ value_t{ nested } }), 3u + 3u * element);
}

// object_t
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_size__object__empty__element)
{
    BOOST_REQUIRE_EQUAL(to_size(object_t{}), element);
}

BOOST_AUTO_TEST_CASE(rpc_size__object__keyed_number__key_size_plus_two_elements)
{
    BOOST_REQUIRE_EQUAL(to_size(object_t{ { "key", value_t{ 1_u64 } } }), 3u + 2u * element);
}

BOOST_AUTO_TEST_CASE(rpc_size__object__keyed_text__key_and_text_sizes_plus_two_elements)
{
    BOOST_REQUIRE_EQUAL(to_size(object_t{ { "key", value_t{ string_t{ "value" } } } }), 3u + 5u + 2u * element);
}

// params_option
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_size__params__none__zero)
{
    BOOST_REQUIRE_EQUAL(to_size(params_option{}), zero);
}

BOOST_AUTO_TEST_CASE(rpc_size__params__value__value_size)
{
    const params_option params{ value_t{ string_t{ "abc" } } };
    BOOST_REQUIRE_EQUAL(to_size(params), 3u + element);
}

BOOST_AUTO_TEST_CASE(rpc_size__params__array__array_size)
{
    const params_option params{ array_t{ value_t{ string_t{ "abc" } } } };
    BOOST_REQUIRE_EQUAL(to_size(params), 3u + 2u * element);
}

BOOST_AUTO_TEST_CASE(rpc_size__params__object__object_size)
{
    const params_option params{ object_t{ { "key", value_t{ 1_u64 } } } };
    BOOST_REQUIRE_EQUAL(to_size(params), 3u + 2u * element);
}

// response_t
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_size__response__empty__envelope)
{
    BOOST_REQUIRE_EQUAL(to_size(response_t{}), envelope);
}

BOOST_AUTO_TEST_CASE(rpc_size__response__text_result__envelope_plus_result)
{
    const response_t message{ .result = value_t{ string_t{ "abcde" } } };
    BOOST_REQUIRE_EQUAL(to_size(message), envelope + 5u + element);
}

BOOST_AUTO_TEST_CASE(rpc_size__response__error__envelope_plus_message_and_element)
{
    const response_t message{ .error = result_t{ .message = "abcde" } };
    BOOST_REQUIRE_EQUAL(to_size(message), envelope + 5u + element);
}

BOOST_AUTO_TEST_CASE(rpc_size__response__error_data__envelope_plus_message_data_and_elements)
{
    const response_t message{ .error = result_t{ .message = "abcde", .data = value_t{ string_t{ "xyz" } } } };
    BOOST_REQUIRE_EQUAL(to_size(message), envelope + 5u + 3u + 2u * element);
}

// request_t
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_size__request__empty__envelope)
{
    BOOST_REQUIRE_EQUAL(to_size(request_t{}), envelope);
}

BOOST_AUTO_TEST_CASE(rpc_size__request__method__envelope_plus_method)
{
    const request_t message{ .method = "getblock" };
    BOOST_REQUIRE_EQUAL(to_size(message), envelope + 8u);
}

BOOST_AUTO_TEST_CASE(rpc_size__request__method_and_params__envelope_plus_method_and_params)
{
    const request_t message{ .method = "getblock", .params = array_t{ value_t{ string_t{ "abc" } } } };
    BOOST_REQUIRE_EQUAL(to_size(message), envelope + 8u + 3u + 2u * element);
}

// The estimate is not less than the serialized size of a text payload.
BOOST_AUTO_TEST_CASE(rpc_size__response__large_text_result__not_less_than_serialized)
{
    const string_t text(1'000, 'x');
    const response_t message{ .jsonrpc = version::v2, .id = identity_t{ 42_i64 }, .result = value_t{ text } };
    BOOST_REQUIRE_GE(to_size(message), boost::json::serialize(value_from(message)).size());
}

BOOST_AUTO_TEST_SUITE_END()
