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
        state_ = consume_text(key_) &&
            (key_ == "jsonrpc" || key_ == "id" ||
             key_ == "method"  || key_ == "params") ?
            state::value : state::error_state;
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
void CLASS::handle_value(char c) NOEXCEPT
{
    if (is_whitespace(c))
    {
        return;
    }
    else if (c != ':')
    {
        state_ = state::error_state;
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
    const auto array = is_array(request_->params);
    const auto empty = is_empty(request_->params);

    if (c == ',' && after_)
    {
        after_ = false;
        state_ = state::params_start;
    }
    else if (c == '"' && !array)
    {
        state_ = consume_text(key_) && !key_.empty() ?
            state::parameter_value : state::error_state;
    }
    else if (c == '"' && array)
    {
        redispatch(state::parameter);
    }
    else if (c == '}' && !array && (empty || after_))
    {
        state_ = state::request_start;
    }
    else if (c == ']' && array && (empty || after_))
    {
        state_ = state::request_start;
    }
    else if (!is_whitespace(c) && array)
    {
        // first character of array unquoted value.
        redispatch(state::parameter);
    }
    else
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_parameter_value(char c) NOEXCEPT
{
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
