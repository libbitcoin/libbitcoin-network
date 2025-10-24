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

#include <iterator>
#include <utility>
#include <variant>

namespace libbitcoin {
namespace network {
namespace json {

// containers
// ----------------------------------------------------------------------------
// static

TEMPLATE
inline const typename CLASS::request_it CLASS::add_request(
    batch_t& batch) NOEXCEPT
{
    batch.emplace_back();
    return std::prev(batch.end());
}

TEMPLATE
inline void CLASS::add_array(params_option& params) NOEXCEPT
{
    params.emplace(std::in_place_type<array_t>);
}

TEMPLATE
inline void CLASS::add_object(params_option& params) NOEXCEPT
{
    params.emplace(std::in_place_type<object_t>);
}

TEMPLATE
inline bool CLASS::is_array(const params_option& params) NOEXCEPT
{
    BC_ASSERT(params.has_value());
    return std::holds_alternative<array_t>(params.value());
}

TEMPLATE
inline bool CLASS::is_empty(const params_option& params) NOEXCEPT
{
    return is_array(params) ?
        std::get<array_t>(params.value()).empty() :
        std::get<object_t>(params.value()).empty();
}

// request.key comparison
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::is_contained(const keys_t& keys, view_t& key) NOEXCEPT
{
    const auto ok = unescape(unescaped_, key) && system::contains(keys, key);
    state_ = ok ? state::request_start : state::error_state;

    // This does not clear key_ as it is used in the next state.
    ////unescaped_.clear();
    ////key = {};
    return ok;
}

// request.jsonrpc version string assign
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::assign_version(version& to, view_t& value) NOEXCEPT
{
    state_ = state::error_state;
    if (!unescape(unescaped_, value))
        return false;

    to = version::invalid;
    if constexpr (require == version::any || require == version::v1)
    {
        if (value == "1.0" || value.empty())
            to = version::v1;
    }

    if constexpr (require == version::any || require == version::v2)
    {
        if (value == "2.0")
            to = version::v2;
    }

    const auto ok = (to != version::invalid);
    state_ = ok ? state::request_start : state::error_state;
    unescaped_.clear();
    value = {};
    return ok;
}

// request.method assign
// ----------------------------------------------------------------------------

TEMPLATE
inline void CLASS::assign_string(string_t& to, view_t& value) NOEXCEPT
{
    state_ = state::error_state;
    if (!unescape(unescaped_, value))
        return;

    state_ = state::request_start;
    to = string_t{ value };
    unescaped_.clear();
    value = {};
}

// request.id assigns
// ----------------------------------------------------------------------------
// id_option is std::optional<std::variant<null_t, code_t, string_t>>

TEMPLATE
inline bool CLASS::assign_number(id_option& to, view_t& value) NOEXCEPT
{
    to.emplace(std::in_place_type<code_t>);
    auto& number = std::get<code_t>(to.value());
    const auto ok = to_signed(number, value);
    state_ = ok ? state::request_start : state::error_state;
    after_ = true;
    value = {};
    return ok;
}

TEMPLATE
inline void CLASS::assign_string(id_option& to, view_t& value) NOEXCEPT
{
    state_ = state::error_state;
    if (!unescape(unescaped_, value))
        return;

    state_ = state::request_start;
    to.emplace(std::in_place_type<string_t>, value);
    after_ = true;
    unescaped_.clear();
    value = {};
}

TEMPLATE
inline void CLASS::assign_null(id_option& to, view_t& value) NOEXCEPT
{
    state_ = state::request_start;
    to.emplace(std::in_place_type<null_t>);
    after_ = true;
    value = {};
}

// parameter types
// ----------------------------------------------------------------------------

TEMPLATE
template <class Type, class... Value>
inline bool CLASS::push_param(params_option& to, const view_t& key,
    Value&&... value) NOEXCEPT
{
    if (is_array(to))
    {
        auto& array = std::get<array_t>(to.value());
        array.emplace_back(std::in_place_type<Type>,
            std::forward<Value>(value)...);
        return true;
    }
    else
    {
        auto& map = std::get<object_t>(to.value());
        const auto inserted = map.try_emplace(string_t{ key },
            std::in_place_type<Type>, std::forward<Value>(value)...).second;
        return inserted;
    }
}

TEMPLATE
inline bool CLASS::push_array(params_option& to, view_t& key,
    view_t& value) NOEXCEPT
{
    const auto ok
    {
        push_param<array_t>(to, key, array_t
        {
            // TODO: value is copied, could be moved.
            value_t
            {
                std::in_place_type<string_t>, value
            }
        })
    };
    state_ = ok ? state::params_start : state::error_state;
    after_ = true;
    value = {};
    key = {};
    return ok;
}

TEMPLATE
inline bool CLASS::push_object(params_option& to, view_t& key,
    view_t& value) NOEXCEPT
{
    const auto ok
    {
        push_param<object_t>(to, key, object_t
        {
            {
                // blob storage of object is unnamed in map.
                {},

                // TODO: value is copied, could be moved.
                value_t
                {
                    std::in_place_type<string_t>, value
                }
            }
        })
    };
    state_ = ok ? state::params_start : state::error_state;
    after_ = true;
    value = {};
    key = {};
    return ok;
}

TEMPLATE
inline bool CLASS::push_string(params_option& to, view_t& key,
    view_t& value) NOEXCEPT
{
    state_ = state::error_state;
    if (!unescape(unescaped_, key))
        return false;

    // Must copy key because unescaped_ buffer will be cleared.
    const string_t ununescaped_key{ key };
    if (!unescape(unescaped_, value))
        return false;

    const auto ok{ push_param<string_t>(to, ununescaped_key, value) };
    state_ = ok ? state::params_start : state::error_state;
    after_ = true;
    unescaped_.clear();
    value = {};
    key = {};
    return ok;
}

TEMPLATE
inline bool CLASS::push_number(params_option& to, view_t& key,
    view_t& value) NOEXCEPT
{
    number_t number{};
    const auto ok{ to_number(number, value) &&
        push_param<number_t>(to, key, number) };
    state_ = ok ? state::params_start : state::error_state;
    after_ = true;
    value = {};
    key = {};
    return ok;
}

TEMPLATE
inline bool CLASS::push_boolean(params_option& to, view_t& key,
    view_t& value) NOEXCEPT
{
    const auto truth{ value == "true" };
    const auto ok{ (truth || value == "false") &&
        push_param<boolean_t>(to, key, truth) };
    state_ = ok ? state::params_start : state::error_state;
    after_ = true;
    value = {};
    key = {};
    return ok;
}

TEMPLATE
inline bool CLASS::push_null(params_option& to, view_t& key,
    view_t& value) NOEXCEPT
{
    const auto ok{ (value == "null") && push_param<null_t>(to, key) };
    state_ = ok ? state::params_start : state::error_state;
    after_ = true;
    value = {};
    key = {};
    return ok;
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
