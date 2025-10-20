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
#include <variant>

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
    return has_error() ? failure() : error_code{};
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
size_t CLASS::write(const std::string_view& data) NOEXCEPT
{
    for (char_ = data.begin(); char_ != data.end(); ++char_)
    {
        if (done_parsing(*char_))
        {
            ++char_;
            break;
        }
    }

    // Check object relationships if complete.
    validate();

    // Parse can successfully (or not) terminate before fully consuming data.
    return distance(data.begin(), char_);
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::done_parsing(char c) NOEXCEPT
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

    std::cout << "[" << *char_ << "]<" << key_ << ">=|" << value_ << "|" << std::endl;
    return is_done();
}

TEMPLATE
void CLASS::validate() NOEXCEPT
{
    // Validation is only relevant to a successful/complete parse.
    // Otherwise there is either an error or intermediate state.
    if (state_ != state::complete)
        return;

    // Unbatched requires a single request/response.
    if (batch_.empty())
        state_ = state::error_state;

    // This needs to be relaxed (!strict) for stratum_v1.
    if constexpr (strict && require == version::v2)
    {
        // Undefined version means the jsonrpc element was not encountered.
        if (parsed_->jsonrpc == version::undefined)
            state_ = state::error_state;
    }

    if constexpr (request)
    {
        // Non-null "id" required in version1.
        if (parsed_->jsonrpc == version::v1 &&
            std::holds_alternative<null_t>(parsed_->id))
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

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
