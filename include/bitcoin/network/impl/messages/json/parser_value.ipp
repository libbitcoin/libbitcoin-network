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
        ////request_->params.emplace(object_t{});
        request_->params.emplace(array_t{});
        state_ = state::parameter;
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

// parameter
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::handle_parameter_key(char c) NOEXCEPT
{
    // Initiated by opening '"' [in named handle_parameter] so no ws skipping.
    // In state::key, at trailing '"', state assigned based on accumulated key.

    if (c != '"')
    {
        consume_quoted(key_);
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
        return;
    }
    else
    {
        state_ = state::parameter;
    }
}

TEMPLATE
void CLASS::handle_parameter(char c) NOEXCEPT
{
    // TODO: utility to add new array/map element and return its value_t&.
    // TODO: make this a method, hiding the array/map distinction.
    // TODO: that will allow this method to work for both, only needing to also
    // TODO: pass the key name for object values. will need another method to
    // TODO: parse the arbitrary name key. setters can hide the application of
    // TODO: key_, its clearance, and even the selection of the arr/obj based
    // TODO: on the current request->params variant type. So we can pass in
    // TODO: request->params, just as we do for jsonrpc, method, and id.

    BC_ASSERT(request_->params.has_value());
    auto& parameters = request_->params.value();
    auto& arr = std::get<array_t>(parameters);
    ////auto& obj = std::get<object_t>(parameters);

    // Disallows empty params keys by using key_.empty() as a sentinel.
    const auto key_required = [this]() NOEXCEPT
    {
        return key_.empty() && std::holds_alternative<object_t>(
            request_->params.value());
    };

    // key (nvp only), as applicable, indicated by the first quote.
    if (c == '"' && key_required())
    {
        state_ = state::parameter_key;
    }

    // value (nvp or arr).

    // string value is terminated by its closing quote.
    else if (c == '"')
    {
        if (toggle(quoted_))
            assign_string(arr.at(42), value_);
    }
    else if (quoted_)
    {
        consume_quoted(value_);
    }

    else if (c == '{')
    {
        if (consume_object(value_))
            assign_object(arr.at(42), value_);
    }
    else if (c == '[')
    {
        if (consume_array(value_))
            assign_array(arr.at(42), value_);
    }

    else if (is_nully(value_, c))
    {
        if (consume_char(value_) == null_size)
            assign_null(arr.at(42), value_);
    }
    else if (is_truthy(value_, c))
    {
        if (consume_char(value_) == true_size)
            assign_true(arr.at(42), value_);
    }
    else if (is_falsy(value_, c))
    {
        if (consume_char(value_) == false_size)
            assign_false(arr.at(42), value_);
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
    else
    {
        // redispatch character that successfully terminated number.
        state_ = state::request_start;
        handle_request_start(c);
    }
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
