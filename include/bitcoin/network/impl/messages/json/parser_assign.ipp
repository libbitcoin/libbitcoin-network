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

#include <algorithm>
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

// Consumption.
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::unescape(view_t& value) NOEXCEPT
{
    // Shortcircuit if no escapes, preserving first position.
    auto out = value.find('\\');
    if (out == view_t::npos)
        return true;

    // Over-size output string to avoid reallocations.
    unescaped_.resize(value.size());

    // Copy unescaped prefix.
    std::copy_n(value.begin(), out, unescaped_.begin());

    // Size of unicode escape.
    constexpr size_t hex = 4;
    auto in = out;

    // TODO: copy in chunks between escapes for efficiency.
    while (in < value.size())
    {
        if (value[in] != '\\')
        {
            unescaped_[out++] = value[in++];
            continue;
        }

        // Skip '\' and ensure at least an escape character.
        if (++in == value.size())
            return false;

        // Skip escape character and process.
        switch (value[in++])
        {
            // '/' is unique in that it must be unescaped but may be literal.
            case '/' : unescaped_[out++] = '/';  break;
            case '"' : unescaped_[out++] = '"';  break;
            case '\\': unescaped_[out++] = '\\'; break;
            case 'b' : unescaped_[out++] = '\b'; break;
            case 'f' : unescaped_[out++] = '\f'; break;
            case 'n' : unescaped_[out++] = '\n'; break;
            case 'r' : unescaped_[out++] = '\r'; break;
            case 't' : unescaped_[out++] = '\t'; break;
            case 'u' :
            {
                if (in + hex >= value.size())
                    return false;

                // TODO: use system.
                // Convert 4 input hex characters to integer.
                size_t point{};
                try
                {
                    size_t count{};
                    constexpr int base = 16;
                    const string_t snip{ value.substr(in, hex) };
                    point = std::stoul(snip, &count, base);
                    if (count != hex) return false;
                    in += hex;
                }
                catch (...)
                {
                    return false;
                }

                // TODO: use system.
                // Convert Unicode codepoint to UTF-8, maximum 3 output bytes.
                if (point <= 0x7f)
                {
                    unescaped_[out++] = (char)(point);
                }
                else if (point <= 0x7ff)
                {
                    unescaped_[out++] = (char)(0xc0 | ((point >> 6) & 0x1f));
                    unescaped_[out++] = (char)(0x80 | (point & 0x3f));
                }
                else if (point <= 0xffff)
                {
                    unescaped_[out++] = (char)(0xe0 | ((point >> 12) & 0x0f));
                    unescaped_[out++] = (char)(0x80 | ((point >> 6) & 0x3f));
                    unescaped_[out++] = (char)(0x80 | (point & 0x3f));
                }
                else
                {
                    return false;
                }

                break;
            }
            default: return false;
        }

        if (const auto next = value.find('\\', in); next == view_t::npos)
        {
            // Copy remaining unescaped section (end of value).
            const auto begin = std::next(value.begin(), in);
            const auto end   = value.end();
            const auto to    = std::next(unescaped_.data(), out);
            std::copy(begin, end, to);
            out += (value.size() - in);
            break;
        }
        else if (next > in)
        {
            // Copy unescaped section before escape (inside value).
            const auto begin = std::next(value.begin(), in);
            const auto end   = std::next(value.begin(), next);
            const auto to    = std::next(unescaped_.data(), out);
            std::copy(begin, end, to);
            out += next - in;
            in = next;
        }
    }

    // Caller should call unescaped_.clear() after assignment.
    // This allows the buffer to remain at its maximum extent until reset.
    // The view result of one unescape is not valid after another unescape.
    unescaped_.resize(out);
    value = view_t{ unescaped_ };
    return true;
}

// request.jsonrpc assign
// ----------------------------------------------------------------------------

TEMPLATE
inline bool CLASS::assign_version(version& to, view_t& value) NOEXCEPT
{
    state_ = state::error_state;
    if (!unescape(value))
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
    if (!unescape(value))
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
    if (!unescape(value))
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
    if (!unescape(value))
        return false;

    const auto ok{ push_param<string_t>(to, ununescaped_key, value) };
    state_ = ok ? state::params_start : state::error_state;
    after_ = true;
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
