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

TEMPLATE
inline void CLASS::assign_error(error_option& to, result_t& from) NOEXCEPT
{
    state_ = state::object_start;
    to = error_option{ from };
    from = {};
}

TEMPLATE
inline void CLASS::assign_value(value_option& to, view_t& from) NOEXCEPT
{
    state_ = state::object_start;
    to = value_option{ value_t{ string_t{ from } } };
    from = {};
}

TEMPLATE
inline void CLASS::assign_string(string_t& to, view_t& from) NOEXCEPT
{
    state_ = state::object_start;
    to = string_t{ from };
    from = {};
}

TEMPLATE
inline void CLASS::assign_version(version& to, view_t& from) NOEXCEPT
{
    state_ = state::object_start;
    to = to_version(from);
    if (to == version::invalid)
        state_ = state::error_state;

    from = {};
}

// id types

TEMPLATE
inline void CLASS::assign_string_id(id_t& to, view_t& from) NOEXCEPT
{
    std::get<string_t>(to) = string_t{ from };
    from = {};
}

TEMPLATE
inline void CLASS::assign_numeric_id(code_t& to, view_t& from) NOEXCEPT
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

    from = {};
}

TEMPLATE
inline void CLASS::assign_numeric_id(id_t& to, view_t& from) NOEXCEPT
{
    assign_numeric_id(std::get<code_t>(to), from);
}

TEMPLATE
inline void CLASS::assign_null_id(id_t& to, view_t& from) NOEXCEPT
{
    state_ = state::object_start;
    std::get<null_t>(to) = null_t{};
    from = {};
}

TEMPLATE
inline void CLASS::assign_unquoted_id(id_t& to, view_t& from) NOEXCEPT
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

    from = {};
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
