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
#include <variant>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {
    
// Static.
// ----------------------------------------------------------------------------

TEMPLATE
inline json::error_code CLASS::parse_error() NOEXCEPT
{
    namespace errc = boost::system::errc;
    return errc::make_error_code(errc::invalid_argument);
}

TEMPLATE
bool CLASS::is_null(const id_t& id) NOEXCEPT
{
    return std::holds_alternative<null_t>(id);
}

TEMPLATE
bool CLASS::is_whitespace(char c) NOEXCEPT
{
    return (c == ' ' || c == '\n' || c == '\r' || c == '\t');
}

TEMPLATE
inline bool CLASS::to_number(int64_t& out, view token) NOEXCEPT
{
    const auto end = std::next(token.data(), token.size());
    return is_zero(std::from_chars(token.data(), end, out).ec);
}

TEMPLATE
inline id_t CLASS::to_id(view token) NOEXCEPT
{
    int64_t out{};
    if (to_number(out, token))
    {
        return out;
    }
    else if (token == "null")
    {
        return null_t{};
    }
    else
    {
        return string_t{ token };
    }
}

TEMPLATE
inline bool CLASS::increment(size_t& depth, state& status) NOEXCEPT
{
    if (is_zero(++depth))
    {
        status = state::error_state;
        return false;
    }

    return true;
}

TEMPLATE
inline bool CLASS::decrement(size_t& depth, state& status) NOEXCEPT
{
    if (is_zero(depth--))
    {
        status = state::error_state;
        return false;
    }

    return true;
}

TEMPLATE
inline void CLASS::consume(view& token, const char_iterator& at) NOEXCEPT
{
    // Token consumes character *at by incrementing its view over at's buffer.
    if (token.empty())
        token = { std::to_address(at), one };
    else
        token = { token.data(), add1(token.size()) };
}

TEMPLATE
inline size_t CLASS::distance(const char_iterator& from,
    const char_iterator& to) NOEXCEPT
{
    using namespace system;
    return possible_narrow_and_sign_cast<size_t>(std::distance(from, to));
}

// Versioning.
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::is_closed() const NOEXCEPT
{
    return is_zero(depth_) && state_ != state::error_state;
}

TEMPLATE
inline bool CLASS::is_terminal(char c) const NOEXCEPT
{
    return is_version2() && *char_ == '\n';
}

TEMPLATE
inline bool CLASS::is_version1() const NOEXCEPT
{
    return protocol_ == protocol::v1;
}

TEMPLATE
inline bool CLASS::is_version2() const NOEXCEPT
{
    return protocol_ == protocol::v2;
}

TEMPLATE
inline bool CLASS::is_version(view token) const NOEXCEPT
{
    return (is_version1() && token == "1.0")
        || (is_version2() && token == "2.0");
}

// Assignment.
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::assign_response(auto& to, const auto& from) NOEXCEPT
{
    if constexpr (response)
    {
        assign_value(to, from);
        return true;
    }
    else
    {
        state_ = state::error_state;
        return false;
    }
}

TEMPLATE
inline bool CLASS::assign_request(auto& to, const auto& from) NOEXCEPT
{
    if constexpr (request)
    {
        assign_value(to, from);
        return true;
    }
    else
    {
        state_ = state::error_state;
        return false;
    }
}

TEMPLATE
inline void CLASS::assign_value(auto& to, const auto& from) NOEXCEPT
{
    state_ = state::object_start;
    to = { from };
    value_ = {};
}

// Escaping.
// ----------------------------------------------------------------------------

TEMPLATE
inline void CLASS::consume_substitute(view& token, char /* c */) NOEXCEPT
{
    // BUGBUG: view is not modifiable, requires dynamic token (vs. view).
    consume(token, char_);
}

TEMPLATE
inline void CLASS::consume_escaped(view& token, char c) NOEXCEPT
{
    // BUGBUG: doesn't support \uXXXX, requires 4 character accumulation.
    switch (c)
    {
        case 'b':
            consume(token, '\b');
            return;
        case 'f':
            consume(token, '\f');
            return;
        case 'n':
            consume(token, '\n');
            return;
        case 'r':
            consume(token, '\r');
            return;
        case 't':
            consume(token, '\t');
            return;
        default:
            consume(token, char_);
    }
}

TEMPLATE
inline bool CLASS::consume_escape(view& token, char c) NOEXCEPT
{
    if (c == '\\' && !escaped_)
    {
        escaped_ = true;
        return true;
    }
    else if (escaped_)
    {
        consume_escaped(token, c);
        escaped_ = false;
        return true;
    }
    else
    {
        escaped_ = false;
        return false;
    }
}

// Clear state for new parse.
// ----------------------------------------------------------------------------

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
json::error_code CLASS::get_error() const NOEXCEPT
{
    return has_error() ? parse_error() : json::error_code{};
}

TEMPLATE
typename const CLASS::batch_t& CLASS::get_parsed() const NOEXCEPT
{
    if (is_done() && !has_error())
        return batch_;

    static const batch_t empty{};
    return empty;
}

// Invoke streaming parse of data.
// ----------------------------------------------------------------------------

TEMPLATE
size_t CLASS::write(std::string_view data, json::error_code& ec) NOEXCEPT
{
    for (auto char_ = data.begin(); char_ != data.end(); ++char_)
    {
        parse_character(*char_);

        if (is_terminal(*char_) && is_closed())
        {
            finalize();
            state_ = state::complete;
        }

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

    if constexpr (require_jsonrpc_v2)
    {
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
    // Nothing to do if value is also empty.
    if (value_.empty())
        return;

    // Assign value to request or error based on state.
    switch (state_)
    {
        // Error object.
        case state::error_message:
        {
            assign_value(error_.message, value_);
            break;
        }
        case state::error_data:
        {
            assign_value(error_.data, value_);
            break;
        }

        // Parsed object.
        case state::jsonrpc:
        {
            if (is_version(value_))
                assign_value(error_.jsonrpc, value_);
            else
                state_ = state::error_state;
        }
        case state::method:
        {
            assign_request(parsed_->method, value_);
            break;
        }
        case state::params:
        {
            assign_request(parsed_->params, value_);
            break;
        }
        case state::result:
        {
            assign_response(parsed_->result, value_);
            break;
        }
        case state::id:
        {
            assign_value(parsed_->id, to_id(value_));
            break;
        }

        // Invalid.
        default:
        {
            state_ = state::error_state;
            break;
        }
    }
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

// Visitors : State transitioners.
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::handle_initialize(char c) NOEXCEPT
{
    // Starting brace.
    if (c == '{')
    {
        batched_ = false;
        parsed_ = batch_.emplace_back();

        state_ = state::object_start;
        increment(depth_, state_);
        return;
    }
    else if (c == '[')
    {
        batched_ = true;

        state_ = state::object_start;
        increment(depth_, state_);
        return;
    }
    else if (!is_whitespace(c))
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
        if (!decrement(depth_, state_))
            return;

        if (is_zero(depth_))
            state_ = state::complete;
    }
    else if (batched_)
    {
        if (c == '{')
        {
            if (is_one(depth_))
            {
                parsed_ = batch_.emplace_back();
                increment(depth_, state_);
            }
            else
            {
                state_ = state::error_state;
            }
        }
        else if (c == ']')
        {
            if (is_one(depth_))
            {
                state_ = state::complete;
                decrement(depth_, state_);
            }
            else
            {
                state_ = state::error_state;
            }
        }
        else if (c == ',')
        {
            if (is_one(depth_))
                state_ = state::object_start;
            else
                state_ = state::error_state;
        }
        else if (!is_whitespace(c))
        {
            state_ = state::error_state;
        }
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_key(char c) NOEXCEPT
{
    if (!quoted_)
    {
        if (!is_whitespace(c))
            state_ = state::error_state;

        return;
    }

    if (consume_escape(key_, c))
        return;

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

// Visitors : Quoted value handlers.
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::handle_jsonrpc(char c) NOEXCEPT
{
    if (consume_escape(value_, c))
        return;

    if (c == '"')
    {
        quoted_ = !quoted_;
        if (!quoted_)
        {
            if (is_version(value_))
                assign_value(parsed_->jsonrpc, value_);
            else
                state_ = state::error_state;
        }
    }
    else if (quoted_)
    {
        consume(value_, char_);
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_method(char c) NOEXCEPT
{
    if (consume_escape(value_, c))
        return;

    if (c == '"')
    {
        quoted_ = !quoted_;
        if (!quoted_)
            assign_request(parsed_->method, value_);
    }
    else if (quoted_)
    {
        consume(value_, char_);
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_params(char c) NOEXCEPT
{
    if (consume_escape(value_, c))
        return;

    if (c == '"')
    {
        quoted_ = !quoted_;
    }
    else if (c == '[')
    {
        if (!increment(depth_, state_))
            return;
    }
    else if (c == ']')
    {
        if (!decrement(depth_, state_))
            return;
    }
    else if (c == '{')
    {
        if (!increment(depth_, state_))
            return;
    }
    else if (c == '}')
    {
        if (!decrement(depth_, state_))
            return;
    }
    else if (c == ',')
    {
        if (is_one(depth_) && !quoted_)
        {
            assign_request(parsed_->params, value_);
            return;
        }
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
        return;
    }

    consume(value_, char_);
}

TEMPLATE
void CLASS::handle_id(char c) NOEXCEPT
{
    if (consume_escape(value_, c))
        return;

    if (c == '"')
    {
        quoted_ = !quoted_;
        if (!quoted_)
            assign_value(parsed_->id, to_id(value_));
    }
    else if (quoted_)
    {
        consume(value_, char_);
    }
    else if (c == 'n' && value_ == "nul")
    {
        consume(value_, char_);
        if (value_ == "null")
            assign_value(parsed_->id, null_t{});
    }
    else if (std::isdigit(c) || c == '-')
    {
        consume(value_, char_);
    }
    else if (c == ',' && !quoted_ && is_one(depth_))
    {
        int64_t out{};
        if (to_number(out, value_))
        {
            assign_value(parsed_->id, out);
            if (c == '}')
                decrement(depth_, state_);
        }
        else
        {
            state_ = state::error_state;
        }
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_result(char c) NOEXCEPT
{
    if (consume_escape(value_, c))
        return;

    if (c == '"')
    {
        quoted_ = !quoted_;
    }
    else if (c == '[')
    {
        if (!increment(depth_, state_))
            return;
    }
    else if (c == ']')
    {
        if (!decrement(depth_, state_))
            return;
    }
    else if (c == '{')
    {
        if (!increment(depth_, state_))
            return;
    }
    else if (c == '}')
    {
        if (!decrement(depth_, state_))
            return;
    }
    else if (c == ',')
    {
        if (is_one(depth_) && !quoted_)
        {
            assign_response(parsed_->result, value_);
            return;
        }
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
        return;
    }

    consume(value_, char_);
}

TEMPLATE
void CLASS::handle_error_start(char c) NOEXCEPT
{
    if (c == '{')
    {
        state_ = state::object_start;
        increment(depth_, state_);
    }
    else if (c == 'n' && value_ == "nul")
    {
        consume(value_, char_);
        if (value_ == "null")
            assign_response(parsed_->error, result_t{});
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_error_code(char c) NOEXCEPT
{
    if (std::isdigit(c) || c == '-')
    {
        consume(value_, char_);
    }
    else if (c == ',' || c == '}')
    {
        int64_t out{};
        if (to_number(out, value_))
        {
            assign_value(error_.code, out);
            if (c == '}')
                decrement(depth_, state_);
        }
        else
        {
            state_ = state::error_state;
        }
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_error_message(char c) NOEXCEPT
{
    if (consume_escape(value_, c))
        return;

    if (c == '"')
    {
        quoted_ = !quoted_;
        if (!quoted_)
            assign_value(error_.message, value_);
    }
    else if (quoted_)
    {
        consume(value_, char_);
    }
    else if (c == ',' || c == '}')
    {
        state_ = state::object_start;
        if (c == '}')
            decrement(depth_, state_);
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
    }
}

TEMPLATE
void CLASS::handle_error_data(char c) NOEXCEPT
{
    if (consume_escape(value_, c))
        return;

    if (c == '"')
    {
        quoted_ = !quoted_;
    }
    else if (c == '[')
    {
       if (!increment(depth_, state_))
           return;
    }
    else if (c == ']')
    {
        if (!decrement(depth_, state_))
            return;
    }
    else if (c == '{')
    {
        if (!increment(depth_, state_))
            return;
    }
    else if (c == '}')
    {
        if (!decrement(depth_, state_))
            return;

        if (is_one(depth_))
        {
            if (is_zero(error_.code) || error_.message.empty())
            {
                state_ = state::error_state;
                return;
            }

            assign_response(parsed_->error, error_);
            return;
        }
    }
    else if (c == ',')
    {
        if (is_one(depth_) && !quoted_)
        {
            assign_value(error_.data, value_);
            return;
        }
    }
    else if (!is_whitespace(c))
    {
        state_ = state::error_state;
        return;
    }

    consume(value_, char_);
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
