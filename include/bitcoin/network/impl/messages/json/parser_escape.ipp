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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_ESCAPE_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_ESCAPE_IPP

namespace libbitcoin {
namespace network {
namespace json {

// protected

TEMPLATE
inline void CLASS::consume_substitute(view& token, char /* c */) NOEXCEPT
{
    // BUGBUG: view is not modifiable, requires dynamic token (vs. view).
    consume(token, char_);
}

TEMPLATE
inline void CLASS::consume_escaped(view& token, char c) NOEXCEPT
{
    // BUGBUG: doesn't support \uXXXX, requires 4 character accumulation.
    switch (c)
    {
        case 'b':
            consume(token, '\b');
            return;
        case 'f':
            consume(token, '\f');
            return;
        case 'n':
            consume(token, '\n');
            return;
        case 'r':
            consume(token, '\r');
            return;
        case 't':
            consume(token, '\t');
            return;
        default:
            consume(token, char_);
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
