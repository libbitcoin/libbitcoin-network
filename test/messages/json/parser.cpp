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

BOOST_AUTO_TEST_SUITE(parser_tests)

using namespace network::json;

template <bool Request, bool Strict, version Require>
struct test_parser
  : public parser<Request, Strict, Require>
{
    using base = parser<Request, Strict, Require>;
    using base::base;
};

using lax_request_parser = test_parser<true, false, version::any>;
using request_parser = test_parser<true, true, version::any>;
static_assert(request_parser::request);
static_assert(!request_parser::response);

using namespace boost::system::errc;
static auto incomplete = make_error_code(interrupted);
static auto failure = make_error_code(invalid_argument);

// jsonrpc v1/v2

BOOST_AUTO_TEST_CASE(request_parser__write__jsonrpc_empty__error)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":""})" };
    const auto size = parse.write(text);
    BOOST_CHECK(parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, text.size());
    BOOST_REQUIRE(parse.get_parsed().empty());
}

BOOST_AUTO_TEST_CASE(request_parser__write__jsonrpc_null__error)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":null})" };
    const auto size = parse.write(text);
    BOOST_CHECK(parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, 12u);
    BOOST_REQUIRE(parse.get_parsed().empty());
}

BOOST_AUTO_TEST_CASE(request_parser__write__jsonrpc_numeric__error)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":42})" };
    const auto size = parse.write(text);
    BOOST_CHECK(parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, 12u);
    BOOST_REQUIRE(parse.get_parsed().empty());
}

BOOST_AUTO_TEST_CASE(request_parser__write__jsonrpc_v2__expected)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":"2.0"})" };
    const auto size = parse.write(text);
    BOOST_CHECK(!parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_CHECK(request.jsonrpc == version::v2);
}

// id

BOOST_AUTO_TEST_CASE(request_parser__write__id_positive__expected)
{
    request_parser parse{};
    const string_t text{ R"({"id":42})" };
    const auto size = parse.write(text);
    BOOST_CHECK(!parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_CHECK(request.jsonrpc == version::undefined);
    BOOST_CHECK(std::holds_alternative<code_t>(request.id));
    BOOST_REQUIRE_EQUAL(std::get<code_t>(request.id), 42);
}

BOOST_AUTO_TEST_CASE(request_parser__write__id_negative__expected)
{
    request_parser parse{};
    const string_t text{ R"({"id":-42})" };
    const auto size = parse.write(text);
    BOOST_CHECK(!parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_CHECK(request.jsonrpc == version::undefined);
    BOOST_CHECK(std::holds_alternative<code_t>(request.id));
    BOOST_REQUIRE_EQUAL(std::get<code_t>(request.id), -42);
}

BOOST_AUTO_TEST_CASE(request_parser__write__id_string__expected)
{
    request_parser parse{};
    const string_t text{ R"({"id":"foobar"})" };
    const auto size = parse.write(text);
    BOOST_CHECK(!parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_CHECK(request.jsonrpc == version::undefined);
    BOOST_CHECK(std::holds_alternative<string_t>(request.id));
    BOOST_CHECK_EQUAL(std::get<string_t>(request.id), "foobar");
}

BOOST_AUTO_TEST_CASE(request_parser__write__id_empty__expected)
{
    request_parser parse{};
    const string_t text{ R"({"id":""})" };
    const auto size = parse.write(text);
    BOOST_CHECK(!parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_CHECK(request.jsonrpc == version::undefined);
    BOOST_CHECK(std::holds_alternative<string_t>(request.id));
    BOOST_CHECK_EQUAL(std::get<string_t>(request.id), "");
}

BOOST_AUTO_TEST_CASE(request_parser__write__id_null__expected)
{
    request_parser parse{};
    const string_t text{ R"({"id":null})" };
    const auto size = parse.write(text);
    BOOST_CHECK(!parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, text.size());
    BOOST_CHECK(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_CHECK(request.jsonrpc == version::undefined);
    BOOST_CHECK(std::holds_alternative<null_t>(request.id));
}

BOOST_AUTO_TEST_CASE(request_parser__write__jsonrpc_v1_null_id__error_no_id)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":"1.0", "id":null})" };
    const auto size = parse.write(text);
    BOOST_CHECK(parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, text.size());
    BOOST_REQUIRE(parse.get_parsed().empty());
}

// jsonrpc/id inteaction
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(request_parser__write__jsonrpc_v1_no_id__error)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":"1.0"})" };
    const auto size = parse.write(text);
    BOOST_CHECK(parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, text.size());
    BOOST_REQUIRE(parse.get_parsed().empty());
}

BOOST_AUTO_TEST_CASE(request_parser__write__jsonrpc_v1_numeric_id__expected)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":"1.0", "id":42})" };
    const auto size = parse.write(text);
    BOOST_CHECK(!parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_CHECK(std::holds_alternative<code_t>(request.id));
    BOOST_CHECK_EQUAL(std::get<code_t>(request.id), 42);
    BOOST_CHECK(request.jsonrpc == version::v1);
}

BOOST_AUTO_TEST_CASE(request_parser__write__jsonrpc_v1_string_id__expected)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":"1.0", "id":"foobar"})" };
    const auto size = parse.write(text);
    BOOST_CHECK(!parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_CHECK(std::holds_alternative<string_t>(request.id));
    BOOST_CHECK_EQUAL(std::get<string_t>(request.id), "foobar");
    BOOST_CHECK(request.jsonrpc == version::v1);
}

// ----------------------------------------------------------------------------

////BOOST_AUTO_TEST_CASE(parser__write__valid_request__parses_correctly)
////{
////    request_parser parse{};
////    string_t text{ R"({"jsonrpc": "2.0", "method": "getblock", "params": ["000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f"], "id": 42})" };
////    error_code ec{};
////
////    const auto size = parse.write(text, ec);
////    ////BOOST_REQUIRE(!ec);
////    BOOST_REQUIRE_EQUAL(size, text.size());
////    BOOST_REQUIRE(parse.is_done());
////
////    const auto& batch = parse.get_parsed();
////    BOOST_REQUIRE_EQUAL(batch.size(), 1);
////
////    const auto& request = batch[0];
////    BOOST_REQUIRE_EQUAL(request.jsonrpc, "2.0");
////    BOOST_REQUIRE_EQUAL(request.method, "getblock");
////    BOOST_REQUIRE(request.params.has_value());
////    BOOST_REQUIRE(std::holds_alternative<string_t>(request.params->value));
////    BOOST_REQUIRE_EQUAL(std::get<string_t>(request.params->value), "[\"000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f\"]");
////    BOOST_REQUIRE(std::holds_alternative<code_t>(request.id));
////    BOOST_REQUIRE_EQUAL(std::get<code_t>(request.id), 42);
////}
////
////BOOST_AUTO_TEST_CASE(parser__write__invalid_request__sets_error)
////{
////    request_parser parse{};
////    const string_t text{ R"({"method": "getblock", "params": { "block": "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f" }})" };
////    error_code ec{};
////
////    const auto size = parse.write(text, ec);
////    BOOST_REQUIRE(parse.has_error());
////    BOOST_REQUIRE_EQUAL(ec, parse_error());
////    BOOST_REQUIRE_EQUAL(size, text.size());
////
////    const auto& batch = parse.get_parsed();
////    BOOST_REQUIRE(batch.empty());
////}
////
////BOOST_AUTO_TEST_CASE(parser__write__batch_request__parses_correctly)
////{
////    request_parser parse{};
////    const string_t text{ R"([{"jsonrpc": "2.0", "method": "method1", "id": 1}, {"jsonrpc": "2.0", "method": "method2", "id": 2}])" };
////    error_code ec{};
////
////    const auto size = parse.write(text, ec);
////    BOOST_REQUIRE(!ec);
////    BOOST_REQUIRE_EQUAL(size, text.size());
////    BOOST_REQUIRE(parse.is_done());
////
////    const auto& batch = parse.get_parsed();
////    BOOST_REQUIRE_EQUAL(batch.size(), 2);
////    BOOST_REQUIRE_EQUAL(batch[0].jsonrpc, "2.0");
////    BOOST_REQUIRE_EQUAL(batch[0].method, "method1");
////    BOOST_REQUIRE(std::holds_alternative<code_t>(batch[0].id));
////    BOOST_REQUIRE_EQUAL(std::get<code_t>(batch[0].id), 1);
////    BOOST_REQUIRE_EQUAL(batch[1].jsonrpc, "2.0");
////    BOOST_REQUIRE_EQUAL(batch[1].method, "method2");
////    BOOST_REQUIRE(std::holds_alternative<code_t>(batch[1].id));
////    BOOST_REQUIRE_EQUAL(std::get<code_t>(batch[1].id), 2);
////}
////
////BOOST_AUTO_TEST_CASE(parser__write__empty_batch__sets_error)
////{
////    request_parser parse{};
////    const string_t text{ R"([])" };
////    error_code ec{};
////
////    const auto size = parse.write(text, ec);
////    BOOST_REQUIRE(parse.has_error());
////    BOOST_REQUIRE_EQUAL(ec, parse_error());
////    BOOST_REQUIRE_EQUAL(size, text.size());
////
////    const auto& batch = parse.get_parsed();
////    BOOST_REQUIRE(batch.empty());
////}
////
////BOOST_AUTO_TEST_CASE(parser__write__response_with_error__parses_correctly)
////{
////    response_parser parse{ json::version::v2 };
////    const string_t text{ R"({"jsonrpc": "2.0", "error": {"code": -32601, "message": "Method not found"}, "id": 42})" };
////    error_code ec{};
////
////    const auto size = parse.write(text, ec);
////    BOOST_REQUIRE(!ec);
////    BOOST_REQUIRE_EQUAL(size, text.size());
////    BOOST_REQUIRE(parse.is_done());
////
////    const auto& batch = parse.get_parsed();
////    BOOST_REQUIRE_EQUAL(batch.size(), 1);
////
////    const auto& reponse = batch[0];
////    BOOST_REQUIRE_EQUAL(reponse.jsonrpc, "2.0");
////    BOOST_REQUIRE(reponse.error.has_value());
////    BOOST_REQUIRE_EQUAL(reponse.error->code, -32601);
////    BOOST_REQUIRE_EQUAL(reponse.error->message, "Method not found");
////    BOOST_REQUIRE(std::holds_alternative<code_t>(reponse.id));
////    BOOST_REQUIRE_EQUAL(std::get<code_t>(reponse.id), 42);
////}
////
////BOOST_AUTO_TEST_CASE(parser__write__malformed_json__sets_error)
////{
////    request_parser parse{};
////    const string_t text{ R"({"jsonrpc": "2.0", "method": "getblock" invalid})" };
////    error_code ec{};
////
////    const auto size = parse.write(text, ec);
////    BOOST_REQUIRE(parse.has_error());
////    BOOST_REQUIRE_EQUAL(ec, parse_error());
////    BOOST_REQUIRE_LE(size, text.size());
////
////    const auto& batch = parse.get_parsed();
////    BOOST_REQUIRE(batch.empty());
////}

BOOST_AUTO_TEST_SUITE_END()
