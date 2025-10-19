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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_IPP

#include <string_view>

namespace libbitcoin {
namespace network {
namespace json {

// Properties.
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::has_error() const NOEXCEPT
{
    return state_ == state::error_state;
}

TEMPLATE
bool CLASS::is_done() const NOEXCEPT
{
    return state_ == state::complete || has_error();
}

TEMPLATE
error_code CLASS::get_error() const NOEXCEPT
{
    return has_error() ? parse_error() : error_code{};
}

TEMPLATE
const typename CLASS::batch_t& CLASS::get_parsed() const NOEXCEPT
{
    if (is_done() && !has_error())
        return batch_;

    static const batch_t empty{};
    return empty;
}

// Methods.
// ----------------------------------------------------------------------------

// private
TEMPLATE
const typename CLASS::parse_it CLASS::add_remote_procedure_call() NOEXCEPT
{
    batch_.emplace_back();
    return std::prev(batch_.end());
}

TEMPLATE
void CLASS::reset() NOEXCEPT
{
    batched_ = {};
    escaped_ = {};
    quoted_ = {};
    state_ = {};
    depth_ = {};
    char_ = {};
    key_ = {};
    value_ = {};
    batch_ = {};
    error_ = {};
    parsed_ = {};
}

TEMPLATE
size_t CLASS::write(const std::string_view& data, error_code& ec) NOEXCEPT
{
    for (char_ = data.begin(); char_ != data.end(); ++char_)
    {
        parse_character(*char_);

        // Capture the last object before the logical close.
        if (is_closed())
            finalize();

        // Can terminate before fully consuming data.
        if (is_done())
        {
            ++char_;
            break;
        }
    }

    validate();
    ec = get_error();
    return distance(data.begin(), char_);
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::validate() NOEXCEPT
{
    if (state_ != state::complete)
        return;

    // Unbatched requires a single element, empty implies error.
    if (batch_.empty())
        state_ = state::error_state;

    if constexpr (require_jsonrpc_element_in_version2)
    {
        // This needs to be relaxed for stratum_v1.
        if (is_version2() && parsed_->jsonrpc.empty())
            state_ = state::error_state;
    }

    if constexpr (request)
    {
        // Non-null "id" required in version1.
        if (is_version1() && is_null(parsed_->id))
            state_ = state::error_state;
    }
    else
    {
        // Exactly one of "result" or "error" allowed in responses.
        if (parsed_->result.has_value() == parsed_->error.has_value())
            state_ = state::error_state;

        // Enforce required error fields if error is present.
        if (parsed_->error.has_value() && (is_zero(parsed_->error->code) ||
            parsed_->error->message.empty()))
            state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::finalize() NOEXCEPT
{
    // Assign value to request or error based on state.
    switch (state_)
    {
        // Complete parsed object.
        case state::jsonrpc:
        {
            if (is_version(value_))
                assign_string(parsed_->jsonrpc, value_);
            else
                state_ = state::error_state;
        }
        case state::method:
        {
            ASSIGN_REQUEST(string, parsed_->method, value_)
            break;
        }
        case state::params:
        {
            ASSIGN_REQUEST(value, parsed_->params, value_)
            break;
        }
        case state::result:
        {
            ASSIGN_RESPONSE(value, parsed_->result, value_)
            break;
        }
        case state::id:
        {
            // Quoted ids cannot land here, always assigned by handle_id.
            assign_unquoted_id(parsed_->id, value_);
            break;
        }

        // Complete error object.
        case state::error_message:
        {
            assign_string(error_.message, value_);
            break;
        }
        case state::error_data:
        {
            assign_value(error_.data, value_);
            break;
        }

        // Invalid state.
        case state::initial:
        case state::object_start:
        case state::key:
        case state::value:
        case state::error_start:
        case state::error_code:
        case state::error_state:
        case state::complete:
        {
            state_ = state::error_state;
        }
    }

    // Close out state following an assignment above.
    if (state_ == state::object_start)
        state_ = state::complete;
}

TEMPLATE
void CLASS::parse_character(char c) NOEXCEPT
{
    switch (state_)
    {
        case state::initial:
            handle_initialize(c);
            break;
        case state::object_start:
            handle_object_start(c);
            break;
        case state::key:
            handle_key(c);
            break;
        case state::value:
            handle_value(c);
            break;
        case state::jsonrpc:
            handle_jsonrpc(c);
            break;
        case state::method:
            handle_method(c);
            break;
        case state::params:
            handle_params(c);
            break;
        case state::id:
            handle_id(c);
            break;
        case state::result:
            handle_result(c);
            break;
        case state::error_start:
            handle_error_start(c);
            break;
        case state::error_code:
            handle_error_code(c);
            break;
        case state::error_message:
            handle_error_message(c);
            break;
        case state::error_data:
            handle_error_data(c);
            break;
        case state::complete:
            break;
        case state::error_state:
            break;
    }
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
