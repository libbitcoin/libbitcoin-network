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

// request.jsonrpc assign
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::assign_version(version& to, view_t& from) NOEXCEPT
{
    to = to_version(from);
    from = {};
    const auto ok = (to != version::invalid);
    state_ = ok ? state::request_start : state::error_state;
    return ok;
}

// request.method assign
// ----------------------------------------------------------------------------

TEMPLATE
inline void CLASS::assign_string(string_t& to, view_t& from) NOEXCEPT
{
    state_ = state::request_start;
    to = string_t{ from };
    from = {};
}

// request.id assigns
// ----------------------------------------------------------------------------
// id_option is std::optional<std::variant<null_t, code_t, string_t>>

TEMPLATE
inline bool CLASS::assign_number(id_option& to, view_t& from) NOEXCEPT
{
    to.emplace(code_t{});
    auto& value = std::get<code_t>(to.value());
    const auto ok = to_signed(value, from);
    from = {};
    state_ = ok ? state::request_start : state::error_state;
    return ok;
}

TEMPLATE
inline void CLASS::assign_string(id_option& to, view_t& from) NOEXCEPT
{
    state_ = state::request_start;
    to.emplace(std::in_place_type<string_t>, from);
    from = {};
}

TEMPLATE
inline void CLASS::assign_null(id_option& to, view_t& from) NOEXCEPT
{
    state_ = state::request_start;
    to.emplace(null_t{});
    from = {};
}

// parameter types
// ----------------------------------------------------------------------------
// value_option is std::optional<value_t> where value_t.inner is of type:
// std::variant<null_t, number_t, string_t, boolean_t, array_t, object_t>

TEMPLATE
inline void CLASS::assign_array(value_t& to, view_t& from) NOEXCEPT
{
    // single (unnamed) string blob in vector<variant<string, ...>>.
    state_ = state::params_start;
    to.inner.emplace<array_t>();
    auto vector = std::get<array_t>(to.inner);
    vector.emplace_back(string_t{ from });
    from = {};
}

TEMPLATE
inline void CLASS::assign_object(value_t& to, view_t& from) NOEXCEPT
{
    // single named string blob in unordered_map<string, variant<string, ...>>.
    state_ = state::params_start;
    to.inner.emplace<object_t>();
    auto& map = std::get<object_t>(to.inner);
    map.emplace(std::make_pair(string_t{ "blob" }, value_t{ string_t{ from } }));
    from = {};
}

TEMPLATE
inline void CLASS::assign_string(value_t& to, view_t& from) NOEXCEPT
{
    state_ = state::params_start;
    to.inner.emplace<string_t>(from);
    from = {};
}

TEMPLATE
inline bool CLASS::assign_number(value_t& to, view_t& from) NOEXCEPT
{
    to.inner.emplace<number_t>();
    auto& value = std::get<number_t>(to.inner);
    const auto ok = to_number(value, from);
    from = {};
    state_ = ok ? state::params_start : state::error_state;
    return ok;
}

TEMPLATE
inline void CLASS::assign_true(value_t& to, view_t& from) NOEXCEPT
{
    state_ = state::params_start;
    to.inner.emplace<boolean_t>(true);
    from = {};
}

TEMPLATE
inline void CLASS::assign_false(value_t& to, view_t& from) NOEXCEPT
{
    state_ = state::params_start;
    to.inner.emplace<boolean_t>(false);
    from = {};
}

TEMPLATE
inline void CLASS::assign_null(value_t& to, view_t& from) NOEXCEPT
{
    state_ = state::params_start;
    to.inner.emplace<null_t>();
    from = {};
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
