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

namespace libbitcoin {
namespace network {
namespace json {

// protected

TEMPLATE
inline void CLASS::assign_unquoted_id(auto& to, const auto& from) NOEXCEPT
{
    code_t number{};
    if (to_signed(number, from))
    {
        state_ = state::object_start;
        to = number;
    }
    else if (from == "null")
    {
        state_ = state::object_start;
        to = null_t{};
    }
    else
    {
        state_ = state::error_state;
    }

    // Assign last, since 'from' may be a referece to value.
    value_ = {};
}

TEMPLATE
inline void CLASS::assign_numeric_id(auto& to, const auto& from) NOEXCEPT
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

    // Assign last, since 'from' may be a referece to value.
    value_ = {};
}

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
    to = from;

    // Assign last, since 'from' may be a referece to value.
    value_ = {};
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
