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
#include <variant>

namespace libbitcoin {
namespace network {
namespace json {
    
// protected/static

TEMPLATE
inline json::error_code CLASS::parse_error() NOEXCEPT
{
    namespace errc = boost::system::errc;
    return errc::make_error_code(errc::invalid_argument);
}

TEMPLATE
bool CLASS::is_null(const id_t& id) NOEXCEPT
{
    return std::holds_alternative<null_t>(id);
}

TEMPLATE
bool CLASS::is_numeric(char c) NOEXCEPT
{
    return std::isdigit(c)
        || c == '-'
        || c == '.'
        || c == 'e'
        || c == 'E'
        || c == '+';
}

TEMPLATE
bool CLASS::is_whitespace(char c) NOEXCEPT
{
    return c == ' '
        || c == '\n'
        || c == '\r'
        || c == '\t';
}

TEMPLATE
bool CLASS::is_nullic(view token, char c) NOEXCEPT
{
    return (value_.empty()  && c == 'n')
        || (value_ == "n"   && c == 'u')
        || (value_ == "nu"  && c == 'l')
        || (value_ == "nul" && c == 'l');
}

TEMPLATE
inline bool CLASS::is_error(const result_t& error) NOEXCEPT
{
    return !is_zero(error.code) || !error.message.empty();
}

TEMPLATE
inline bool CLASS::to_number(int64_t& out, view token) NOEXCEPT
{
    // BUGBUG: accept/convert any valid json number and reject all others.
    const auto end = std::next(token.data(), token.size());
    return is_zero(std::from_chars(token.data(), end, out).ec);
}

TEMPLATE
inline id_t CLASS::to_id(view token) NOEXCEPT
{
    int64_t out{};
    if (to_number(out, token))
    {
        // BUGBUG: conflates with quoted.
        return out;
    }
    else if (token == "null")
    {
        // BUGBUG: conflates with quoted.
        return null_t{};
    }
    else
    {
        // BUGBUG: conflates with null/number.
        return string_t{ token };
    }
}

TEMPLATE
inline bool CLASS::toggle(bool& quoted) NOEXCEPT
{
    return !((quoted = !quoted));
}

TEMPLATE
inline bool CLASS::increment(size_t& depth, state& status) NOEXCEPT
{
    if (is_zero(++depth))
    {
        status = state::error_state;
        return false;
    }

    return true;
}

TEMPLATE
inline bool CLASS::decrement(size_t& depth, state& status) NOEXCEPT
{
    if (is_zero(depth--))
    {
        status = state::error_state;
        return false;
    }

    return true;
}

TEMPLATE
inline size_t CLASS::distance(const char_iterator& from,
    const char_iterator& to) NOEXCEPT
{
    using namespace system;
    return possible_narrow_and_sign_cast<size_t>(std::distance(from, to));
}
 
} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
