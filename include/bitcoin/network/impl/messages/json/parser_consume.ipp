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
inline void CLASS::consume_char(view& token) NOEXCEPT
{
    // Token consumes character *char_ by incrementing its view over buffer.
    if (token.empty())
        token = { std::to_address(char_), one };
    else
        token = { token.data(), add1(token.size()) };
}

TEMPLATE
inline void CLASS::consume_substitute(view& token, char /* c */) NOEXCEPT
{
    // BUGBUG: view is not modifiable, requires dynamic token (vs. view).
    consume_char(token);
}

TEMPLATE
inline void CLASS::consume_escaped(view& token, char c) NOEXCEPT
{
    // BUGBUG: doesn't support \uXXXX, requires 4 character accumulation.
    switch (c)
    {
        case 'b':
            consume_substitute(token, '\b');
            return;
        case 'f':
            consume_substitute(token, '\f');
            return;
        case 'n':
            consume_substitute(token, '\n');
            return;
        case 'r':
            consume_substitute(token, '\r');
            return;
        case 't':
            consume_substitute(token, '\t');
            return;
        default:
            consume_char(token);
    }
}

TEMPLATE
inline bool CLASS::consume_escape(view& token, char c) NOEXCEPT
{
    if (c == '\\' && !escaped_)
    {
        escaped_ = true;
        return true;
    }
    else if (escaped_)
    {
        consume_escaped(token, c);
        escaped_ = false;
        return true;
    }
    else
    {
        escaped_ = false;
        return false;
    }
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
