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

TEMPLATE
void CLASS::handle_initialize(char c) NOEXCEPT
{
    if (c == '[')
    {
        batched_ = true;
        state_ = state::batch_start;
    }
    else if (c == '{')
    {
        request_ = add_request();
        state_ = state::request_start;
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_batch_start(char c) NOEXCEPT
{
    if (c == ',')
    {
        state_ = state::batch_start;
    }
    else if (c == '{')
    {
        reset_internal();
        request_ = add_request();
        state_ = state::request_start;
    }
    else if (c == ']')
    {
        state_ = state::complete;
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_request_start(char c) NOEXCEPT
{
    if (c == ',')
    {
        state_ = state::request_start;
    }
    else if (c == '"')
    {
        state_ = state::key;
    }
    else if (c == '}')
    {
        state_ = batched_ ? state::batch_start : state::complete;
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

// key discovery to value type dispatch
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::handle_key(char c) NOEXCEPT
{
    // Initiated by opening '"' [in handle_request_start] so no ws skipping.
    // In state::key, at trailing '"', state assigned based on accumulated key.

    if (c != '"')
    {
        consume_quoted(key_);
    }
    else if (key_ == "jsonrpc")
    {
        state_ = state::value;
    }
    else if (key_ == "id")
    {
        state_ = state::value;
    }
    else if (key_ == "data")
    {
        state_ = state::value;
    }
    else if (key_ == "method")
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

TEMPLATE
void CLASS::handle_value(char c) NOEXCEPT
{
    // Initiated by trailing '"' [in handle_key] so yes ws skipping.
    // In state::value, at first ':', state changes based on current key.

    if (is_whitespace(c))
    {
        return;
    }
    else if (c != ':')
    {
        state_ = state::error_state;
        return;
    }
    else if (key_ == "jsonrpc")
    {
        state_ = state::jsonrpc;
    }
    else if (key_ == "id")
    {
        state_ = state::id;
    }
    else if (key_ == "method")
    {
        state_ = state::method;
    }
    else if (key_ == "params")
    {
        state_ = state::params;
    }
    else
    {
        state_ = state::error_state;
    }

    key_ = {};
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
