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

    // redispatch character that successfully terminated number.
    else
    {
        state_ = state::request_start;
        handle_request_start(c);
    }
}

TEMPLATE
void CLASS::handle_params(char c) NOEXCEPT
{
    // params_t : optional<variant<array_t, object_t>>

    if (c == '[')
    {
        request_->params.emplace(array_t{});
        state_ = state::parameter;
    }
    else if (c == '{')
    {
        request_->params.emplace(object_t{});
        state_ = state::parameter;
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

// parameter (collection of objects, not a value)
// ----------------------------------------------------------------------------
// if expected_ is ']' then key_ should already be empty.
// if expected_ is '}' then key_ holds name for nvp (must use and clear).
// reset open_[to_int(parameters_)] and parameters_ at either closure.

TEMPLATE
void CLASS::handle_parameter(char c) NOEXCEPT
{
    BC_ASSERT(request_->params.has_value());

    auto& parameters = request_->params.value();
    ////const auto name = std::holds_alternative<object_t>(parameters);
    auto& obj = std::get<object_t>(parameters);
    ////const auto anon = std::holds_alternative<array_t>(parameters);
    auto& arr = std::get<array_t>(parameters);

    // TODO: utility to add new array/map element and return its value_t&.
    // TODO: make this a method, hiding the array/map distinction.
    // TODO: that will allow this method to work for both, only needing to also
    // TODO: pass the key name for object values. will need another method to
    // TODO: parse the arbitrary name key. setters can hide the application of
    // TODO: key_, its clearance, and even the selection of the arr/obj based
    // TODO: on the current request->params variant type. So we can pass in
    // TODO: request->params, just as we do for jsonrpc, method, and id.

    // string is terminated by its closing quote.
    if (c == '"')
    {
        if (toggle(quoted_))
            assign_string(arr.at(0), value_);
    }
    else if (quoted_)
    {
        consume_quoted(value_);
    }

    // TODO: blobs... light parse from c to terminator, handle \\ and \" in "".
    // TODO: apply to value_ view, and assign obj|arr value to current element.
    else if (c == '{')
    {
    }
    else if (c == '[')
    {
    }

    // null is terminated by its 4th character.
    else if (is_nully(value_, c))
    {
        if (consume_char(value_) == null_size)
            assign_null(obj.at("key..."), value_);
    }

    // true is terminated by its 4th character.
    else if (is_truthy(value_, c))
    {
        if (consume_char(value_) == true_size)
            assign_null(obj.at("key..."), value_);
    }

    // false is terminated by its 5th character.
    else if (is_falsy(value_, c))
    {
        if (consume_char(value_) == false_size)
            assign_null(obj.at("key..."), value_);
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
    else if (!assign_number(arr.at(42), value_))
    {
        // empty string or other failures, state set by number parse.
        return;
    }

    // redispatch character that successfully terminated number.
    else
    {
        state_ = state::request_start;
        handle_request_start(c);
    }
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
