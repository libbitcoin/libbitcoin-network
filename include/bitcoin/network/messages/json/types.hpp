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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_TYPES_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_TYPES_HPP

#include <optional>
#include <unordered_map>
#include <utility>
#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/messages/json/enums/version.hpp>

namespace libbitcoin {
namespace network {
namespace json {

/// Forward declaration for array_t/object_t. 
struct value_t;

struct BCT_API null_t {};
using code_t = int64_t;
using boolean_t = bool;
using number_t = double;
using string_t = std::string;
using array_t = std::vector<value_t>;
using object_t = std::unordered_map<string_t, value_t>;

// linux and macos define id_t in the global namespace.
// typedef __darwin_id_t id_t;
// typedef __id_t id_t;
using identity_t = std::variant
<
    null_t,
    code_t,
    string_t
>;
using id_option = std::optional<identity_t>;

struct BCT_API value_t
{
    using type = std::variant
    <
        null_t,
        boolean_t,
        number_t,
        string_t,
        array_t,
        object_t
    >;

    type inner{};

    /// Forwarding constructors for in-place variant construction.

    template <class Type, class... Args>
    constexpr value_t(std::in_place_type_t<Type>, Args&&... args) NOEXCEPT
      : inner(std::in_place_type<Type>, std::forward<Args>(args)...)
    {
    }

    template <size_t Index, class... Args>
    constexpr value_t(std::in_place_index_t<Index>, Args&&... args) NOEXCEPT
      : inner(std::in_place_index<Index>, std::forward<Args>(args)...)
    {
    }
};
using value_option = std::optional<value_t>;

using params_t = std::variant
<
    array_t,
    object_t
>;
using params_option = std::optional<params_t>;

struct BCT_API result_t
{
    code_t code{};
    string_t message{};
    value_option data{};
};
using error_option = std::optional<result_t>;

struct BCT_API response_t
{
    version jsonrpc{ version::undefined };
    id_option id{};
    error_option error{};
    value_option result{};
};

struct BCT_API request_t
{
    version jsonrpc{ version::undefined };
    id_option id{};
    string_t method{};
    params_option params{};
};

using error_code = error::boost_code;

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
