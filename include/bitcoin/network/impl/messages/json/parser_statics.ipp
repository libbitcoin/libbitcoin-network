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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_STATICS_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_STATICS_IPP

#include <cctype>
#include <charconv>
#include <iterator>
#include <regex>
#include <variant>

namespace libbitcoin {
namespace network {
namespace json {

// protected/static

TEMPLATE
inline json::error_code CLASS::failure() NOEXCEPT
{
    // TODO: use network error codes.
    namespace errc = boost::system::errc;
    return errc::make_error_code(errc::invalid_argument);
}

TEMPLATE
inline json::error_code CLASS::incomplete() NOEXCEPT
{
    // TODO: use network error codes.
    namespace errc = boost::system::errc;
    return errc::make_error_code(errc::interrupted);
}

TEMPLATE
bool CLASS::is_null_t(const identity_t& id) NOEXCEPT
{
    return std::holds_alternative<null_t>(id);
}

TEMPLATE
constexpr bool CLASS::is_control(char c) NOEXCEPT
{
    // Quoted token characters in the range [0x00 - 0x1f].
    if (0x00 <= c && c <= 0x1f)
        return true;

    // The delete character [0x7f] often included, but this is not specified.
    if constexpr (strict)
    {
        return false;
    }
    else
    {
        return c == 0x7f;
    }
}

TEMPLATE
constexpr bool CLASS::is_prohibited(char c) NOEXCEPT
{
    // Reject non-whitespace control characters in all contexts.
    return is_control(c) && !is_whitespace(c);
}

TEMPLATE
constexpr bool CLASS::is_whitespace(char c) NOEXCEPT
{
    return c == ' '
        || c == '\n'
        || c == '\r'
        || c == '\t';
}

TEMPLATE
bool CLASS::is_numeric(char c) NOEXCEPT
{
    // 'e' is shared with true/false, those must be parsed first.
    return std::isdigit(c)
        || c == '-'
        || c == '.'
        || c == 'e'
        || c == 'E'
        || c == '+';
}

TEMPLATE
bool CLASS::is_truthy(const view_t& token, char c) NOEXCEPT
{
    // 'e' is shared with numeric, but since it is last here, it will always be
    // unambiguous if accepted here first, but this must preceed is_numeric().
    return (token.empty()  && c == 't')
        || (token == "t"   && c == 'r')
        || (token == "tr"  && c == 'u')
        || (token == "tru" && c == 'e');
}

TEMPLATE
bool CLASS::is_falsy(const view_t& token, char c) NOEXCEPT
{
    // 'e' is shared with numeric, but since it is last here, it will always be
    // unambiguous if accepted here first, but this must preceed is_numeric().
    return (token.empty()   && c == 'f')
        || (token == "f"    && c == 'a')
        || (token == "fa"   && c == 'l')
        || (token == "fal"  && c == 's')
        || (token == "fals" && c == 'e');
}

TEMPLATE
bool CLASS::is_nully(const view_t& token, char c) NOEXCEPT
{
    return (token.empty()  && c == 'n')
        || (token == "n"   && c == 'u')
        || (token == "nu"  && c == 'l')
        || (token == "nul" && c == 'l');
}

TEMPLATE
inline bool CLASS::is_error(const result_t& error) NOEXCEPT
{
    return !is_zero(error.code) || !error.message.empty();
}

TEMPLATE
inline bool CLASS::to_signed(code_t& out, const view_t& token) NOEXCEPT
{
    // JSON-RPC 2.0: Numbers SHOULD NOT contain fractional parts.
    // In other words, numbers can contain fractional parts :[. But we exclude
    // that in the "id" field using this utility. "params" allows JSON numbers.

    // TODO: unit test to ensure empty token always produces false (not zero).
    const auto end = std::next(token.data(), token.size());
    return is_zero(std::from_chars(token.data(), end, out).ec);
}

TEMPLATE
inline bool CLASS::to_number(number_t& out, const view_t& token) NOEXCEPT
{
    static const std::regex json_number{
        R"(-?(0|[1-9]\d*)(\.\d+)?([eE][+-]?\d+)?$)" };

    try
    {
        if (!std::regex_match(token.begin(), token.end(), json_number))
            return false;
    }
    catch (...)
    {
    }

#if defined(HAVE_APPLE)
    errno = 0;
    char* end = nullptr;
    out = std::strtod(token.data(), &end);
    return (errno != ERANGE) && (end == std::next(token.data(), token.size()));
#else
    const auto end = std::next(token.data(), token.size());
    const auto result = std::from_chars(token.data(), end, out);
    return (is_zero(result.ec) && result.ptr == end);
#endif
}

TEMPLATE
inline bool CLASS::toggle(bool& quoted) NOEXCEPT
{
    return !((quoted = !quoted));
}

TEMPLATE
inline size_t CLASS::distance(const char_it& from,
    const char_it& to) NOEXCEPT
{
    using namespace system;
    return possible_narrow_and_sign_cast<size_t>(std::distance(from, to));
}
 
} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
