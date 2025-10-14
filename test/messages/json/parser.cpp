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

BOOST_AUTO_TEST_SUITE(parser_tests)

// This is the namespace for parser and its types unless otherwise specified.
using namespace network::json;
namespace errc = boost::system::errc;
const auto invalid_argument = errc::make_error_code(errc::invalid_argument);

// Test-specific derived class to access protected members
struct test_parser
  : public parser
{
    // methods
    using parser::parser;
    using parser::finalize;
    using parser::parse_character;
    using parser::handle_initialize;
    using parser::handle_object_start;
    using parser::handle_key;
    using parser::handle_value;
    using parser::handle_jsonrpc;
    using parser::handle_method;
    using parser::handle_params;
    using parser::handle_id;
    using parser::handle_result;
    using parser::handle_error_code;
    using parser::handle_error_message;
    using parser::handle_error_data;

    // fields
    using parser::quoted_;
    using parser::state_;
    using parser::depth_;

    // Do not assign these values for test fakes, they are references.
    using parser::it_;
    using parser::key_;
    using parser::value_;

    using parser::error_;
    using parser::request_;
    using parser::response_;
    using parser::protocol_;
};

// publics
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(parser__construct__v2_protocol__initializes_correctly)
{
    constexpr auto expected = json::protocol::v2;
    test_parser parse{ expected };
    BOOST_REQUIRE(parse.protocol_ == expected);

    BOOST_REQUIRE(!parse.quoted_);
    BOOST_REQUIRE(parse.state_ == parser::state::initial);
    BOOST_REQUIRE(is_zero(parse.depth_));

    BOOST_REQUIRE(parse.it_ == std::string_view::iterator{});
    BOOST_REQUIRE(parse.key_.empty());
    BOOST_REQUIRE(parse.value_.empty());

    BOOST_REQUIRE(parse.request_.jsonrpc.empty());
    BOOST_REQUIRE(parse.request_.method.empty());
    BOOST_REQUIRE(!parse.request_.params.has_value());
    BOOST_REQUIRE(std::holds_alternative<null_t>(parse.request_.id));

    BOOST_REQUIRE(parse.response_.jsonrpc.empty());
    BOOST_REQUIRE(!parse.response_.result.has_value());
    BOOST_REQUIRE(!parse.response_.error.has_value());
    BOOST_REQUIRE(std::holds_alternative<null_t>(parse.response_.id));
}

BOOST_AUTO_TEST_CASE(parser__reset__populated_state__clears_all_fields)
{
    test_parser parse{ json::protocol::v2 };

    // Don't assign to it_, key_, or value_ as these are references.
    parse.state_ = parser::state::jsonrpc;
    parse.depth_ = one;
    parse.quoted_ = true;
    parse.request_ = { "2.0", "getbalance", value_t{ { "[1,2]" } }, code_t{ 42 } };
    parse.response_ = { "2.0", value_t{ { "0.5" } }, result_t{ -1, "error", {} }, code_t{ 42 } };
    parse.error_ = { -1, "error", value_t{ { "data" } } };
    
    parse.reset();
    BOOST_REQUIRE(!parse.quoted_);
    BOOST_REQUIRE(parse.state_ == parser::state::initial);
    BOOST_REQUIRE(is_zero(parse.depth_));

    BOOST_REQUIRE(parse.it_ == std::string_view::iterator{});
    BOOST_REQUIRE(parse.key_.empty());
    BOOST_REQUIRE(parse.value_.empty());

    BOOST_REQUIRE(parse.request_.jsonrpc.empty());
    BOOST_REQUIRE(parse.request_.method.empty());
    BOOST_REQUIRE(!parse.request_.params.has_value());
    BOOST_REQUIRE(std::holds_alternative<null_t>(parse.request_.id));

    BOOST_REQUIRE(parse.response_.jsonrpc.empty());
    BOOST_REQUIRE(!parse.response_.result.has_value());
    BOOST_REQUIRE(!parse.response_.error.has_value());
    BOOST_REQUIRE(std::holds_alternative<null_t>(parse.response_.id));
}

BOOST_AUTO_TEST_CASE(parser__is_done__complete_state__returns_true)
{
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::complete;
    BOOST_REQUIRE(parse.is_done());
}

BOOST_AUTO_TEST_CASE(parser__is_done__non_complete_state__returns_false)
{
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::initial;
    BOOST_REQUIRE(!parse.is_done());

    parse.state_ = parser::state::jsonrpc;
    BOOST_REQUIRE(!parse.is_done());

    parse.state_ = parser::state::error_state;
    BOOST_REQUIRE(!parse.is_done());
}

BOOST_AUTO_TEST_CASE(parser__has_error__error_state__returns_true)
{
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::error_state;
    BOOST_REQUIRE(parse.has_error());
}

BOOST_AUTO_TEST_CASE(parser__has_error__non_error_state__returns_false)
{
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::initial;
    BOOST_REQUIRE(!parse.has_error());

    parse.state_ = parser::state::jsonrpc;
    BOOST_REQUIRE(!parse.has_error());

    parse.state_ = parser::state::complete;
    BOOST_REQUIRE(!parse.has_error());
}

BOOST_AUTO_TEST_CASE(parser__get_error__error_state__returns_invalid_argument)
{
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::error_state;
    BOOST_REQUIRE_EQUAL(parse.get_error(), invalid_argument);
}

BOOST_AUTO_TEST_CASE(parser__get_error__non_error_state__returns_no_error)
{
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::initial;
    BOOST_REQUIRE(!parse.get_error());

    parse.state_ = parser::state::jsonrpc;
    BOOST_REQUIRE(!parse.get_error());

    parse.state_ = parser::state::complete;
    BOOST_REQUIRE(!parse.get_error());
}

BOOST_AUTO_TEST_CASE(parser__get_request__complete_no_error__returns_valid_request)
{
    constexpr auto expected = 42;
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::complete;
    parse.request_ = { "2.0", "getbalance", value_t{ string_t{ "[1,2]" } }, code_t{ expected } };

    const auto request = parse.get_request();
    BOOST_REQUIRE(request.has_value());
    BOOST_REQUIRE_EQUAL(request->jsonrpc, "2.0");
    BOOST_REQUIRE_EQUAL(request->method, "getbalance");
    BOOST_REQUIRE(request->params.has_value());
    BOOST_REQUIRE(std::holds_alternative<string_t>(request->params->value));
    BOOST_REQUIRE_EQUAL(std::get<string_t>(request->params->value), "[1,2]");
    BOOST_REQUIRE(std::holds_alternative<code_t>(request->id));
    BOOST_REQUIRE_EQUAL(std::get<code_t>(request->id), expected);
}

BOOST_AUTO_TEST_CASE(parser__get_request__incomplete_or_error__returns_empty)
{
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::initial;
    BOOST_REQUIRE(!parse.get_request());

    parse.state_ = parser::state::jsonrpc;
    BOOST_REQUIRE(!parse.get_request());

    parse.state_ = parser::state::error_state;
    BOOST_REQUIRE(!parse.get_request());
}

BOOST_AUTO_TEST_CASE(parser__get_response__complete_no_error__returns_valid_response)
{
    constexpr auto expected = 42;
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::complete;
    parse.response_ = { "2.0", value_t{ string_t{ "0.5" } }, result_t{ -1, "error", {} }, code_t{ expected } };

    const auto response = parse.get_response();
    BOOST_REQUIRE(response.has_value());
    BOOST_REQUIRE_EQUAL(response->jsonrpc, "2.0");
    BOOST_REQUIRE(response->result.has_value());
    BOOST_REQUIRE(std::holds_alternative<string_t>(response->result->value));
    BOOST_REQUIRE_EQUAL(std::get<string_t>(response->result->value), "0.5");
    BOOST_REQUIRE(response->error.has_value());
    BOOST_REQUIRE_EQUAL(response->error->code, -1);
    BOOST_REQUIRE_EQUAL(response->error->message, "error");
    BOOST_REQUIRE(!response->error->data.has_value());
    BOOST_REQUIRE(std::holds_alternative<code_t>(response->id));
    BOOST_REQUIRE_EQUAL(std::get<code_t>(response->id), expected);
}

BOOST_AUTO_TEST_CASE(parser__get_response__incomplete_or_error__returns_empty)
{
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::initial;
    BOOST_REQUIRE(!parse.get_response());

    parse.state_ = parser::state::jsonrpc;
    BOOST_REQUIRE(!parse.get_response());

    parse.state_ = parser::state::error_state;
    BOOST_REQUIRE(!parse.get_response());
}

BOOST_AUTO_TEST_CASE(parser__write__valid_v2_jsonrpc__parses_correctly)
{
    test_parser parse{ json::protocol::v2 };
    const std::string json{ R"({"jsonrpc": "2.0"})" };
    error::boost_code ec{};

    const auto size = parse.write(json, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(size, json.size());
    BOOST_REQUIRE(parse.state_ == parser::state::complete);

    BOOST_REQUIRE_EQUAL(parse.request_.jsonrpc, "2.0");
    BOOST_REQUIRE(parse.request_.method.empty());
    BOOST_REQUIRE(!parse.request_.params.has_value());
    BOOST_REQUIRE(std::holds_alternative<null_t>(parse.request_.id));

    BOOST_REQUIRE_EQUAL(parse.response_.jsonrpc, "2.0");
    BOOST_REQUIRE(!parse.response_.result.has_value());
    BOOST_REQUIRE(!parse.response_.error.has_value());
    BOOST_REQUIRE(std::holds_alternative<null_t>(parse.response_.id));

    BOOST_REQUIRE(is_zero(parse.error_.code));
    BOOST_REQUIRE(parse.error_.message.empty());
    BOOST_REQUIRE(!parse.error_.data.has_value());
}

BOOST_AUTO_TEST_CASE(parser__write__partial_v2_jsonrpc__parses_without_error)
{
    test_parser parse{ json::protocol::v2 };
    const std::string json{ R"({"jsonrpc": "2.0")" };
    error::boost_code ec{};

    const auto size = parse.write(json, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(size, json.size());
    BOOST_REQUIRE(parse.state_ == parser::state::object_start);
    BOOST_REQUIRE(!parse.quoted_);
    BOOST_REQUIRE(is_one(parse.depth_));
    BOOST_REQUIRE(parse.value_.empty());
    BOOST_REQUIRE_EQUAL(parse.request_.jsonrpc, "2.0");
    BOOST_REQUIRE_EQUAL(parse.response_.jsonrpc, "2.0");
}

BOOST_AUTO_TEST_CASE(parser__write__invalid_v2_jsonrpc__sets_error_state)
{
    test_parser parse{ json::protocol::v2 };
    const std::string json{ R"({"invalid": "2.0"})" };
    const auto expected = std::string{ R"({"invalid")" }.size();
    error::boost_code ec{};

    const auto size = parse.write(json, ec);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE_EQUAL(ec, invalid_argument);
    BOOST_REQUIRE_EQUAL(size, expected);
    BOOST_REQUIRE(parse.state_ == parser::state::error_state);
    BOOST_REQUIRE(parse.request_.jsonrpc.empty());
    BOOST_REQUIRE(parse.response_.jsonrpc.empty());
    BOOST_REQUIRE(is_one(parse.depth_));
    BOOST_REQUIRE(!parse.quoted_);
    BOOST_REQUIRE_EQUAL(parse.key_, "invalid");
}

// protected
// ----------------------------------------------------------------------------

// finalize

// parse_character

BOOST_AUTO_TEST_CASE(parser__parse_character__initial_open_brace__transitions_to_object_start)
{
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::initial;

    parse.parse_character('{');
    BOOST_REQUIRE(parse.state_ == parser::state::object_start);
    BOOST_REQUIRE(is_one(parse.depth_));
    BOOST_REQUIRE(!parse.quoted_);
}

BOOST_AUTO_TEST_CASE(parser__parse_character__initial_invalid_char__sets_error_state)
{
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::initial;

    parse.parse_character('x');
    BOOST_REQUIRE(parse.state_ == parser::state::error_state);
    BOOST_REQUIRE(is_zero(parse.depth_));
    BOOST_REQUIRE(!parse.quoted_);
}

// handle_initialize

BOOST_AUTO_TEST_CASE(parser__handle_initialize__open_brace__transitions_to_object_start)
{
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::initial;

    parse.handle_initialize('{');
    BOOST_REQUIRE(parse.state_ == parser::state::object_start);
    BOOST_REQUIRE(is_one(parse.depth_));
    BOOST_REQUIRE(!parse.quoted_);
}

BOOST_AUTO_TEST_CASE(parser__handle_initialize__invalid_char__sets_error_state)
{
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::initial;

    parse.handle_initialize('x');
    BOOST_REQUIRE(parse.state_ == parser::state::error_state);
    BOOST_REQUIRE(is_zero(parse.depth_));
    BOOST_REQUIRE(!parse.quoted_);
}

// handle_object_start

BOOST_AUTO_TEST_CASE(parser__handle_object_start__quote__transitions_to_key)
{
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::object_start;

    parse.handle_object_start('"');
    BOOST_REQUIRE(parse.state_ == parser::state::key);
    BOOST_REQUIRE(parse.quoted_);
    BOOST_REQUIRE(parse.key_.empty());
}

BOOST_AUTO_TEST_CASE(parser__handle_object_start__close_brace__transitions_to_complete)
{
    test_parser parse{ json::protocol::v2 };
    parse.state_ = parser::state::object_start;
    parse.depth_ = one;

    parse.handle_object_start('}');
    BOOST_REQUIRE(parse.state_ == parser::state::complete);
    BOOST_REQUIRE(is_zero(parse.depth_));
    BOOST_REQUIRE(!parse.quoted_);
}

// handle_key
// handle_value
// handle_jsonrpc
// handle_method
// handle_params
// handle_id
// handle_result
// handle_error_start
// handle_error_code
// handle_error_message
// handle_error_data

BOOST_AUTO_TEST_SUITE_END()
