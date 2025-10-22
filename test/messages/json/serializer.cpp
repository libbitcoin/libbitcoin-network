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

#include <variant>

BOOST_AUTO_TEST_SUITE(serializer_tests)

using namespace network::json;

BOOST_AUTO_TEST_CASE(serializer__serialize__deserialized_request__expected)
{
    const string_t text
    {
        R"({)"
            R"("jsonrpc":"2.0",)"
            R"("id":-42,)"
            R"("method":"random",)"
            R"("params":)"
            R"({)"
                R"("array":[A],)"
                R"("false":false,)"
                R"("foo":"bar",)"
                R"("null":null,)"
                R"("number":42,)"
                R"("object":{O},)"
                R"("true":true)"
            R"(})"
        R"(})"
    };

    parser parse{};
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(parse);

    // params are sorted by serializer, so must be above as well.
    BOOST_REQUIRE_EQUAL(serialize(parse.get()), text);
}

BOOST_AUTO_TEST_CASE(response_serializer__serialize__simple_result__expected)
{
    response_t response;
    response.jsonrpc = version::v2;
    response.id = id_t{ code_t{ 42 } };
    response.result = value_t{ std::in_place_type<number_t>, number_t{ 100.5 } };

    const auto json = serialize(response);
    BOOST_CHECK_EQUAL(json, R"({"jsonrpc":"2.0","id":42,"result":100.5})");
}

BOOST_AUTO_TEST_CASE(response_serializer__serialize__error_response__expected)
{
    response_t response;
    response.jsonrpc = version::v2;
    response.id = id_t{ string_t{ "abc123" } };
    response.error = result_t{ -32602, "Invalid params", {} };

    const auto json = serialize(response);
    BOOST_CHECK_EQUAL(json, R"({"jsonrpc":"2.0","id":"abc123","error":{"code":-32602,"message":"Invalid params"}})");
}

BOOST_AUTO_TEST_CASE(response_serializer__serialize__error_with_data__expected)
{
    response_t response;
    response.jsonrpc = version::v1;
    response.id = id_t{ null_t{} };
    response.error = result_t{ -32700, "Parse error", value_t{std::in_place_type<string_t>, string_t{ "Invalid JSON" }} };

    const auto json = serialize(response);
    BOOST_CHECK_EQUAL(json, R"({"jsonrpc":"1.0","id":null,"error":{"code":-32700,"message":"Parse error","data":"Invalid JSON"}})");
}

BOOST_AUTO_TEST_CASE(response_serializer__serialize__empty_result__expected)
{
    response_t response;
    response.jsonrpc = version::v2;
    response.id = id_t{ code_t{} };
    response.result = value_t{ std::in_place_type<null_t>, null_t{} };

    const auto json = serialize(response);
    BOOST_CHECK_EQUAL(json, R"({"jsonrpc":"2.0","id":0,"result":null})");
}

BOOST_AUTO_TEST_SUITE_END()
