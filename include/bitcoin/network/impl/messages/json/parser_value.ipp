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

    // TODO: use siliar to id processing, but use full precision json number.
    // TODO: change value type to 

    ////// array [positional] or object {named}.
    ////else if (c == '[')
    ////{
    ////    // BUGBUG: depth conflict with {}.
    ////    // BUGBUG: depth inconsistent within [], see batch.
    ////    increment(depth_, state_);
    ////    expected_ = ']';
    ////}
    ////else if (c == '{')
    ////{
    ////    // BUGBUG: depth conflict with [].
    ////    increment(depth_, state_);
    ////    expected_ = '}';
    ////}
    ////else if (c == ']' || c == '}')
    ////{
    ////    if ((c == ']' && expected_ != ']') ||
    ////        (c == '}' && expected_ != '}'))
    ////    {
    ////        state_ = state::error_state;
    ////        return;
    ////    }
    ////
    ////    // BUGBUG: depth issues (above).
    ////    if (!decrement(depth_, state_))
    ////        return;
    ////
    ////    // BUGBUG: depth issues (above).
    ////    // BUGBUG: string assignment only.
    ////    if (is_one(depth_))
    ////        assign_value(request_->params, value_);
    ////    else
    ////        state_ = state::error_state;
    ////}
    ////else if (c == ',' && trailing_)
    ////{
    ////    // BUGBUG: depth issues (above).
    ////    // BUGBUG: string assignment only.
    ////    if (is_one(depth_) && !value_.empty())
    ////        assign_value(request_->params, value_);
    ////    else
    ////        state_ = state::error_state;
    ////}

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

    // number is terminated by the first non-number character.
    else if (is_numeric(c))
    {
        consume_char(value_);
    }
    else if (is_whitespace(c) && value_.empty())
    {
        // skip whitespace before first number character.
        return;
    }
    else if (!assign_numeric_id(request_->id, value_))
    {
        // empty string or other failures, state set by number parse.
        return;
    }
    else
    {
        // redispatch the terminating character.
        state_ = state::object_start;
        handle_object_start(c);
    }
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
