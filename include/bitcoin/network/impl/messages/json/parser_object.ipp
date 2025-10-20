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
    if (c == '{')
    {
        state_ = state::object_start;
        request_ = add_request();
        expected_ = '}';
    }
    else if (c == '[')
    {
        state_ = state::array_start;
        batched_ = true;
        expected_ = ']';
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_array_start(char c) NOEXCEPT
{
    if (c == '{')
    {
        reset_internal();
        state_ = state::object_start;
        request_ = add_request();
        expected_ = '}';
    }
    else if (c == ']' && expected_ == ']')
    {
        state_ = batch_.empty() ? state::error_state : state::complete;
    }

    // Leading batch array ',' and unexpected ']' are caught here.
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_object_start(char c) NOEXCEPT
{
    // key is terminated by its closing quote in handle_key.
    // nvp start (will close), so at least one element in current csv set.
    if (c == '"')
    {
        trailing_ = true;
        state_ = state::key;
    }

    else if (c == ',' && trailing_)
    {
        state_ = state::object_start;
    }

    // object close, so at least one element in current csv set.
    else if (c == '}' && expected_ == '}')
    {
        trailing_ = true;
        state_ = batched_ ? state::array_start : state::complete;
    }

    else if (c == ']' && expected_ == ']')
    {
        state_ = state::complete;
    }

    // Leading object/array ',' and unexpected ']' and '}' are caught here.
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_key(char c) NOEXCEPT
{
    // Initiated by quote [in handle_object_start] so no whitespace skipping.

    if (c != '"')
    {
        consume_quoted(key_);
        return;
    }

    // In state::key, upon '"' state changes based on accumulated key chars.
    if (key_ == "jsonrpc")
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
