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

// private

TEMPLATE
json::version CLASS::to_version(const view_t& token) NOEXCEPT
{
    if constexpr (require == version::any || require == version::v1)
    {
        if (token == "1.0" || token.empty())
            return version::v1;
    }

    if constexpr (require == version::any || require == version::v2)
    {
        if (token == "2.0")
            return version::v2;
    }

    return version::invalid;
}

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
inline bool CLASS::assign_version(version& to, view_t& from) NOEXCEPT
{
    state_ = state::object_start;
    to = to_version(from);
    if (to == version::invalid)
        state_ = state::error_state;

    from = {};
    return state_ == state::object_start;
}

// id types

TEMPLATE
inline void CLASS::assign_string_id(id_t& to, view_t& from) NOEXCEPT
{
    state_ = state::object_start;
    to.emplace<string_t>(from);
    from = {};
}

TEMPLATE
inline bool CLASS::assign_numeric_id(code_t& to, view_t& from) NOEXCEPT
{
    state_ = state::error_state;
    if (to_signed(to, from))
        state_ = state::object_start;

    from = {};
    return state_ == state::object_start;
}

TEMPLATE
inline bool CLASS::assign_numeric_id(id_t& to, view_t& from) NOEXCEPT
{
    to.emplace<code_t>();
    return assign_numeric_id(std::get<code_t>(to), from);
}

TEMPLATE
inline bool CLASS::assign_unquoted_id(id_t& to, view_t& from) NOEXCEPT
{
    if (from == "null")
    {
        assign_null_id(to, from);
        return true;
    }

    return assign_numeric_id(to, from);
}

TEMPLATE
inline void CLASS::assign_null_id(id_t& to, view_t& from) NOEXCEPT
{
    state_ = state::object_start;
    to.emplace<null_t>();
    from = {};
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
