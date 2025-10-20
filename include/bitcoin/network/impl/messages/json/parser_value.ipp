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

// protected

// quoted value handlers.
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::handle_jsonrpc(char c) NOEXCEPT
{
    if (consume_escape(value_, c))
        return;

    if (c == '"')
    {
        if (toggle(quoted_))
            assign_version(parsed_->jsonrpc, value_);
    }
    else if (quoted_)
    {
        consume_char(value_);
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_method(char c) NOEXCEPT
{
    if (consume_escape(value_, c))
        return;

    if (c == '"')
    {
        if (toggle(quoted_))
            ASSIGN_REQUEST(string, parsed_->method, value_)
    }
    else if (quoted_)
    {
        consume_char(value_);
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_params(char c) NOEXCEPT
{
    if (consume_escape(value_, c))
        return;

    if (c == '"')
    {
        if (toggle(quoted_))
            ASSIGN_REQUEST(value, parsed_->params, value_)
    }
    else if (quoted_)
    {
        consume_char(value_);
    }
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
        if (is_one(depth_))
            ASSIGN_REQUEST(value, parsed_->params, value_)
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
    if (consume_escape(value_, c))
        return;

    if (c == '"')
    {
        if (toggle(quoted_))
            assign_string_id(parsed_->id, value_);
    }
    else if (quoted_)
    {
        consume_char(value_);
    }
    else if (is_nullic(value_, c))
    {
        if (consume_char(value_) == null_size)
            assign_null_id(parsed_->id, value_);
    }
    else if (is_numeric(c))
    {
        consume_char(value_);
    }
    else if (c == ',')
    {
        if (is_one(depth_))
            assign_numeric_id(parsed_->id, value_);
        else
            state_ = state::error_state;
    }
    else if (c == '}')
    {
        if (is_one(depth_))
        {
            if (!decrement(depth_, state_))
                return;

            if (assign_numeric_id(parsed_->id, value_))
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

TEMPLATE
void CLASS::handle_result(char c) NOEXCEPT
{
    if (consume_escape(value_, c))
        return;

    if (c == '"')
    {
        if (toggle(quoted_))
            ASSIGN_RESPONSE(value, parsed_->result, value_)
    }
    else if (quoted_)
    {
        consume_char(value_);
    }
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
        if (is_one(depth_))
            ASSIGN_RESPONSE(value, parsed_->result, value_)
        else
            state_ = state::error_state;
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_error_message(char c) NOEXCEPT
{
    if (consume_escape(value_, c))
        return;

    if (c == '"')
    {
        if (toggle(quoted_))
            assign_string(error_.message, value_);
    }
    else if (quoted_)
    {
        consume_char(value_);
    }
    else if (c == ',')
    {
        state_ = state::object_start;
    }
    else if (c == '}')
    {
        state_ = state::object_start;
        decrement(depth_, state_);
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_error_data(char c) NOEXCEPT
{
    if (consume_escape(value_, c))
        return;

    if (c == '"')
    {
        if (toggle(quoted_))
            ASSIGN_RESPONSE(value, error_.data, value_)
    }
    else if (quoted_)
    {
        consume_char(value_);
    }
    else if (c == '[' || c == '{')
    {
        increment(depth_, state_);
    }
    else if (c == ']' || c == '}')
    {
        if (!decrement(depth_, state_))
            return;

        if (c == '}')
        {
            if (is_one(depth_) && is_error(error_))
                ASSIGN_RESPONSE(error, parsed_->error, error_)
            else
                state_ = state::error_state;
        }
    }
    else if (c == ',')
    {
        if (is_one(depth_))
            assign_value(error_.data, value_);
        else
            state_ = state::error_state;
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

// unquoted value handlers.
// ----------------------------------------------------------------------------

// This is both state visitor and value visitor.
TEMPLATE
void CLASS::handle_error_start(char c) NOEXCEPT
{
    if (c == '{')
    {
        state_ = state::object_start;
        increment(depth_, state_);
    }
    else if (is_nullic(value_, c))
    {
        consume_char(value_);
        if (value_ == "null")
            ASSIGN_RESPONSE(error, parsed_->error, {})
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_error_code(char c) NOEXCEPT
{
    if (is_numeric(c))
    {
        consume_char(value_);
    }
    else if (c == ',' || c == '}')
    {
        assign_numeric_id(error_.code, value_);
        if (c == '}')
            decrement(depth_, state_);
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
