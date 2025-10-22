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
void CLASS::handle_id(char c) NOEXCEPT
{
    // id_t : variant<null_t, code_t, string_t>

    // string is terminated by its closing quote.
    if (c == '"')
    {
        if (toggle(quoted_))
            assign_string(request_->id, value_);
    }
    else if (quoted_)
    {
        consume_quoted(value_);
    }

    // null is terminated by its 4th character.
    else if (is_nully(value_, c))
    {
        if (consume_char(value_) == null_size)
            assign_null(request_->id, value_);
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
    else if (!assign_number(request_->id, value_))
    {
        // empty string or other failures, state set by number parse.
        return;
    }
    else
    {
        // redispatch character that successfully terminated number.
        --char_;
        state_ = state::request_start;
    }
}

TEMPLATE
void CLASS::handle_params(char c) NOEXCEPT
{
    // params_t : optional<variant<array_t, object_t>>
    // There is only one params element, so no comma processing.

    if (c == '[')
    {
        after_ = false;
        add_array(request_->params);
        state_ = state::params_start;
    }
    else if (c == '{')
    {
        after_ = false;
        add_object(request_->params);
        state_ = state::params_start;
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_parameter(char c) NOEXCEPT
{
    // key_ is used for paramter{} but not parameter[].

    // string value is terminated by its closing quote.
    if (c == '"')
    {
        if (toggle(quoted_))
            push_string(request_->params, key_, value_);
    }
    else if (quoted_)
    {
        consume_quoted(value_);
    }

    else if (c == '{')
    {
        if (consume_object(value_))
            push_object(request_->params, key_, value_);
    }
    else if (c == '[')
    {
        if (consume_array(value_))
            push_array(request_->params, key_, value_);
    }

    else if (is_nully(value_, c))
    {
        if (consume_char(value_) == null_size)
            push_null(request_->params, key_, value_);
    }
    else if (is_truthy(value_, c))
    {
        if (consume_char(value_) == true_size)
            push_boolean(request_->params, key_, value_);
    }
    else if (is_falsy(value_, c))
    {
        if (consume_char(value_) == false_size)
            push_boolean(request_->params, key_, value_);
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
    else if (!push_number(request_->params, key_, value_))
    {
        // empty string or other failures, state set by number parse.
        return;
    }
    else
    {
        // redispatch character that successfully terminated number.
        --char_;
        state_ = state::params_start;
    }
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
