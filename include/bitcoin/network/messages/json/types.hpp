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
using id_t = std::variant<null_t, code_t, string_t>;

struct BCT_API value_t
{
    using type = std::variant
    <
        null_t,
        number_t,
        string_t,
        boolean_t,
        array_t,
        object_t
    >;

    /// First type initialized by default (null_t).
    type value{};
};
using value_option = std::optional<value_t>;

struct BCT_API result_t
{
    code_t code{};
    string_t message{};
    value_option data{};
};
using error_option = std::optional<result_t>;

struct BCT_API request_t
{
    version jsonrpc{ version::undefined };
    string_t method{};
    value_option params{};
    id_t id{};

    bool result{};
    bool error{};
};

struct BCT_API response_t
{
    version jsonrpc{ version::undefined };
    value_option result{};
    error_option error{};
    id_t id{};

    bool method{};
    bool params{};
};

using error_code = error::boost_code;

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
