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

template <bool Strict, version Require, bool Trace>
struct test_parser
  : public parser<Strict, Require, Trace>
{
    using base = parser<Strict, Require, Trace>;
    using base::base;
};

using lax_request_parser = test_parser<false, version::any, true>;
using request_parser = test_parser<true, version::any, true>;

////using namespace boost::system::errc;
////static auto incomplete = make_error_code(interrupted);
////static auto failure = make_error_code(invalid_argument);

// jsonrpc v1/v2
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(request_parser__write__jsonrpc_empty__error)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":""})" };
    const auto size = parse.write(text);
    BOOST_CHECK(parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, 14u);
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
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_CHECK(request.jsonrpc == version::v2);
}

// id
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(request_parser__write__id_positive__expected)
{
    request_parser parse{};
    const string_t text{ R"({"id":42})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_CHECK(request.jsonrpc == version::undefined);
    BOOST_REQUIRE(request.id.has_value());
    BOOST_REQUIRE(std::holds_alternative<code_t>(request.id.value()));
    BOOST_CHECK_EQUAL(std::get<code_t>(request.id.value()), 42);
}

BOOST_AUTO_TEST_CASE(request_parser__write__id_negative__expected)
{
    request_parser parse{};
    const string_t text{ R"({"id":-42})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(request.id.has_value());
    BOOST_REQUIRE(std::holds_alternative<code_t>(request.id.value()));
    BOOST_CHECK_EQUAL(std::get<code_t>(request.id.value()), -42);
}

BOOST_AUTO_TEST_CASE(request_parser__write__id_string__expected)
{
    request_parser parse{};
    const string_t text{ R"({"id":"foobar"})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(request.id.has_value());
    BOOST_REQUIRE(std::holds_alternative<string_t>(request.id.value()));
    BOOST_CHECK_EQUAL(std::get<string_t>(request.id.value()), "foobar");
}

BOOST_AUTO_TEST_CASE(request_parser__write__id_empty__expected)
{
    request_parser parse{};
    const string_t text{ R"({"id":""})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(request.id.has_value());
    BOOST_REQUIRE(std::holds_alternative<string_t>(request.id.value()));
    BOOST_CHECK_EQUAL(std::get<string_t>(request.id.value()), "");
}

BOOST_AUTO_TEST_CASE(request_parser__write__id_null__expected)
{
    request_parser parse{};
    const string_t text{ R"({"id":null})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_CHECK(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(request.id.has_value());
    BOOST_REQUIRE(std::holds_alternative<null_t>(request.id.value()));
}

// method
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(request_parser__write__method__expected)
{
    request_parser parse{};
    const string_t text{ R"({"method":"foobar"})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));
    BOOST_CHECK_EQUAL(parse.get_parsed().front().method, "foobar");
}

BOOST_AUTO_TEST_CASE(request_parser__write__jsonrpc_v2_method__expected)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":"2.0","method":"foobar"})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_CHECK(request.jsonrpc == version::v2);
    BOOST_CHECK_EQUAL(request.method, "foobar");
}

BOOST_AUTO_TEST_CASE(request_parser__write__jsonrpc_v1_id_string_method__expected)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":"1.0","id":"libbitcoin","method":"fast"})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_CHECK(request.jsonrpc == version::v1);
    BOOST_REQUIRE(request.id.has_value());
    BOOST_REQUIRE(std::holds_alternative<string_t>(request.id.value()));
    BOOST_CHECK_EQUAL(std::get<string_t>(request.id.value()), "libbitcoin");
    BOOST_CHECK_EQUAL(request.method, "fast");
}

BOOST_AUTO_TEST_CASE(request_parser__write__method_id_string_jsonrpc_v1__expected)
{
    request_parser parse{};
    const string_t text{ R"({"method":"fast","id":"libbitcoin","jsonrpc":"1.0"})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_CHECK(request.jsonrpc == version::v1);
    BOOST_REQUIRE(request.id.has_value());
    BOOST_REQUIRE(std::holds_alternative<string_t>(request.id.value()));
    BOOST_CHECK_EQUAL(std::get<string_t>(request.id.value()), "libbitcoin");
    BOOST_CHECK_EQUAL(request.method, "fast");
}

BOOST_AUTO_TEST_CASE(request_parser__write__id_string_jsonrpc_v1_method__expected)
{
    request_parser parse{};
    const string_t text{ R"({"id":"libbitcoin","jsonrpc":"1.0","method":"fast"})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_CHECK(request.jsonrpc == version::v1);
    BOOST_REQUIRE(request.id.has_value());
    BOOST_CHECK(std::holds_alternative<string_t>(request.id.value()));
    BOOST_CHECK_EQUAL(std::get<string_t>(request.id.value()), "libbitcoin");
    BOOST_CHECK_EQUAL(request.method, "fast");
}

// jsonrpc/id interaction
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(request_parser__write__jsonrpc_v1_no_id__error)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":"1.0"})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(parse.get_parsed().empty());
}

BOOST_AUTO_TEST_CASE(request_parser__write__jsonrpc_v1_null_id__error)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":"1.0","id":null})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(parse.get_parsed().empty());
}

BOOST_AUTO_TEST_CASE(request_parser__write__jsonrpc_v1_numeric_id__expected)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":"1.0","id":42})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(request.id.has_value());
    BOOST_CHECK(std::holds_alternative<code_t>(request.id.value()));
    BOOST_CHECK_EQUAL(std::get<code_t>(request.id.value()), 42);
    BOOST_CHECK(request.jsonrpc == version::v1);
}

BOOST_AUTO_TEST_CASE(request_parser__write__jsonrpc_v1_string_id__expected)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":"1.0","id":"foobar"})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(request.id.has_value());
    BOOST_CHECK(std::holds_alternative<string_t>(request.id.value()));
    BOOST_CHECK_EQUAL(std::get<string_t>(request.id.value()), "foobar");
    BOOST_CHECK(request.jsonrpc == version::v1);
}

// whitespace
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(request_parser__write__whitespace_all__expected)
{
    request_parser parse{};
    const string_t text
    {
        " \n\r\t { \n\r\t \"jsonrpc\" \n\r\t : \n\r\t \"2.0\" \n\r\t ,"
        " \n\r\t \"id\" \n\r\t : \n\r\t \"foobar\" \n\r\t } \n\r\t "
    };
    BOOST_CHECK_EQUAL(parse.write(text), 76);

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(request.id.has_value());
    BOOST_CHECK(std::holds_alternative<string_t>(request.id.value()));
    BOOST_CHECK_EQUAL(std::get<string_t>(request.id.value()), "foobar");
    BOOST_CHECK(request.jsonrpc == version::v2);
}

BOOST_AUTO_TEST_CASE(request_parser__write__whitespace_invalid_error)
{
    request_parser parse{};
    const string_t text
    {
        " \n\r\t { \n\r\t \"jsonrpc\" \n\v\t : \n\r\t \"2.0\" \n\r\t ,"
        " \n\r\t \"id\" \n\r\t : \n\r\t \"foobar\" \n\r\t } \n\r\t "
    };
    BOOST_CHECK_EQUAL(parse.write(text), 23);
    BOOST_CHECK(parse.has_error());
}

// escape
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(request_parser__write__json_escape__not_implemented)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":"2.0","id":"foo\\bar"})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(request.id.has_value());
    BOOST_CHECK(std::holds_alternative<string_t>(request.id.value()));

    // Escapes are not yet supported.
    BOOST_CHECK_NE(std::get<string_t>(request.id.value()), R"("foo\bar")");
    BOOST_CHECK(request.jsonrpc == version::v2);
}

BOOST_AUTO_TEST_CASE(request_parser__write__native_escape__expected)
{
    request_parser parse{};
    const string_t text{ "{\"jsonrpc\":\"2.0\",\"id\":\"foo\\bar\"}" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(request.id.has_value());
    BOOST_CHECK(std::holds_alternative<string_t>(request.id.value()));

    // Escapes are not yet supported.
    BOOST_CHECK_NE(std::get<string_t>(request.id.value()), R"("foo\bar")");
    BOOST_CHECK(request.jsonrpc == version::v2);
}

// params
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(request_parser__write__params_null__error)
{
    request_parser parse{};
    const string_t text{ R"({"params":null})" };
    const auto size = parse.write(text);
    BOOST_CHECK(parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, 11u);
    BOOST_REQUIRE(parse.get_parsed().empty());
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_string__error)
{
    request_parser parse{};
    const string_t text{ R"({"params":"foobar"})" };
    const auto size = parse.write(text);
    BOOST_CHECK(parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, 11u);
    BOOST_REQUIRE(parse.get_parsed().empty());
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_number__error)
{
    request_parser parse{};
    const string_t text{ R"({"params":42})" };
    const auto size = parse.write(text);
    BOOST_CHECK(parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, 11u);
    BOOST_REQUIRE(parse.get_parsed().empty());
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_boolean__error)
{
    request_parser parse{};
    const string_t text{ R"({"params":true})" };
    const auto size = parse.write(text);
    BOOST_CHECK(parse.has_error());
    BOOST_CHECK(parse.is_done());
    BOOST_CHECK_EQUAL(size, 11u);
    BOOST_REQUIRE(parse.get_parsed().empty());
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_empty_array__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":[]})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(request.params.has_value());
    BOOST_REQUIRE(std::holds_alternative<array_t>(request.params.value()));

    const auto& value = std::get<array_t>(request.params.value());
    BOOST_CHECK(value.empty());
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_empty_object__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":{}})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(request.params.has_value());
    BOOST_REQUIRE(std::holds_alternative<object_t>(request.params.value()));

    const auto& value = std::get<object_t>(request.params.value());
    BOOST_CHECK(value.empty());
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_populated_array__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":[42]})" };
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(std::holds_alternative<array_t>(request.params.value()));

    const auto& value = std::get<array_t>(request.params.value());
    BOOST_REQUIRE_EQUAL(value.size(), one);

    const auto& only = value.front();
    BOOST_REQUIRE(std::holds_alternative<number_t>(only.inner));
    BOOST_CHECK_EQUAL(std::get<number_t>(only.inner), 42);
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_populated_object__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":{"solution":42}})" };
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(std::holds_alternative<object_t>(request.params.value()));

    const auto& value = std::get<object_t>(request.params.value());
    BOOST_REQUIRE_EQUAL(value.size(), one);

    const auto& only = value.at("solution");
    BOOST_REQUIRE(std::holds_alternative<number_t>(only.inner));
    BOOST_CHECK_EQUAL(std::get<number_t>(only.inner), 42);
}

BOOST_AUTO_TEST_SUITE_END()
