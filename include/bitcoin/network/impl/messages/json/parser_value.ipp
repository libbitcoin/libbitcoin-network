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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_VISITORS_VALUE_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_VISITORS_VALUE_IPP

namespace libbitcoin {
namespace network {
namespace json {

TEMPLATE
void CLASS::handle_jsonrpc(char c) NOEXCEPT
{
    // string is terminated by its closing quote.
    if (c == '"')
    {
        if (toggle(quoted_))
            assign_version(request_->jsonrpc, value_);
    }
    else if (quoted_)
    {
        consume_quoted(value_);
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_method(char c) NOEXCEPT
{
    // string is terminated by its closing quote.
    if (c == '"')
    {
        if (toggle(quoted_))
            assign_string(request_->method, value_);
    }
    else if (quoted_)
    {
        consume_quoted(value_);
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_params(char c) NOEXCEPT
{
    // string is terminated by its closing quote.
    if (c == '"')
    {
        if (toggle(quoted_))
            assign_value(request_->params, value_);
    }
    else if (quoted_)
    {
        consume_quoted(value_);
    }

    // BUGBUG: possible overlapping terminal chars and depth confusion.
    else if (c == '[' || c == '{')
    {
        increment(depth_, state_);
    }
    else if (c == ']' || c == '}')
    {
        decrement(depth_, state_);
    }
    else if (c == ',')
    {
        // BUGBUG: depth assumption correct?
        // BUGBUG: leading (prob not trailing) ',' is ignored.
        if (is_one(depth_))
            assign_value(request_->params, value_);
        else
            state_ = state::error_state;
    }

    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_id(char c) NOEXCEPT
{
    // string is terminated by its closing quote.
    if (c == '"')
    {
        if (toggle(quoted_))
            assign_string_id(request_->id, value_);
    }
    else if (quoted_)
    {
        consume_quoted(value_);
    }

    // null is terminated by its 4th character.
    else if (is_nullic(value_, c))
    {
        if (consume_char(value_) == null_size)
            assign_null_id(request_->id, value_);
    }

    // BUGBUG: terminal characters, ] unhandled?
    // numeric is terminated by a terminal character for its context.
    else if (is_numeric(c))
    {
        consume_char(value_);
    }
    else if (c == ',')
    {
        // BUGBUG: depth assumption correct?
        if (is_one(depth_))
            assign_numeric_id(request_->id, value_);
        else
            state_ = state::error_state;
    }
    else if (c == '}')
    {
        // BUGBUG: depth assumption correct?
        if (is_one(depth_))
        {
            if (!decrement(depth_, state_))
                return;

            if (assign_numeric_id(request_->id, value_))
                state_ = state::complete;
        }
        else
        {
            state_ = state::error_state;
        }
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
