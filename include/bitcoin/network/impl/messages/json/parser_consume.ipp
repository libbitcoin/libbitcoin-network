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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_CONSUME_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_CONSUME_IPP

#include <memory>

namespace libbitcoin {
namespace network {
namespace json {

// protected

TEMPLATE
inline bool CLASS::consume_substitute(view_t&, char) NOEXCEPT
{
    // BUGBUG: view is not modifiable, requires dynamic token (vs. view_t).
    return false; ////consume_char(token);
}

TEMPLATE
inline bool CLASS::consume_escaped(view_t& token, char c) NOEXCEPT
{
    // BUGBUG: doesn't support \uXXXX, requires 4 character accumulation.
    switch (c)
    {
        case 'b': c = '\b'; break;
        case 'f': c = '\f'; break;
        case 'n': c = '\n'; break;
        case 'r': c = '\r'; break;
        case 't': c = '\t'; break;
        default:
            return false;
    }

    return consume_substitute(token, c);
}

TEMPLATE
inline bool CLASS::consume_escape(view_t& token, char c) NOEXCEPT
{
    if (c == '\\' && !escaped_)
    {
        escaped_ = true;
        return true;
    }
    else if (escaped_)
    {
        escaped_ = false;
        return consume_escaped(token, c);
    }
    else
    {
        escaped_ = false;
        return false;
    }
}

TEMPLATE
inline size_t CLASS::consume_quoted(view_t& token) NOEXCEPT
{
    // BUGBUG: escape sequences not yet supported.
    return consume_char(token);
}

TEMPLATE
inline size_t CLASS::consume_char(view_t& token) NOEXCEPT
{
    // Token consumes character *char_ by incrementing its view over buffer.
    const auto size = add1(token.size());
    const auto start = token.empty() ? std::to_address(char_) : token.data();
    token = { start, size };
    return size;
}

// blobs
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::consume_blob(view_t& token) NOEXCEPT
{
    if (char_ == end_ || (*char_ != '[' && *char_ != '{'))
        return false;

    const auto start = char_;
    const auto open = *start;
    const auto close = (open == '[') ? ']' : '}';

    auto escaped = false;
    auto quoted = false;
    auto depth = one;

    while (++char_ != end_)
    {
        const char ch = *char_;

        if (escaped)
        {
            escaped = false;
        }
        else if (ch == '\\')
        {
            escaped = true;
        }
        else if (quoted)
        {
            if (ch == '"')
                quoted = false;
        }
        else if (ch == '"')
        {
            quoted = true;
        }
        else if (ch == open)
        {
            ++depth;
        }
        else if (ch == close)
        {
            if (is_zero(--depth))
            {
                token =
                {
                    std::to_address(start), distance(start, std::next(char_))
                };
                return true;
            }
        }
    }

    return false;
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
