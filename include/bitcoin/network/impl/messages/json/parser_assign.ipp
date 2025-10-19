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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_ASSIGN_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_ASSIGN_IPP

#include <utility>

namespace libbitcoin {
namespace network {
namespace json {

// protected

// Assign value_ after assignment, since 'from' may be a reference to value_.

TEMPLATE
inline void CLASS::assign_error(error_option& to, const result_t& from) NOEXCEPT
{
    state_ = state::object_start;
    to = error_option{ from };
    value_ = {};
}

TEMPLATE
inline void CLASS::assign_error(error_option& to, result_t&& from) NOEXCEPT
{
    state_ = state::object_start;
    to = error_option{ std::move(from) };
    value_ = {};
}

TEMPLATE
inline void CLASS::assign_value(value_option& to, const value_t& from) NOEXCEPT
{
    state_ = state::object_start;
    to = value_option{ from };
    value_ = {};
}

TEMPLATE
inline void CLASS::assign_value(value_option& to, value_t&& from) NOEXCEPT
{
    state_ = state::object_start;
    to = value_option{ std::move(from) };
    value_ = {};
}

TEMPLATE
inline void CLASS::assign_string(string_t& to, const view_t& from) NOEXCEPT
{
    state_ = state::object_start;
    to = string_t{ from };
    value_ = {};
}

TEMPLATE
inline void CLASS::assign_string_id(id_t& to, const view_t& from) NOEXCEPT
{
    std::get<string_t>(to) = string_t{ from };
    value_ = {};
}

TEMPLATE
inline void CLASS::assign_numeric_id(code_t& to, const view_t& from) NOEXCEPT
{
    code_t number{};

    if (to_signed(number, from))
    {
        state_ = state::object_start;
        to = number;
    }
    else
    {
        state_ = state::error_state;
    }

    value_ = {};
}

TEMPLATE
inline void CLASS::assign_numeric_id(id_t& to, const view_t& from) NOEXCEPT
{
    assign_numeric_id(std::get<code_t>(to), from);
}

TEMPLATE
inline void CLASS::assign_null_id(id_t& to) NOEXCEPT
{
    state_ = state::object_start;
    std::get<null_t>(to) = null_t{};
    value_ = {};
}

TEMPLATE
inline void CLASS::assign_unquoted_id(id_t& to, const view_t& from) NOEXCEPT
{
    code_t number{};

    if (from == "null")
    {
        state_ = state::object_start;
        to = null_t{};
    }
    else if (to_signed(number, from))
    {
        state_ = state::object_start;
        to = number;
    }
    else
    {
        state_ = state::error_state;
    }

    value_ = {};
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
