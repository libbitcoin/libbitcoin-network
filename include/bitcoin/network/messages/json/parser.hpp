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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_HPP

#include <charconv>
#include <iterator>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {

/// A no-copy parser for boost asio JSON-RPC v1/v2 stream parsing.
class BCT_API parser
{
public:
    /// Required for body<Parser> template.
    using buffer_t = string_t;

    /// Parser states.
    enum class state
    {
        initial,
        object_start,
        key,
        value,
        jsonrpc,
        method,
        params,
        id,
        result,
        error_start,
        error_code,
        error_message,
        error_data,
        complete,
        error_state
    };

    inline explicit parser(json::protocol proto) NOEXCEPT
      : protocol_{ proto }
    {
    }

    inline void reset() NOEXCEPT
    {
        state_ = {};
        depth_ = {};
        quoted_ = {};
        it_ = {};
        key_ = {};
        value_ = {};
        error_ = {};
        request_ = {};
        response_ = {};
    }

    inline bool is_done() const NOEXCEPT
    {
        return state_ == state::complete;
    }

    inline bool has_error() const NOEXCEPT
    {
        return !!get_error();
    }

    inline json::error_code get_error() const NOEXCEPT
    {
        if (state_ == state::error_state)
            return parse_error();

        return {};
    }

    inline std::optional<request_t> get_request() const NOEXCEPT
    {
        if (is_done() && !has_error())
            return request_;

        return {};
    }

    inline std::optional<response_t> get_response() const NOEXCEPT
    {
        if (is_done() && !has_error())
            return response_;

        return {};
    }

    // TODO: why is this processing request AND response.
    inline size_t write(std::string_view data, json::error_code& ec) NOEXCEPT
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

        ec.clear();
        if (state_ == state::error_state)
            ec = parse_error();

        const auto consumed = std::distance(data.begin(), it_);
        return system::possible_narrow_and_sign_cast<size_t>(consumed);
    }

protected:
    using view = std::string_view;
    using iterator = view::const_iterator;

    inline void finalize() NOEXCEPT
    {
        // Assign non-empty key (value should be empty).
        if (!key_.empty())
        {
            request_.jsonrpc = string_t{ key_ };
            key_ = {};
        }

        // Nothing to do if value is also empty.
        if (value_.empty())
            return;

        // Assign value to request or error based on state.
        switch (state_)
        {
            case state::jsonrpc:
            {
                state_ = parser::state::object_start;
                request_.jsonrpc = string_t{ value_ };
                response_.jsonrpc = string_t{ value_ };
                break;
            }
            case state::method:
            {
                state_ = state::object_start;
                request_.method = string_t{ value_ };
                break;
            }
            case state::params:
            {
                state_ = state::object_start;
                request_.params = { string_t{ value_ } };
                break;
            }
            case state::result:
            {
                state_ = state::object_start;
                response_.result = { string_t{ value_ } };
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
                    request_.id = code_t{ out };
                    response_.id = code_t{ out };
                }
                else if (value_ == "null")
                {
                    request_.id = null_t{};
                    response_.id = null_t{};
                }
                else
                {
                    request_.id = string_t{ value_ };
                    response_.id = string_t{ value_ };
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

    inline void parse_character(char c) NOEXCEPT
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

    inline void handle_initialize(char c) NOEXCEPT
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

    inline void handle_object_start(char c) NOEXCEPT
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

    inline void handle_key(char c) NOEXCEPT
    {
        if (c == '"' && quoted_)
        {
            quoted_ = false;
            if (key_ == "jsonrpc" || key_ == "method"  ||
                key_ == "params"  || key_ == "id"      ||
                key_ == "result"  || key_ == "error"   ||
                key_ == "code"    || key_ == "message" ||
                key_ == "data")
            {
                state_ = state::value;
            }
            else
            {
                state_ = state::error_state;
            }
        }
        else if (quoted_)
        {
            consume(key_, it_);
        }
    }

    inline void handle_value(char c) NOEXCEPT
    {
        if (key_ == "jsonrpc")
        {
            if (c == ':') state_ = state::jsonrpc;
        }
        else if (key_ == "method")
        {
            if (c == ':') state_ = state::method;
        }
        else if (key_ == "params")
        {
            if (c == ':') state_ = state::params;
        }
        else if (key_ == "id")
        {
            if (c == ':') state_ = state::id;
        }
        else if (key_ == "result")
        {
            if (c == ':') state_ = state::result;
        }
        else if (key_ == "error")
        {
            if (c == ':') state_ = state::error_start;
        }
        else if (key_ == "code")
        {
            if (c == ':') state_ = state::error_code;
        }
        else if (key_ == "message")
        {
            if (c == ':') state_ = state::error_message;
        }
        else if (key_ == "data")
        {
            if (c == ':') state_ = state::error_data;
        }
    }

    inline void handle_jsonrpc(char c) NOEXCEPT
    {
        if (c == '"')
        {
            quoted_ = !quoted_;
            if (!quoted_)
            {
                if ((protocol_ == protocol::v1 &&
                        (value_ == "1.0" || value_.empty())) ||
                    (protocol_ == protocol::v2 &&
                        (value_ == "2.0" || value_.empty())))
                {
                    state_ = state::object_start;
                    request_.jsonrpc = string_t{ value_ };
                    response_.jsonrpc = string_t{ value_ };
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

    inline void handle_method(char c) NOEXCEPT
    {
        if (c == '"')
        {
            quoted_ = !quoted_;
            if (!quoted_)
            {
                state_ = state::object_start;
                request_.method = string_t{ value_ };
                value_ = {};
            }
        }
        else if (quoted_)
        {
            consume(value_, it_);
        }
    }

    inline void handle_params(char c) NOEXCEPT
    {
        if (c == '"')
        {
            quoted_ = !quoted_;
        }
        else if (c == '{')
        {
            ++depth_;
        }
        else if (c == '}')
        {
            --depth_;
        }
        else if (c == '[')
        {
            ++depth_;
        }
        else if (c == ']')
        {
            --depth_;
        }
        else if (c == ',' && is_one(depth_) && !quoted_)
        {
            state_ = state::object_start;
            request_.params = { string_t{ value_ } };
            value_ = {};
        }

        consume(value_, it_);
    }

    inline void handle_id(char c) NOEXCEPT
    {
        if (c == '"')
        {
            quoted_ = !quoted_;
            if (!quoted_)
            {
                int64_t out{};
                if (to_number(out, value_))
                {
                    request_.id = out;
                    response_.id = out;
                }
                else if (value_ == "null")
                {
                    request_.id = null_t{};
                    response_.id = null_t{};
                }
                else
                {
                    request_.id = string_t{ value_ };
                    response_.id = string_t{ value_ };
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
                request_.id = null_t{};
                response_.id = null_t{};
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
            {
                request_.id = out;
                response_.id = out;
            }

            state_ = state::object_start;
            value_ = {};
        }
    }

    inline void handle_result(char c) NOEXCEPT
    {
        if (c == '"')
        {
            quoted_ = !quoted_;
        }
        else if (c == '{')
        {
            ++depth_;
        }
        else if (c == '}')
        {
            --depth_;
        }
        else if (c == '[')
        {
            ++depth_;
        }
        else if (c == ']')
        {
            --depth_;
        }
        else if (c == ',' && is_one(depth_) && !quoted_)
        {
            state_ = state::object_start;
            response_.result = { string_t{ value_ } };
            value_ = {};
        }

        consume(value_, it_);
    }

    inline void handle_error_start(char c) NOEXCEPT
    {
        if (c == '[')
        {
            state_ = state::error_code;
            ++depth_;
        }
        else if (c == '{')
        {
            state_ = state::error_code;
            ++depth_;
        }
        else if (c == 'n' && value_ == "nul")
        {
            consume(value_, it_);

            if (value_ == "null")
            {
                state_ = state::object_start;
                response_.error = {};
                value_ = {};
            }
        }
        else
        {
            state_ = state::error_state;
        }
    }

    inline void handle_error_code(char c) NOEXCEPT
    {
        if (std::isdigit(c) || c == '-')
        {
            consume(value_, it_);
        }
        else if (c == ',' && !quoted_)
        {
            int64_t out{};
            if (to_number(out, value_))
            {
                state_ = state::error_message;
                error_.code = out;
                value_ = {};
            }
            else
            {
                state_ = state::error_state;
            }
        }
    }

    inline void handle_error_message(char c) NOEXCEPT
    {
        if (c == '"')
        {
            quoted_ = !quoted_;
            if (!quoted_)
            {
                state_ = state::error_data;
                error_.message = string_t{ value_ };
                value_ = {};
            }
        }
        else if (quoted_)
        {
            consume(value_, it_);
        }
    }

    inline void handle_error_data(char c) NOEXCEPT
    {
        if (c == '"')
        {
            quoted_ = !quoted_;
        }
        else if (c == '{')
        {
            ++depth_;
        }
        else if (c == '}')
        {
            --depth_;
        }
        else if (c == '[')
        {
            ++depth_;
        }
        else if (c == ']')
        {
            --depth_;
        }
        else if (c == ',' && is_one(depth_) && !quoted_)
        {
            state_ = state::object_start;
            error_.data = { string_t{ value_ } };
            value_ = {};
        }
        else if (c == '}' && is_one(depth_))
        {
            // Assign error object to response.
            state_ = state::object_start;
            response_.error = error_;
        }

        consume(value_, it_);
    }

private:
    // Return the fixed parser error code.
    static inline json::error_code parse_error() NOEXCEPT
    {
        namespace errc = boost::system::errc;
        return errc::make_error_code(errc::invalid_argument);
    }

    // Token is converted to number in out, if true.
    static inline bool to_number(int64_t& out, view token) NOEXCEPT
    {
        const auto end = std::next(token.data(), token.size());
        return is_zero(std::from_chars(token.data(), end, out).ec);
    }

    // Token consumes character c by adjusting its view over the buffer.
    static inline void consume(view& token, const iterator& it) NOEXCEPT
    {
        if (token.empty())
            token = { std::to_address(it), one };
        else
            token = { token.data(), add1(token.size()) };
    }

////private:
protected:
    bool quoted_{};
    state state_{};
    size_t depth_{};

    iterator it_{};
    view key_{};
    view value_{};

    result_t error_{};
    request_t request_{};
    response_t response_{};

    const json::protocol protocol_;
};

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
