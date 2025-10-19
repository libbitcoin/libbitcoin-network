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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_VISITORS_STATE_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_VISITORS_STATE_IPP

namespace libbitcoin {
namespace network {
namespace json {

// protected

TEMPLATE
void CLASS::handle_initialize(char c) NOEXCEPT
{
    // delimiters cannot be escaped.

    if (c == '{')
    {
        batched_ = false;
        parsed_ = add_remote_procedure_call();

        state_ = state::object_start;
        increment(depth_, state_);
        return;
    }
    else if (c == '[')
    {
        batched_ = true;

        state_ = state::object_start;
        increment(depth_, state_);
        return;
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_object_start(char c) NOEXCEPT
{
    // delimiters cannot be escaped.

    if (c == '"')
    {
        // state::key implies quoted.
        state_ = state::key;
    }
    else if (c == '}')
    {
        if (!decrement(depth_, state_))
            return;

        if (is_zero(depth_))
            state_ = state::complete;
    }
    else if (batched_)
    {
        if (c == '{')
        {
            if (is_one(depth_))
            {
                parsed_ = add_remote_procedure_call();
                increment(depth_, state_);
            }
            else
            {
                state_ = state::error_state;
            }
        }
        else if (c == ']')
        {
            if (is_one(depth_))
            {
                state_ = state::complete;
                decrement(depth_, state_);
            }
            else
            {
                state_ = state::error_state;
            }
        }
        else if (c == ',')
        {
            if (is_one(depth_))
            {
                // no depth change.
                state_ = state::object_start;
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
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

// TODO:
// Shift to key-based parsing inside errors by adding error-specific key handling,
// or route to error-specific handlers [if key_ == "code", go to handle_error_code].
TEMPLATE
void CLASS::handle_key(char c) NOEXCEPT
{
    // terminating quote cannot be an escape or be escaped.
    if (consume_escape(key_, c))
        return;

    // consume non-quote.
    if (c != '"')
    {
        consume_char(key_);
        return;
    }

    // In state::key, upon '"' state changes based on accumulated key.
    if (key_ == "jsonrpc")
    {
        state_ = state::value;
    }
    else if (key_ == "id")
    {
        state_ = state::value;
    }
    else if (key_ == "code")
    {
        state_ = state::value;
    }
    else if (key_ == "message")
    {
        state_ = state::value;
    }
    else if (key_ == "data")
    {
        state_ = state::value;
    }
    else if constexpr (request)
    {
        if (key_ == "method")
        {
            state_ = state::value;
        }
        else if (key_ == "params")
        {
            state_ = state::value;
        }
        else
        {
            state_ = state::error_state;
        }
    }
    else if constexpr (request)
    {
        if (key_ == "result")
        {
            state_ = state::value;
        }
        else if (key_ == "error")
        {
            state_ = state::value;
        }
        else
        {
            state_ = state::error_state;
        }
    }
}

// TODO:
// Shift to key-based parsing inside errors by adding error-specific key handling,
// or route to error-specific handlers [if key_ == "code", go to handle_error_code].
TEMPLATE
void CLASS::handle_value(char c) NOEXCEPT
{
    if (is_whitespace(c))
        return;

    if (c != ':')
    {
        state_ = state::error_state;
        return;
    }

    // In state::value, upon ':' state changes based on current key.
    if (key_ == "jsonrpc")
    {
        state_ = state::jsonrpc;
    }
    else if (key_ == "id")
    {
        state_ = state::id;
    }
    else if (key_ == "code")
    {
        state_ = state::error_code;
    }
    else if (key_ == "message")
    {
        state_ = state::error_message;
    }
    else if (key_ == "data")
    {
        state_ = state::error_data;
    }
    else if constexpr (request)
    {
        if (request && key_ == "method")
        {
            state_ = state::method;
        }
        else if (request && key_ == "params")
        {
            state_ = state::params;
        }
        else
        {
            state_ = state::error_state;
        }
    }
    else if constexpr (request)
    {
        if (response && key_ == "result")
        {
            state_ = state::result;
        }
        else if (response && key_ == "error")
        {
            state_ = state::error_start;
        }
        else
        {
            state_ = state::error_state;
        }
    }
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
