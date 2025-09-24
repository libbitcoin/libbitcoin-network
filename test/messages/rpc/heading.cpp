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
#include <sstream>

BOOST_AUTO_TEST_SUITE(rpc_heading_tests)

using namespace system;
using namespace network::messages::rpc;

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields_from_fields__empty__empty)
{
    const heading::fields original{};
    std::ostringstream out;
    write::bytes::ostream writer(out);
    heading::from_fields(original, writer);
    std::istringstream in(out.str());
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE(fields.empty());
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields_from_fields__empty_value__round_trip)
{
    const heading::fields original{ { "x-custom-header", "" } };
    std::ostringstream out;
    write::bytes::ostream writer(out);
    heading::from_fields(original, writer);
    std::istringstream in(out.str());
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE_EQUAL(fields.size(), 1);
    BOOST_REQUIRE_EQUAL(fields.begin()->first, "x-custom-header");
    BOOST_REQUIRE_EQUAL(fields.begin()->second, "");
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields_from_fields__unquoted_value__round_trip)
{
    const heading::fields original{ { "content-type", "text/plain" } };
    std::ostringstream out;
    write::bytes::ostream writer(out);
    heading::from_fields(original, writer);
    std::istringstream in(out.str());
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE_EQUAL(fields.size(), 1);
    BOOST_REQUIRE_EQUAL(fields.begin()->first, "content-type");
    BOOST_REQUIRE_EQUAL(fields.begin()->second, "text/plain");
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields_from_fields__quoted_value__round_trip)
{
    const heading::fields original{ { "content-type", "text/plain; charset=UTF-8" } };
    std::ostringstream out;
    write::bytes::ostream writer(out);
    heading::from_fields(original, writer);
    std::istringstream in(out.str());
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE_EQUAL(fields.size(), 1);
    BOOST_REQUIRE_EQUAL(fields.begin()->first, "content-type");
    BOOST_REQUIRE_EQUAL(fields.begin()->second, "text/plain; charset=UTF-8");
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields_from_fields__quoted_escapes__round_trip)
{
    const heading::fields original{ { "content-type", "text/plain; charset=\"UTF-8 \\\"quoted\\\"\"" } };
    std::ostringstream out;
    write::bytes::ostream writer(out);
    heading::from_fields(original, writer);
    std::istringstream in(out.str());
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE_EQUAL(fields.size(), 1);
    BOOST_REQUIRE_EQUAL(fields.begin()->first, "content-type");
    BOOST_REQUIRE_EQUAL(fields.begin()->second, "text/plain; charset=\"UTF-8 \\\"quoted\\\"\"");
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields_from_fields__multiple_headers__round_trip)
{
    const heading::fields original{ { "accept", "application/json" }, { "content-type", "text/plain" }, { "x-custom-header", "" } };
    std::ostringstream out;
    write::bytes::ostream writer(out);
    heading::from_fields(original, writer);
    std::istringstream in(out.str());
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE_EQUAL(fields.size(), 3);
    auto it = fields.begin();
    BOOST_REQUIRE_EQUAL(it->first, "accept");
    BOOST_REQUIRE_EQUAL(it->second, "application/json");
    ++it;
    BOOST_REQUIRE_EQUAL(it->first, "content-type");
    BOOST_REQUIRE_EQUAL(it->second, "text/plain");
    ++it;
    BOOST_REQUIRE_EQUAL(it->first, "x-custom-header");
    BOOST_REQUIRE_EQUAL(it->second, "");
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields__invalid_field_name__empty)
{
    std::istringstream in("Invalid Field: text/plain\r\n\r\n");
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE(fields.empty());
    BOOST_CHECK(!reader);
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields__unquoted_control_character__empty)
{
    std::istringstream in("Content-Type: text\nplain\r\n\r\n");
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE(fields.empty());
    BOOST_CHECK(!reader);
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields__quoted_control_character__empty)
{
    std::istringstream in("Content-Type: \"text\nplain\"\r\n\r\n");
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE(fields.empty());
    BOOST_CHECK(!reader);
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields__unterminated_quoted_pair__empty)
{
    std::istringstream in("Content-Type: \"text\"\"\r\n\r\n");
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE(fields.empty());
    BOOST_CHECK(!reader);
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields__invalid_quoted_pair__empty)
{
    std::istringstream in("Content-Type: \"text\\z\"\r\n\r\n");
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE(fields.empty());
    BOOST_CHECK(!reader);
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields__whitespace_only_value__empty_value)
{
    std::istringstream in("Content-Type:   \r\n\r\n");
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE_EQUAL(fields.size(), 1);
    BOOST_REQUIRE_EQUAL(fields.begin()->first, "content-type");
    BOOST_REQUIRE_EQUAL(fields.begin()->second, "");
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields__unquoted_nul__empty)
{
    std::istringstream in("Content-Type: text\x00plain\r\n\r\n");
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE(fields.empty());
    BOOST_CHECK(!reader);
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields__quoted_nul__empty)
{
    std::istringstream in("Content-Type: \"text\x00plain\"\r\n\r\n");
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE(fields.empty());
    BOOST_CHECK(!reader);
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields__missing_final_crlf__empty)
{
    std::istringstream in("Content-Type: text/plain");
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE(fields.empty());
    BOOST_CHECK(!reader);
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields__empty_quoted__empty_value)
{
    std::istringstream in("Content-Type: \"\"\r\n\r\n");
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE_EQUAL(fields.size(), 1);
    BOOST_REQUIRE_EQUAL(fields.begin()->first, "content-type");
    BOOST_REQUIRE_EQUAL(fields.begin()->second, "");
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields__host_empty__empty_value)
{
    std::istringstream in("Host: \r\n\r\n");
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE_EQUAL(fields.size(), 1);
    BOOST_REQUIRE_EQUAL(fields.begin()->first, "host");
    BOOST_REQUIRE_EQUAL(fields.begin()->second, "");
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields__unquoted_ctl__empty)
{
    std::istringstream in("Content-Type: text\x01plain\r\n\r\n");
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE(fields.empty());
    BOOST_CHECK(!reader);
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_fields__quoted_ctl__empty)
{
    std::istringstream in("Content-Type: \"text\x01plain\"\r\n\r\n");
    read::bytes::istream reader(in);
    const auto fields = heading::to_fields(reader);
    BOOST_REQUIRE(fields.empty());
    BOOST_CHECK(!reader);
}

BOOST_AUTO_TEST_CASE(rpc_heading__fields_size__multiple_headers__expected)
{
    const heading::fields headers{ { "content-type", "text/plain" }, { "accept", "application/json" } };
    const size_t expected = 12 + 2 + 10 + 2 + 6 + 2 + 16 + 2;
    BOOST_REQUIRE_EQUAL(heading::fields_size(headers), expected);
}

class accessor : public heading
{
public:
    static bool validate_unquoted_value_(std::string_view value) NOEXCEPT
    {
        return validate_unquoted_value(value);
    }
    static string_t unescape_quoted_value_(std::string_view value) NOEXCEPT
    {
        return unescape_quoted_value(value);
    }
    static string_t to_field_name_(const std::string& value) NOEXCEPT
    {
        return to_field_name(value);
    }
    static string_t to_field_value_(std::string&& value) NOEXCEPT
    {
        return to_field_value(std::move(value));
    }
};

BOOST_AUTO_TEST_CASE(rpc_heading__to_field_name__valid_token__lowercase)
{
    const auto result = accessor::to_field_name_("Content-Type");
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(*result, "content-type");
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_field_name__invalid_token__nullopt)
{
    const auto result = accessor::to_field_name_("Invalid Field");
    BOOST_REQUIRE(!result);
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_field_value__empty__empty)
{
    const auto result = accessor::to_field_value_("");
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(*result, "");
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_field_value__whitespace_only__empty)
{
    const auto result = accessor::to_field_value_("   ");
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(*result, "");
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_field_value__unquoted_valid__unchanged)
{
    const auto result = accessor::to_field_value_("text/plain");
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(*result, "text/plain");
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_field_value__quoted_valid__unescaped)
{
    const auto result = accessor::to_field_value_("\"UTF-8 \\\"quoted\\\"\"");
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(*result, "UTF-8 \"quoted\"");
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_field_value__unquoted_control__nullopt)
{
    const auto result = accessor::to_field_value_("text\nplain");
    BOOST_REQUIRE(!result);
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_field_value__quoted_control__nullopt)
{
    const auto result = accessor::to_field_value_("\"text\nplain\"");
    BOOST_REQUIRE(!result);
}

BOOST_AUTO_TEST_CASE(rpc_heading__to_field_value__empty_quoted__empty)
{
    const auto result = accessor::to_field_value_("\"\"");
    BOOST_REQUIRE(result);
    BOOST_REQUIRE_EQUAL(*result, "");
}

BOOST_AUTO_TEST_SUITE_END()
