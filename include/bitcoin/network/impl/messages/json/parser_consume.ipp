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

#include <iterator>
#include <memory>

namespace libbitcoin {
namespace network {
namespace json {

// consume_char
// ----------------------------------------------------------------------------
// Accumulate a single character into the token view.

TEMPLATE
inline size_t CLASS::consume_char(view_t& token) NOEXCEPT
{
    // Token consumes character *char_ by incrementing its view over buffer.
    const auto size = add1(token.size());
    const auto start = token.empty() ? std::to_address(char_) : token.data();
    token = { start, size };
    return size;
}

// spans (consume_text/consume_span)
// ----------------------------------------------------------------------------
// These handle but do not substitute escape sequences. Escape sequences are
// retained for processing after string object is constructed from the view.
// This allows parsing to remain non-allocating, eliminating reallocations from
// the progressive accumulation of characters into a dynamic string object.

TEMPLATE
inline bool CLASS::consume_text(view_t& token) NOEXCEPT
{
    if (char_ == end_ || *char_ != '"')
        return false;

    const auto start = char_;
    auto escaped = false;

    while (++char_ != end_)
    {
        const char c = *char_;

        if (is_prohibited(c))
        {
            return false;
        }
        else if (escaped)
        {
            escaped = false;
        }
        else if (c == '\\')
        {
            escaped = true;
        }
        else if (c == '"')
        {
            token =
            {
                std::next(std::to_address(start)), sub1(distance(start, char_))
            };

            return true;
        }
    }

    return false;
}

TEMPLATE
inline bool CLASS::consume_span(view_t& token) NOEXCEPT
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
        const char c = *char_;

        if (is_prohibited(c))
        {
            return false;
        }
        else if (escaped)
        {
            escaped = false;
        }
        else if (quoted && c == '\\')
        {
            escaped = true;
        }
        else if (c == '"')
        {
            quoted = !quoted;
        }
        else if (quoted)
        {
            // nop
        }
        else if (c == open)
        {
            ++depth;
        }
        else if (c == close)
        {
            --depth;

            if (is_zero(depth))
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
