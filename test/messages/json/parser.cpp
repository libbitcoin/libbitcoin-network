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
template <bool Request>
struct test_parser
  : public parser<Request>
{
    using base = parser<Request>;

    // methods
    using base::base;
    using base::finalize;
    using base::parse_character;
    using base::handle_initialize;
    using base::handle_object_start;
    using base::handle_key;
    using base::handle_value;
    using base::handle_jsonrpc;
    using base::handle_method;
    using base::handle_params;
    using base::handle_id;
    using base::handle_result;
    using base::handle_error_code;
    using base::handle_error_message;
    using base::handle_error_data;

    // fields
    using base::quoted_;
    using base::state_;
    using base::depth_;

    // Do not assign these values for test fakes, they are references.
    using base::it_;
    using base::key_;
    using base::value_;

    using base::error_;
    using base::parsed_;
    using base::protocol_;
};

using request_parser = test_parser<true>;
using response_parser = test_parser<false>;
static_assert(request_parser::request);
static_assert(response_parser::response);
static_assert(!request_parser::response);
static_assert(!response_parser::request);

// publics
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(parser__request_construct__v2_protocol__initializes_correctly)
{
    constexpr auto expected = json::protocol::v2;
    request_parser parse{ expected };
    BOOST_REQUIRE(parse.protocol_ == expected);

    BOOST_REQUIRE(!parse.quoted_);
    BOOST_REQUIRE(parse.state_ == parser_state::initial);
    BOOST_REQUIRE(is_zero(parse.depth_));

    BOOST_REQUIRE(parse.it_ == std::string_view::iterator{});
    BOOST_REQUIRE(parse.key_.empty());
    BOOST_REQUIRE(parse.value_.empty());

    BOOST_REQUIRE(parse.parsed_.jsonrpc.empty());
    BOOST_REQUIRE(parse.parsed_.method.empty());
    BOOST_REQUIRE(!parse.parsed_.params.has_value());
    BOOST_REQUIRE(std::holds_alternative<null_t>(parse.parsed_.id));
}

BOOST_AUTO_TEST_CASE(parser__response_construct__v2_protocol__initializes_correctly)
{
    constexpr auto expected = json::protocol::v2;
    response_parser parse{ expected };
    BOOST_REQUIRE(parse.protocol_ == expected);

    BOOST_REQUIRE(!parse.quoted_);
    BOOST_REQUIRE(parse.state_ == parser_state::initial);
    BOOST_REQUIRE(is_zero(parse.depth_));

    BOOST_REQUIRE(parse.it_ == std::string_view::iterator{});
    BOOST_REQUIRE(parse.key_.empty());
    BOOST_REQUIRE(parse.value_.empty());

    BOOST_REQUIRE(parse.parsed_.jsonrpc.empty());
    BOOST_REQUIRE(!parse.parsed_.result.has_value());
    BOOST_REQUIRE(!parse.parsed_.error.has_value());
    BOOST_REQUIRE(std::holds_alternative<null_t>(parse.parsed_.id));
}

BOOST_AUTO_TEST_CASE(parser__request_reset__populated_state__clears_all_fields)
{
    request_parser parse{ json::protocol::v2 };

    // Don't assign to it_, key_, or value_ as these are references.
    parse.state_ = parser_state::jsonrpc;
    parse.depth_ = one;
    parse.quoted_ = true;
    parse.parsed_ = { "2.0", "getbalance", value_t{ { "[1,2]" } }, code_t{ 42 } };
    parse.error_ = { -1, "error", value_t{ { "data" } } };
    
    parse.reset();
    BOOST_REQUIRE(!parse.quoted_);
    BOOST_REQUIRE(parse.state_ == parser_state::initial);
    BOOST_REQUIRE(is_zero(parse.depth_));

    BOOST_REQUIRE(parse.it_ == std::string_view::iterator{});
    BOOST_REQUIRE(parse.key_.empty());
    BOOST_REQUIRE(parse.value_.empty());

    BOOST_REQUIRE(parse.parsed_.jsonrpc.empty());
    BOOST_REQUIRE(parse.parsed_.method.empty());
    BOOST_REQUIRE(!parse.parsed_.params.has_value());
    BOOST_REQUIRE(std::holds_alternative<null_t>(parse.parsed_.id));
}

BOOST_AUTO_TEST_CASE(parser__response_reset__populated_state__clears_all_fields)
{
    response_parser parse{ json::protocol::v2 };

    // Don't assign to it_, key_, or value_ as these are references.
    parse.state_ = parser_state::jsonrpc;
    parse.depth_ = one;
    parse.quoted_ = true;
    parse.parsed_ = { "2.0", value_t{ { "0.5" } }, result_t{ -1, "error", {} }, code_t{ 42 } };
    parse.error_ = { -1, "error", value_t{ { "data" } } };

    parse.reset();
    BOOST_REQUIRE(!parse.quoted_);
    BOOST_REQUIRE(parse.state_ == parser_state::initial);
    BOOST_REQUIRE(is_zero(parse.depth_));

    BOOST_REQUIRE(parse.it_ == std::string_view::iterator{});
    BOOST_REQUIRE(parse.key_.empty());
    BOOST_REQUIRE(parse.value_.empty());

    BOOST_REQUIRE(parse.parsed_.jsonrpc.empty());
    BOOST_REQUIRE(!parse.parsed_.result.has_value());
    BOOST_REQUIRE(!parse.parsed_.error.has_value());
    BOOST_REQUIRE(std::holds_alternative<null_t>(parse.parsed_.id));
}

BOOST_AUTO_TEST_CASE(parser__is_done__complete_state__returns_true)
{
    request_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::complete;
    BOOST_REQUIRE(parse.is_done());
}

BOOST_AUTO_TEST_CASE(parser__is_done__non_complete_state__returns_false)
{
    request_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::initial;
    BOOST_REQUIRE(!parse.is_done());

    parse.state_ = parser_state::jsonrpc;
    BOOST_REQUIRE(!parse.is_done());

    parse.state_ = parser_state::error_state;
    BOOST_REQUIRE(!parse.is_done());
}

BOOST_AUTO_TEST_CASE(parser__has_error__error_state__returns_true)
{
    request_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::error_state;
    BOOST_REQUIRE(parse.has_error());
}

BOOST_AUTO_TEST_CASE(parser__has_error__non_error_state__returns_false)
{
    request_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::initial;
    BOOST_REQUIRE(!parse.has_error());

    parse.state_ = parser_state::jsonrpc;
    BOOST_REQUIRE(!parse.has_error());

    parse.state_ = parser_state::complete;
    BOOST_REQUIRE(!parse.has_error());
}

BOOST_AUTO_TEST_CASE(parser__get_error__error_state__returns_invalid_argument)
{
    request_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::error_state;
    BOOST_REQUIRE_EQUAL(parse.get_error(), invalid_argument);
}

BOOST_AUTO_TEST_CASE(parser__get_error__non_error_state__returns_no_error)
{
    request_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::initial;
    BOOST_REQUIRE(!parse.get_error());

    parse.state_ = parser_state::jsonrpc;
    BOOST_REQUIRE(!parse.get_error());

    parse.state_ = parser_state::complete;
    BOOST_REQUIRE(!parse.get_error());
}

BOOST_AUTO_TEST_CASE(parser__get_request__complete_no_error__returns_valid_request)
{
    constexpr auto expected = 42;
    request_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::complete;
    parse.parsed_ = { "2.0", "getbalance", value_t{ string_t{ "[1,2]" } }, code_t{ expected } };

    const auto request = parse.get_parsed();
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
    request_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::initial;
    BOOST_REQUIRE(!parse.get_parsed());

    parse.state_ = parser_state::jsonrpc;
    BOOST_REQUIRE(!parse.get_parsed());

    parse.state_ = parser_state::error_state;
    BOOST_REQUIRE(!parse.get_parsed());
}

BOOST_AUTO_TEST_CASE(parser__get_response__complete_no_error__returns_valid_response)
{
    constexpr auto expected = 42;
    response_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::complete;
    parse.parsed_ = { "2.0", value_t{ string_t{ "0.5" } }, result_t{ -1, "error", {} }, code_t{ expected } };

    const auto response = parse.get_parsed();
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
    response_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::initial;
    BOOST_REQUIRE(!parse.get_parsed());

    parse.state_ = parser_state::jsonrpc;
    BOOST_REQUIRE(!parse.get_parsed());

    parse.state_ = parser_state::error_state;
    BOOST_REQUIRE(!parse.get_parsed());
}

BOOST_AUTO_TEST_CASE(parser__request_write__valid_v2_jsonrpc__parses_correctly)
{
    request_parser parse{ json::protocol::v2 };
    const std::string json{ R"({"jsonrpc": "2.0"})" };
    error::boost_code ec{};

    const auto size = parse.write(json, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(size, json.size());
    BOOST_REQUIRE(parse.state_ == parser_state::complete);

    BOOST_REQUIRE_EQUAL(parse.parsed_.jsonrpc, "2.0");
    BOOST_REQUIRE(parse.parsed_.method.empty());
    BOOST_REQUIRE(!parse.parsed_.params.has_value());
    BOOST_REQUIRE(std::holds_alternative<null_t>(parse.parsed_.id));

    BOOST_REQUIRE(is_zero(parse.error_.code));
    BOOST_REQUIRE(parse.error_.message.empty());
    BOOST_REQUIRE(!parse.error_.data.has_value());
}

// This test was invalidated (commented out) by handle_error_data() validation.
BOOST_AUTO_TEST_CASE(parser__response_write__valid_v2_jsonrpc__parses_correctly)
{
    response_parser parse{ json::protocol::v2 };
    const std::string json{ R"({"jsonrpc": "2.0"})" };
    error::boost_code ec{};

    const auto size = parse.write(json, ec);
////BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(size, json.size());
////    BOOST_REQUIRE(parse.state_ == parser_state::complete);

    BOOST_REQUIRE_EQUAL(parse.parsed_.jsonrpc, "2.0");
    BOOST_REQUIRE(!parse.parsed_.result.has_value());
    BOOST_REQUIRE(!parse.parsed_.error.has_value());
    BOOST_REQUIRE(std::holds_alternative<null_t>(parse.parsed_.id));

    BOOST_REQUIRE(is_zero(parse.error_.code));
    BOOST_REQUIRE(parse.error_.message.empty());
    BOOST_REQUIRE(!parse.error_.data.has_value());
}

BOOST_AUTO_TEST_CASE(parser__request_write__partial_v2_jsonrpc__parses_without_error)
{
    request_parser parse{ json::protocol::v2 };
    const std::string json{ R"({"jsonrpc": "2.0")" };
    error::boost_code ec{};

    const auto size = parse.write(json, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(size, json.size());
    BOOST_REQUIRE(parse.state_ == parser_state::object_start);
    BOOST_REQUIRE(!parse.quoted_);
    BOOST_REQUIRE(is_one(parse.depth_));
    BOOST_REQUIRE(parse.value_.empty());
    BOOST_REQUIRE_EQUAL(parse.parsed_.jsonrpc, "2.0");
}

BOOST_AUTO_TEST_CASE(parser__response_write__partial_v2_jsonrpc__parses_without_error)
{
    response_parser parse{ json::protocol::v2 };
    const std::string json{ R"({"jsonrpc": "2.0")" };
    error::boost_code ec{};

    const auto size = parse.write(json, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(size, json.size());
    BOOST_REQUIRE(parse.state_ == parser_state::object_start);
    BOOST_REQUIRE(!parse.quoted_);
    BOOST_REQUIRE(is_one(parse.depth_));
    BOOST_REQUIRE(parse.value_.empty());
    BOOST_REQUIRE_EQUAL(parse.parsed_.jsonrpc, "2.0");
}

BOOST_AUTO_TEST_CASE(parser__request_write__invalid_v2_jsonrpc__sets_error_state)
{
    request_parser parse{ json::protocol::v2 };
    const std::string json{ R"({"invalid": "2.0"})" };
    const auto expected = std::string{ R"({"invalid")" }.size();
    error::boost_code ec{};

    const auto size = parse.write(json, ec);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE_EQUAL(ec, invalid_argument);
    BOOST_REQUIRE_EQUAL(size, expected);
    BOOST_REQUIRE(parse.state_ == parser_state::error_state);
    BOOST_REQUIRE(is_one(parse.depth_));
    BOOST_REQUIRE(!parse.quoted_);
    BOOST_REQUIRE_EQUAL(parse.key_, "invalid");
}
BOOST_AUTO_TEST_CASE(parser__response_write__invalid_v2_jsonrpc__sets_error_state)
{
    response_parser parse{ json::protocol::v2 };
    const std::string json{ R"({"invalid": "2.0"})" };
    const auto expected = std::string{ R"({"invalid")" }.size();
    error::boost_code ec{};

    const auto size = parse.write(json, ec);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE_EQUAL(ec, invalid_argument);
    BOOST_REQUIRE_EQUAL(size, expected);
    BOOST_REQUIRE(parse.state_ == parser_state::error_state);
    BOOST_REQUIRE(is_one(parse.depth_));
    BOOST_REQUIRE(!parse.quoted_);
    BOOST_REQUIRE_EQUAL(parse.key_, "invalid");
    BOOST_REQUIRE(parse.parsed_.jsonrpc.empty());
}

// protected
// ----------------------------------------------------------------------------

// finalize

// parse_character

BOOST_AUTO_TEST_CASE(parser__parse_character__initial_open_brace__transitions_to_object_start)
{
    request_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::initial;

    parse.parse_character('{');
    BOOST_REQUIRE(parse.state_ == parser_state::object_start);
    BOOST_REQUIRE(is_one(parse.depth_));
    BOOST_REQUIRE(!parse.quoted_);
}

BOOST_AUTO_TEST_CASE(parser__parse_character__initial_invalid_char__sets_error_state)
{
    request_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::initial;

    parse.parse_character('x');
    BOOST_REQUIRE(parse.state_ == parser_state::error_state);
    BOOST_REQUIRE(is_zero(parse.depth_));
    BOOST_REQUIRE(!parse.quoted_);
}

// handle_initialize

BOOST_AUTO_TEST_CASE(parser__handle_initialize__open_brace__transitions_to_object_start)
{
    request_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::initial;

    parse.handle_initialize('{');
    BOOST_REQUIRE(parse.state_ == parser_state::object_start);
    BOOST_REQUIRE(is_one(parse.depth_));
    BOOST_REQUIRE(!parse.quoted_);
}

BOOST_AUTO_TEST_CASE(parser__handle_initialize__invalid_char__sets_error_state)
{
    request_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::initial;

    parse.handle_initialize('x');
    BOOST_REQUIRE(parse.state_ == parser_state::error_state);
    BOOST_REQUIRE(is_zero(parse.depth_));
    BOOST_REQUIRE(!parse.quoted_);
}

// handle_object_start

BOOST_AUTO_TEST_CASE(parser__handle_object_start__quote__transitions_to_key)
{
    request_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::object_start;

    parse.handle_object_start('"');
    BOOST_REQUIRE(parse.state_ == parser_state::key);
    BOOST_REQUIRE(parse.quoted_);
    BOOST_REQUIRE(parse.key_.empty());
}

BOOST_AUTO_TEST_CASE(parser__handle_object_start__close_brace__transitions_to_complete)
{
    request_parser parse{ json::protocol::v2 };
    parse.state_ = parser_state::object_start;
    parse.depth_ = one;

    parse.handle_object_start('}');
    BOOST_REQUIRE(parse.state_ == parser_state::complete);
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
