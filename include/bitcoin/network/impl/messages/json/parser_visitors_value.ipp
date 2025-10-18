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

// protected

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
