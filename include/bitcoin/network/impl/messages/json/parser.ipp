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

#include <charconv>
#include <iterator>
#include <optional>
#include <string_view>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {
    
// Utilities.
// ----------------------------------------------------------------------------

#define IF_REQUEST(expression) if constexpr (request) expression
#define IF_RESPONSE(expression) if constexpr (response) expression

// Return the fixed parser error code.
TEMPLATE
inline json::error_code CLASS::parse_error() NOEXCEPT
{
    namespace errc = boost::system::errc;
    return errc::make_error_code(errc::invalid_argument);
}

// Token is converted to number returned via out (when return is true).
TEMPLATE
inline bool CLASS::to_number(int64_t& out, view token) NOEXCEPT
{
    const auto end = std::next(token.data(), token.size());
    return is_zero(std::from_chars(token.data(), end, out).ec);
}

// Token consumes character c by adjusting its view over the buffer.
TEMPLATE
inline void CLASS::consume(view& token, const iterator& it) NOEXCEPT
{
    if (token.empty())
        token = { std::to_address(it), one };
    else
        token = { token.data(), add1(token.size()) };
}

// Clear state for new parse.
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::reset() NOEXCEPT
{
    state_ = {};
    depth_ = {};
    quoted_ = {};
    it_ = {};
    key_ = {};
    value_ = {};
    error_ = {};
    parsed_ = {};
}

// Extractors.
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::is_done() const NOEXCEPT
{
    return state_ == state::complete;
}

TEMPLATE
bool CLASS::has_error() const NOEXCEPT
{
    return !!get_error();
}

TEMPLATE
json::error_code CLASS::get_error() const NOEXCEPT
{
    if (state_ == state::error_state)
        return parse_error();

    return {};
}

TEMPLATE
std::optional<typename CLASS::parsed_t> CLASS::get_parsed() const NOEXCEPT
{
    if (is_done() && !has_error())
        return parsed_;

    return {};
}

// Invoke streaming parse of data.
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::write(std::string_view data, json::error_code& ec) NOEXCEPT
{
    for (it_ = data.begin(); it_ != data.end(); ++it_)
    {
        // Within parse_character(c), it_ always points to c in data.
        parse_character(*it_);

        // Terminal states.
        if (state_ == state::complete || state_ == state::error_state)
        {
            ++it_;
            break;
        }

        // Terminal v2 character....
        if (protocol_ == protocol::v2 && (*it_ == '\n'))
        {
            // ...if after closing brace.
            if (state_ != state::error_state && is_zero(depth_))
            {
                finalize();
                state_ = state::complete;
            }

            ++it_;
            break;
        }
    }

    // Enforce require content following parse.
    if (state_ == state::complete)
    {
        if (protocol_ == protocol::v2 && parsed_.jsonrpc.empty())
            state_ = state::error_state;

        if constexpr (request)
        {
            if (protocol_ == protocol::v1 &&
                std::holds_alternative<null_t>(parsed_.id))
                state_ = state::error_state;
        }
        else
        {
            // Exactly one of "result" or "error" in responses.
            if (parsed_.result.has_value() == parsed_.error.has_value())
                state_ = state::error_state;

            // Enforce required error fields if error is present.
            if (parsed_.error.has_value() && (is_zero(parsed_.error->code) ||
                parsed_.error->message.empty()))
                state_ = state::error_state;
        }
    }

    // Set ec outparam.
    ec.clear();
    if (state_ == state::error_state)
        ec = parse_error();

    // Set parsed character count for return.
    const auto consumed = std::distance(data.begin(), it_);
    return system::possible_narrow_and_sign_cast<size_t>(consumed);
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::finalize() NOEXCEPT
{
    // Nothing to do if value is also empty.
    if (value_.empty())
        return;

    // Assign value to request or error based on state.
    switch (state_)
    {
        // Independently handled.
        ////case state::jsonrpc:
        ////{
        ////    state_ = CLASS::state::object_start;
        ////    parsed_.jsonrpc = string_t{ value_ };
        ////    break;
        ////}
        case state::method:
        {
            state_ = state::object_start;
            IF_REQUEST(parsed_.method = string_t{ value_ });
            break;
        }
        case state::params:
        {
            state_ = state::object_start;
            IF_REQUEST(parsed_.params = { string_t{ value_ } });
            break;
        }
        case state::result:
        {
            state_ = state::object_start;
            IF_RESPONSE(parsed_.result = { string_t{ value_ } });
            break;
        }
        case state::error_message:
        {
            state_ = state::object_start;
            error_.message = string_t{ value_ };
            break;
        }
        case state::error_data:
        {
            state_ = state::object_start;
            error_.data = { string_t{ value_ } };
            break;
        }
        case state::id:
        {
            state_ = state::object_start;

            int64_t out{};
            if (to_number(out, value_))
            {
                parsed_.id = code_t{ out };
            }
            else if (value_ == "null")
            {
                parsed_.id = null_t{};
            }
            else
            {
                parsed_.id = string_t{ value_ };
            }

            break;
        }
        default:
        {
            state_ = state::error_state;
            break;
        }
    }

    // Value and key are now both empty.
    value_ = {};
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

// Visitors.
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::handle_initialize(char c) NOEXCEPT
{
    // Starting brace.
    if (c == '{')
    {
        state_ = state::object_start;
        ++depth_;
        return;
    }

    // JSON whitespace characters allowed before brace.
    if (c != ' ' && c != '\n' && c != '\r' && c != '\t')
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_object_start(char c) NOEXCEPT
{
    if (c == '"')
    {
        quoted_ = true;
        state_ = state::key;
    }
    else if (c == '}')
    {
        --depth_;
        if (is_zero(depth_))
            state_ = state::complete;
    }
}

TEMPLATE
void CLASS::handle_key(char c) NOEXCEPT
{
    if (!quoted_)
        return;

    if (c != '"')
    {
        consume(key_, it_);
        return;
    }

    quoted_ = false;

    // TODO:
    // Shift to key-based parsing inside errors by adding error-specific key handling,
    // or route to error-specific handlers [if key_ == "code", go to handle_error_code].
    if (key_ == "jsonrpc")
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
    else if (key_ == "id")
    {
        state_ = state::value;
    }
    else if (key_ == "result")
    {
        state_ = state::value;
    }
    else if (key_ == "error")
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
    else
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_value(char c) NOEXCEPT
{
    if (c != ':')
    {
        state_ = state::error_state;
        return;
    }

    // TODO:
    // Shift to key-based parsing inside errors by adding error-specific key handling,
    // or route to error-specific handlers [if key_ == "code", go to handle_error_code].
    if (key_ == "jsonrpc")
    {
        state_ = state::jsonrpc;
    }
    else if (key_ == "method")
    {
        state_ = state::method;
    }
    else if (key_ == "params")
    {
        state_ = state::params;
    }
    else if (key_ == "id")
    {
        state_ = state::id;
    }
    else if (key_ == "result")
    {
        state_ = state::result;
    }
    else if (key_ == "error")
    {
        state_ = state::error_start;
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
    else
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_jsonrpc(char c) NOEXCEPT
{
    if (c == '"')
    {
        quoted_ = !quoted_;
        if (!quoted_)
        {
            if ((protocol_ == protocol::v1 && value_ == "1.0") ||
                (protocol_ == protocol::v2 && value_ == "2.0"))
            {
                state_ = state::object_start;
                parsed_.jsonrpc = string_t{ value_ };
                value_ = {};
            }
            else
            {
                state_ = state::error_state;
            }
        }
    }
    else if (quoted_)
    {
        consume(value_, it_);
    }
}

TEMPLATE
void CLASS::handle_method(char c) NOEXCEPT
{
    if (c == '"')
    {
        quoted_ = !quoted_;
        if (!quoted_)
        {
            state_ = state::object_start;
            IF_REQUEST(parsed_.method = string_t{ value_ });
            value_ = {};
        }
    }
    else if (quoted_)
    {
        consume(value_, it_);
    }
}

TEMPLATE
void CLASS::handle_params(char c) NOEXCEPT
{
    if (c == '"')
    {
        quoted_ = !quoted_;
    }
    else if (c == '[')
    {
        ++depth_;
    }
    else if (c == ']')
    {
        --depth_;
    }
    else if (c == '{')
    {
        ++depth_;
    }
    else if (c == '}')
    {
        --depth_;
    }
    else if (c == ',')
    {
        if (is_one(depth_) && !quoted_)
        {
            state_ = state::object_start;
            IF_REQUEST(parsed_.params = { string_t{ value_ } });
            value_ = {};

            // Don't consume the comma.
            return;
        }
    }

    consume(value_, it_);
}

TEMPLATE
void CLASS::handle_id(char c) NOEXCEPT
{
    if (c == '"')
    {
        quoted_ = !quoted_;
        if (!quoted_)
        {
            int64_t out{};
            if (to_number(out, value_))
            {
                parsed_.id = out;
            }
            else if (value_ == "null")
            {
                parsed_.id = null_t{};
            }
            else
            {
                parsed_.id = string_t{ value_ };
            }

            state_ = state::object_start;
            value_ = {};
        }
    }
    else if (quoted_)
    {
        consume(value_, it_);
    }
    else if (c == 'n' && value_ == "nul")
    {
        consume(value_, it_);

        if (value_ == "null")
        {
            state_ = state::object_start;
            parsed_.id = null_t{};
            value_ = {};
        }
    }
    else if (std::isdigit(c) || c == '-')
    {
        consume(value_, it_);
    }
    else if (c == ',' && !quoted_ && is_one(depth_))
    {
        int64_t out{};
        if (to_number(out, value_))
            parsed_.id = out;

        state_ = state::object_start;
        value_ = {};
    }
}

TEMPLATE
void CLASS::handle_result(char c) NOEXCEPT
{
    if (c == '"')
    {
        quoted_ = !quoted_;
    }
    else if (c == '[')
    {
        ++depth_;
    }
    else if (c == ']')
    {
        --depth_;
    }
    else if (c == '{')
    {
        ++depth_;
    }
    else if (c == '}')
    {
        --depth_;
    }
    else if (c == ',')
    {
        if (is_one(depth_) && !quoted_)
        {
            state_ = state::object_start;
            IF_RESPONSE(parsed_.result = { string_t{ value_ } });
            value_ = {};

            // Don't consume the comma.
            return;
        }
    }

    consume(value_, it_);
}

TEMPLATE
void CLASS::handle_error_start(char c) NOEXCEPT
{
    if (c == '{')
    {
        state_ = state::object_start;
        ++depth_;
    }
    else if (c == 'n' && value_ == "nul")
    {
        consume(value_, it_);

        if (value_ == "null")
        {
            state_ = state::object_start;
            IF_RESPONSE(parsed_.error = {});
            value_ = {};
        }
    }
    else
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_error_code(char c) NOEXCEPT
{
    if (std::isdigit(c) || c == '-')
    {
        consume(value_, it_);
    }
    else if (c == ',' || c == '}')
    {
        int64_t out{};
        if (to_number(out, value_))
        {
            state_ = state::object_start;
            error_.code = out;
            value_ = {};
            if (c == '}')
                --depth_;
        }
        else
        {
            state_ = state::error_state;
        }
    }
}

TEMPLATE
void CLASS::handle_error_message(char c) NOEXCEPT
{
    if (c == '"')
    {
        quoted_ = !quoted_;
        if (!quoted_)
        {
            // Return to key parsing.
            state_ = state::object_start;
            error_.message = string_t{ value_ };
            value_ = {};
        }
    }
    else if (quoted_)
    {
        consume(value_, it_);
    }
    else if (c == ',' || c == '}')
    {
        state_ = state::object_start;
        if (c == '}')
            --depth_;
    }
}

TEMPLATE
void CLASS::handle_error_data(char c) NOEXCEPT
{
    if (c == '"')
    {
        quoted_ = !quoted_;
    }
    else if (c == '[')
    {
        ++depth_;
    }
    else if (c == ']')
    {
        --depth_;
    }
    else if (c == '{')
    {
        ++depth_;
    }
    else if (c == '}')
    {
        --depth_;
        if (is_one(depth_))
        {
            // Validate required fields before assignment.
            if (is_zero(error_.code) || error_.message.empty())
            {
                state_ = state::error_state;
                return;
            }

            // Assign error object to response.
            state_ = state::object_start;
            IF_RESPONSE(parsed_.error = error_);
            value_ = {};

            // Don't consume the closing brace.
            return;
        }
    }
    else if (c == ',')
    {
        if (is_one(depth_) && !quoted_)
        {
            state_ = state::object_start;
            error_.data = { string_t{ value_ } };
            value_ = {};

            // Don't consume the comma.
            return;
        }
    }

    consume(value_, it_);
}

#undef IF_REQUEST
#undef IF_RESPONSE

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
