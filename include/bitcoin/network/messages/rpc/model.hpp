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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_MODEL_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_MODEL_HPP

#include <optional>
#include <unordered_map>
#include <utility>
#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/enums/version.hpp>
#include <bitcoin/network/messages/peer/peer.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

/// Forward declaration for array_t/object_t. 
struct value_t;

struct null_t {};
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

struct value_t
{
    using inner_t = std::variant
    <
        null_t,
        boolean_t,
        number_t,
        string_t,
        array_t,
        object_t,
        messages::peer::ping::cptr
    >;

    /// Explicit initialization constructors.
    value_t(null_t) NOEXCEPT : inner_{ null_t{} } {}
    value_t(boolean_t value) NOEXCEPT : inner_{ value } {}
    value_t(number_t value) NOEXCEPT : inner_{ value } {}
    value_t(string_t value) NOEXCEPT : inner_{ std::move(value) } {}
    value_t(array_t value) NOEXCEPT : inner_{ std::move(value) } {}
    value_t(object_t value) NOEXCEPT : inner_{ std::move(value) } {}
    value_t(messages::peer::ping::cptr value) NOEXCEPT : inner_{ std::move(value) } {}

    /// Forwarding constructors for in-place variant construction.
    FORWARD_VARIANT_CONSTRUCT(value_t, inner_)
    FORWARD_VARIANT_ASSIGNMENT(value_t, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, boolean_t, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, number_t, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, string_t, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, array_t, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, object_t, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, messages::peer::ping::cptr, inner_)
        
    inner_t& value() NOEXCEPT
    {
        return inner_;
    }

    const inner_t& value() const NOEXCEPT
    {
        return inner_;
    }

private:
    inner_t inner_;
};
using value_option = std::optional<value_t>;

using params_t = std::variant
<
    array_t,
    object_t
>;
using params_option = std::optional<params_t>;

struct result_t
{
    code_t code{};
    string_t message{};
    value_option data{};
};
using error_option = std::optional<result_t>;

struct response_t
{
    version jsonrpc{ version::undefined };
    id_option id{};
    error_option error{};
    value_option result{};
};

struct request_t
{
    version jsonrpc{ version::undefined };
    id_option id{};
    string_t method{};
    params_option params{};
};

DECLARE_JSON_TAG_INVOKE(version);
DECLARE_JSON_TAG_INVOKE(value_t);
DECLARE_JSON_TAG_INVOKE(identity_t);
DECLARE_JSON_TAG_INVOKE(request_t);
DECLARE_JSON_TAG_INVOKE(response_t);

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
