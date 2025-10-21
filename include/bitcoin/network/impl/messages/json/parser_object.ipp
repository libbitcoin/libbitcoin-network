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

// startup
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::handle_root(char c) NOEXCEPT
{
    // There is only one root element, so no comma processing.

    if (c == '[')
    {
        batched_ = true;
        state_ = state::batch_start;
    }
    else if (c == '{')
    {
        request_ = add_request(batch_);
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
        request_ = add_request(batch_);
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

// properties
// ----------------------------------------------------------------------------

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

// parameters
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::handle_params_start(char c) NOEXCEPT
{
    const auto array_params = is_array(request_->params);

    if (c == ',')
    {
        state_ = state::params_start;
    }
    else if (c == '"')
    {
        state_ = array_params ? state::parameter : state::parameter_key;
    }
    else if (c == '}' && !array_params)
    {
        state_ = state::request_start;
    }
    else if (c == ']' && array_params)
    {
        state_ = state::request_start;
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_parameter_key(char c) NOEXCEPT
{
    // Initiated by opening '"' [in named handle_parameter] so no ws skipping.
    // In state::key, at trailing '"', state assigned based on accumulated key.
    // Empty key disallowed as service paramter, and is used as a sentinel.

    if (c != '"')
    {
        consume_quoted(key_);
    }
    else if (key_.empty())
    {
        state_ = state::error_state;
    }
    else
    {
        state_ = state::parameter_value;
    }
}

TEMPLATE
void CLASS::handle_parameter_value(char c) NOEXCEPT
{
    // Initiated by trailing '"' [in handle_parameter_key] so yes ws skipping.
    // In state::value, at first ':', state changes based on current key.

    if (is_whitespace(c))
    {
        return;
    }
    else if (c != ':')
    {
        state_ = state::error_state;
    }
    else
    {
        state_ = state::parameter;
    }
}
} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
