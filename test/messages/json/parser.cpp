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

using lax_request_parser = test_parser<false, version::any, false>;
using request_parser = test_parser<true, version::any, false>;

////using namespace boost::system::errc;
////static auto incomplete = make_error_code(interrupted);
////static auto failure = make_error_code(invalid_argument);

std::string_view to_string(version value) NOEXCEPT
{
    switch (value)
    {
        case version::v1:
            return "1.0";
        case version::v2:
            return "2.0";
        default:
            return "";
    }
}

// Escapes a string for JSON output.
inline std::string escape_string(const string_t& str)
{
    std::ostringstream out{};
    out << '"';
    for (const char character : str)
    {
        switch (character)
        {
            case '"': out << '\\' << '"';
                break;
            case '\\': out << "\\\\";
                break;
            case '\b': out << "\\b";
                break;
            case '\f': out << "\\f";
                break;
            case '\n': out << "\\n";
                break;
            case '\r': out << "\\r";
                break;
            case '\t': out << "\\t";
                break;
            default:
                out << character; break;
        }
    }

    out << '"';
    return out.str();
}

// Serializes value_t to JSON string, handling blobs as nested structures.
inline std::string serialize_value(const value_t& value)
{
    std::ostringstream out{};
    std::visit(overload
    {
        [&](null_t)
        {
            out << "null";
        },
        [&](boolean_t visit)
        {
            out << (visit ? "true" : "false");
        },
        [&](number_t visit)
        {
            out << visit;
        },
        [&](const string_t& visit)
        {
            out << escape_string(visit);
        },
        [&](const array_t& visit)
        {
            if (visit.empty())
            {
                out << "[-empty-array-]";
                return;
            }

            const auto& first = visit.front().inner;
            if (!std::holds_alternative<string_t>(first))
            {
                out << "[-non-string-array-value-]";
                return;
            }

            out << std::get<string_t>(first);
        },
        [&](const object_t& visit)
        {
            if (visit.empty())
            {
                out << "{-empty-object-}";
                return;
            }

            const auto& first = visit.begin()->second.inner;
            if (!std::holds_alternative<string_t>(first))
            {
                out << "{-non-string-object-value-}";
                return;
            }

            out << std::get<string_t>(first);
        }
    }, value.inner);

    return out.str();
}

// Serializes the request_t to a compact JSON string for testing.
// Handles flat blob strings in params structures as literal JSON.
string_t to_string(const request_t& request)
{
    std::ostringstream out{};
    out << '{';

    const auto jsonrpc = to_string(request.jsonrpc);
    const auto& method = request.method;

    if (!jsonrpc.empty())
        out << "\"jsonrpc\":\"" << jsonrpc << '"';

    if (!jsonrpc.empty() && !method.empty())
        out << ",";

    if (!method.empty())
        out << "\"method\":\"" << method << '"';

    if (request.id.has_value())
    {
        if (!jsonrpc.empty() || !method.empty())
            out << ",";

        out << "\"id\":";

        const auto& id = request.id.value();
        std::visit(overload
        {
            [&](null_t)
            {
                out << "null";
            },
            [&](code_t visit)
            {
                out << visit;
            },
            [&](const string_t& visit)
            {
                out << escape_string(visit);
            }
        }, id);
    }

    if (request.params.has_value())
    {
        if (!jsonrpc.empty() || !method.empty() || request.id.has_value())
            out << ",";

        out << "\"params\":";

        const auto& params = request.params.value();
        if (std::holds_alternative<array_t>(params))
        {
            out << '[';

            const auto& parameters = std::get<array_t>(params);
            for (auto index = zero; index < parameters.size(); ++index)
            {
                if (!is_zero(index)) out << ',';
                out << serialize_value(parameters.at(index));
            }
            out << ']';
        }
        else
        {
            out << '{';

            auto first = true;
            const auto& parameters = std::get<object_t>(params);

            // Sort keys for predictable output.
            std::vector<string_t> keys{};
            keys.reserve(parameters.size());
            for (const auto& pair: parameters)
                keys.push_back(pair.first);

            std::sort(keys.begin(), keys.end());

            for (const auto& key: keys)
            {
                if (!first) out << ',';
                out << escape_string(key);
                out << ":" << serialize_value(parameters.at(key));
                first = false;
            }

            out << '}';
        }
    }

    out << '}';
    return out.str();
}

// test the test tool
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(request_parser__to_string__request__expected)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":"2.0","method":"random","id":-42,"params":{"array":[A],"false":false,"foo":"bar","null":null,"number":42,"object":{O},"true":true}})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(is_one(parse.get_parsed().size()));

    const auto request = parse.get_parsed().front();
    BOOST_CHECK(request.jsonrpc == version::v2);
    BOOST_CHECK_EQUAL(to_string(request), text);
}

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

BOOST_AUTO_TEST_CASE(request_parser__write__params_array_empty__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":[]})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(request.params.has_value());
    BOOST_REQUIRE(std::holds_alternative<array_t>(request.params.value()));

    const auto& params = std::get<array_t>(request.params.value());
    BOOST_CHECK(params.empty());
}

// The "params" property is array only in v1.
BOOST_AUTO_TEST_CASE(request_parser__write__params_object_jsonrpc_v1__error)
{
    request_parser parse{};
    const string_t text{ R"({"jsonrpc":"1.0","params":{}})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());
    BOOST_CHECK(parse.has_error());
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_object_empty__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":{}})" };
    BOOST_CHECK_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(request.params.has_value());
    BOOST_REQUIRE(std::holds_alternative<object_t>(request.params.value()));

    const auto& params = std::get<object_t>(request.params.value());
    BOOST_CHECK(params.empty());
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_array_single_number__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":[42]})" };
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(std::holds_alternative<array_t>(request.params.value()));

    const auto& params = std::get<array_t>(request.params.value());
    BOOST_REQUIRE_EQUAL(params.size(), one);

    const auto& only = params.front().inner;
    BOOST_REQUIRE(std::holds_alternative<number_t>(only));
    BOOST_CHECK_EQUAL(std::get<number_t>(only), 42);
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_object_single_number__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":{"solution":42}})" };
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(std::holds_alternative<object_t>(request.params.value()));

    const auto& params = std::get<object_t>(request.params.value());
    BOOST_REQUIRE_EQUAL(params.size(), one);

    const auto& only = params.at("solution").inner;
    BOOST_REQUIRE(std::holds_alternative<number_t>(only));
    BOOST_CHECK_EQUAL(std::get<number_t>(only), 42);
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_array_multiple_number__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":[4242,-2424,0]})" };
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(std::holds_alternative<array_t>(request.params.value()));

    const auto& params = std::get<array_t>(request.params.value());
    BOOST_REQUIRE_EQUAL(params.size(), 3u);
    BOOST_REQUIRE(std::holds_alternative<number_t>(params.at(0).inner));
    BOOST_REQUIRE(std::holds_alternative<number_t>(params.at(1).inner));
    BOOST_REQUIRE(std::holds_alternative<number_t>(params.at(2).inner));
    BOOST_CHECK_EQUAL(std::get<number_t>(params.at(0).inner), 4242);
    BOOST_CHECK_EQUAL(std::get<number_t>(params.at(1).inner), -2424);
    BOOST_CHECK_EQUAL(std::get<number_t>(params.at(2).inner), 0);
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_object_multiple_number__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":{"a":4242,"b":-2424,"c":0}})" };
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(std::holds_alternative<object_t>(request.params.value()));

    const auto& params = std::get<object_t>(request.params.value());
    BOOST_REQUIRE_EQUAL(params.size(), 3u);
    BOOST_REQUIRE(std::holds_alternative<number_t>(params.at("a").inner));
    BOOST_REQUIRE(std::holds_alternative<number_t>(params.at("b").inner));
    BOOST_REQUIRE(std::holds_alternative<number_t>(params.at("c").inner));
    BOOST_CHECK_EQUAL(std::get<number_t>(params.at("a").inner), 4242);
    BOOST_CHECK_EQUAL(std::get<number_t>(params.at("b").inner), -2424);
    BOOST_CHECK_EQUAL(std::get<number_t>(params.at("c").inner), 0);
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_array_mixed__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":[null,true,false,42,-42,"foo","bar"]})" };
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(std::holds_alternative<array_t>(request.params.value()));

    const auto& params = std::get<array_t>(request.params.value());
    BOOST_REQUIRE_EQUAL(params.size(), 7u);
    BOOST_REQUIRE(std::holds_alternative<null_t>(params.at(0).inner));
    BOOST_REQUIRE(std::holds_alternative<boolean_t>(params.at(1).inner));
    BOOST_REQUIRE(std::holds_alternative<boolean_t>(params.at(2).inner));
    BOOST_REQUIRE(std::holds_alternative<number_t>(params.at(3).inner));
    BOOST_REQUIRE(std::holds_alternative<number_t>(params.at(4).inner));
    BOOST_REQUIRE(std::holds_alternative<string_t>(params.at(5).inner));
    BOOST_REQUIRE(std::holds_alternative<string_t>(params.at(6).inner));
    BOOST_CHECK_EQUAL(std::get<boolean_t>(params.at(1).inner), true);
    BOOST_CHECK_EQUAL(std::get<boolean_t>(params.at(2).inner), false);
    BOOST_CHECK_EQUAL(std::get<number_t>(params.at(3).inner), 42);
    BOOST_CHECK_EQUAL(std::get<number_t>(params.at(4).inner), -42);
    BOOST_CHECK_EQUAL(std::get<string_t>(params.at(5).inner), "foo");
    BOOST_CHECK_EQUAL(std::get<string_t>(params.at(6).inner), "bar");
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_object_mixed__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":{"a":null,"b":true,"c":false,"d":42,"e":-42,"f":"foo","g":"bar"}})" };
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(std::holds_alternative<object_t>(request.params.value()));

    const auto& params = std::get<object_t>(request.params.value());
    BOOST_REQUIRE_EQUAL(params.size(), 7u);
    BOOST_REQUIRE(std::holds_alternative<null_t>(params.at("a").inner));
    BOOST_REQUIRE(std::holds_alternative<boolean_t>(params.at("b").inner));
    BOOST_REQUIRE(std::holds_alternative<boolean_t>(params.at("c").inner));
    BOOST_REQUIRE(std::holds_alternative<number_t>(params.at("d").inner));
    BOOST_REQUIRE(std::holds_alternative<number_t>(params.at("e").inner));
    BOOST_REQUIRE(std::holds_alternative<string_t>(params.at("f").inner));
    BOOST_REQUIRE(std::holds_alternative<string_t>(params.at("g").inner));
    BOOST_CHECK_EQUAL(std::get<boolean_t>(params.at("b").inner), true);
    BOOST_CHECK_EQUAL(std::get<boolean_t>(params.at("c").inner), false);
    BOOST_CHECK_EQUAL(std::get<number_t>(params.at("d").inner), 42);
    BOOST_CHECK_EQUAL(std::get<number_t>(params.at("e").inner), -42);
    BOOST_CHECK_EQUAL(std::get<string_t>(params.at("f").inner), "foo");
    BOOST_CHECK_EQUAL(std::get<string_t>(params.at("g").inner), "bar");
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_array_single_array_empty__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":[[]]})" };
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(std::holds_alternative<array_t>(request.params.value()));

    const auto& array_params = std::get<array_t>(request.params.value());
    BOOST_REQUIRE_EQUAL(array_params.size(), one);

    const auto& first_param = array_params.front().inner;
    BOOST_REQUIRE(std::holds_alternative<array_t>(first_param));

    const auto& array = std::get<array_t>(first_param);
    const auto& first_value = array.front().inner;
    BOOST_REQUIRE(std::holds_alternative<string_t>(first_value));

    const auto& blob = std::get<string_t>(first_value);
    BOOST_CHECK_EQUAL(blob, "[]");
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_object_single_array_empty__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":{"abc":[]}})" };
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(std::holds_alternative<object_t>(request.params.value()));

    const auto& object_params = std::get<object_t>(request.params.value());
    BOOST_REQUIRE_EQUAL(object_params.size(), one);

    const auto& first_param = object_params.at("abc").inner;
    BOOST_REQUIRE(std::holds_alternative<array_t>(first_param));

    const auto& array = std::get<array_t>(first_param);
    const auto& first_value = array.front().inner;
    BOOST_REQUIRE(std::holds_alternative<string_t>(first_value));

    const auto& blob = std::get<string_t>(first_value);
    BOOST_CHECK_EQUAL(blob, "[]");
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_array_single_object_empty__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":[{}]})" };
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(std::holds_alternative<array_t>(request.params.value()));

    const auto& array_params = std::get<array_t>(request.params.value());
    BOOST_REQUIRE_EQUAL(array_params.size(), one);

    const auto& first_param = array_params.front().inner;
    BOOST_REQUIRE(std::holds_alternative<object_t>(first_param));

    const auto& object = std::get<object_t>(first_param);
    const auto& first_value = object.at("").inner;
    BOOST_REQUIRE(std::holds_alternative<string_t>(first_value));

    const auto& blob = std::get<string_t>(first_value);
    BOOST_CHECK_EQUAL(blob, "{}");
}

BOOST_AUTO_TEST_CASE(request_parser__write__params_object_single_object_empty__expected)
{
    request_parser parse{};
    const string_t text{ R"({"params":{"abc":{}}})" };
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(!parse.has_error());

    const auto request = parse.get_parsed().front();
    BOOST_REQUIRE(std::holds_alternative<object_t>(request.params.value()));

    const auto& object_params = std::get<object_t>(request.params.value());
    BOOST_REQUIRE_EQUAL(object_params.size(), one);

    const auto& first_param = object_params.at("abc").inner;
    BOOST_REQUIRE(std::holds_alternative<object_t>(first_param));

    const auto& object = std::get<object_t>(first_param);
    const auto& first_value = object.at("").inner;
    BOOST_REQUIRE(std::holds_alternative<string_t>(first_value));

    const auto& blob = std::get<string_t>(first_value);
    BOOST_CHECK_EQUAL(blob, "{}");
}

// TODO: expand params tests using test serialization function.

BOOST_AUTO_TEST_SUITE_END()
