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
#ifndef LIBBITCOIN_NETWORK_RPC_MODEL_HPP
#define LIBBITCOIN_NETWORK_RPC_MODEL_HPP

#include <optional>
#include <unordered_map>
#include <utility>
#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/rpc/any.hpp>
#include <bitcoin/network/rpc/enums/version.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

/// This is a document object model for extended json-rpc.
/// Extensions consist of int#_t, uint#_t, and any_t types.
/// any_t accepts shared_ptr<T> and exposes shared_ptr<T> via dispatcher<>.

/// Forward declaration for array_t/object_t. 
struct value_t;

struct null_t {};
using code_t = int64_t;
using boolean_t = bool;
using number_t = double;
using string_t = std::string;
using array_t = std::vector<value_t>;
using object_t = std::unordered_map<string_t, value_t>;
using any_t = rpc::any;

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
        /// json-rpc
        null_t,
        boolean_t,
        number_t,
        string_t,
        array_t,
        object_t,

        /// signed/unsigned integrals, not json deserializable.
        int8_t,
        int16_t,
        int32_t,
        int64_t,

        uint8_t,
        uint16_t,
        uint32_t,
        uint64_t,
        
        /// type-erased shared_ptr<Type>, not json deserializable.
        /// Pass ptr via any_t and specify it directly in the handler.
        any_t
    >;

    /// Explicit initialization constructors.
    value_t() NOEXCEPT : inner_{ null_t{} } {}
    value_t(null_t) NOEXCEPT : inner_{ null_t{} } {}
    value_t(boolean_t value) NOEXCEPT : inner_{ value } {}
    value_t(number_t value) NOEXCEPT : inner_{ value } {}
    value_t(const string_t& value) NOEXCEPT : inner_{ value } {}
    value_t(const array_t& value) NOEXCEPT : inner_{ value } {}
    value_t(const object_t& value) NOEXCEPT : inner_{ value } {}
    value_t(string_t&& value) NOEXCEPT : inner_{ std::move(value) } {}
    value_t(array_t&& value) NOEXCEPT : inner_{ std::move(value) } {}
    value_t(object_t&& value) NOEXCEPT : inner_{ std::move(value) } {}
    value_t(int8_t value) NOEXCEPT : inner_{ value } {}
    value_t(int16_t value) NOEXCEPT : inner_{ value } {}
    value_t(int32_t value) NOEXCEPT : inner_{ value } {}
    value_t(int64_t value) NOEXCEPT : inner_{ value } {}
    value_t(uint8_t value) NOEXCEPT : inner_{ value } {}
    value_t(uint16_t value) NOEXCEPT : inner_{ value } {}
    value_t(uint32_t value) NOEXCEPT : inner_{ value } {}
    value_t(uint64_t value) NOEXCEPT : inner_{ value } {}
    value_t(const any_t& value) NOEXCEPT : inner_{ value } {}
    value_t(any_t&& value) NOEXCEPT : inner_{ std::move(value) } {}

    /// Forwarding constructors for in-place variant construction.
    FORWARD_VARIANT_CONSTRUCT(value_t, inner_)
    FORWARD_VARIANT_ASSIGNMENT(value_t, inner_)
    ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, boolean_t, inner_)
    ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, number_t, inner_)
    ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, string_t, inner_)
    ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, array_t, inner_)
    ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, object_t, inner_)
    ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, int8_t, inner_)
    ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, int16_t, inner_)
    ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, int32_t, inner_)
    ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, int64_t, inner_)
    ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, uint8_t, inner_)
    ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, uint16_t, inner_)
    ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, uint32_t, inner_)
    ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, uint64_t, inner_)
    ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, any_t, inner_)
        
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
