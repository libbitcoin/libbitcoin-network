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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_RPC_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_RPC_HPP

#include <concepts>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/enums/version.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

/// Types.
/// ---------------------------------------------------------------------------

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
        object_t
    >;

    /// Explicit initialization constructors.
    value_t(null_t) NOEXCEPT : inner_{ null_t{} } {}
    value_t(boolean_t value) NOEXCEPT : inner_{ value } {}
    value_t(number_t value) NOEXCEPT : inner_{ value } {}
    value_t(string_t value) NOEXCEPT : inner_{ std::move(value) } {}
    value_t(array_t value) NOEXCEPT : inner_{ std::move(value) } {}
    value_t(object_t value) NOEXCEPT : inner_{ std::move(value) } {}

    /// Forwarding constructors for in-place variant construction.
    FORWARD_VARIANT_CONSTRUCT(value_t, inner_)
    FORWARD_VARIANT_ASSIGNMENT(value_t, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, boolean_t, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, number_t, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, string_t, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, array_t, inner_)
    FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_t, object_t, inner_)
        
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

/// Methods.
/// ---------------------------------------------------------------------------

enum class group { positional, named, either };

BC_PUSH_WARNING(NO_ARRAY_TO_POINTER_DECAY)

/// Defines methods assignable to an rpc interface.
template <text_t Text, typename ...Args>
struct method
{
    // TODO: std::string_view/std::string switch with extractor change (gcc14).
    static constexpr std::string_view name{ Text.text.data(), Text.text.size() };

    // TODO: std::string_view/std::string switch with extractor change (gcc14).
    static constexpr auto size = sizeof...(Args);
    using names = std::array<std::string_view, size>;
    using args = std::tuple<Args...>;
    using tag = method;

    /// Required for construction of tag{}.
    inline constexpr method() NOEXCEPT
      : names_{}
    {
    }

    /// Defines a method assignable to an rpc interface.
    template <typename ...Names, if_equal<sizeof...(Names), size> = true>
    inline constexpr method(Names&&... names) NOEXCEPT
      : names_{ std::forward<Names>(names)... }
    {
    }

    inline constexpr const names& parameter_names() const NOEXCEPT
    {
        return names_;
    }

private:
    const names names_;
};

BC_POP_WARNING()

/// Type helpers.
/// ---------------------------------------------------------------------------

/// method extraction

template <typename Type, typename = bool>
struct parameter_names {};

template <typename Type>
struct parameter_names<Type, bool_if<!is_tuple<Type>>>
{
    using type = typename Type::names;
};

template <typename Type>
struct parameter_names<Type, bool_if<is_tuple<Type>>>
{
    template <typename Tuple>
    struct unpack;

    template <typename ...Args>
    struct unpack<std::tuple<Args...>>
    {
        using type = typename method<"", Args...>::names;
    };

    using type = typename unpack<Type>::type;
};

template <typename Type>
using names_t = typename parameter_names<Type>::type;

template <typename Method>
using args_t = typename Method::args;

template <typename Method>
using tag_t = typename Method::tag;

template <size_t Index, typename Methods>
using method_t = std::tuple_element_t<Index, Methods>;

/// handler traits extraction

template <typename Handler, typename = void>
struct traits;

template <typename Handler>
struct traits<Handler, std::void_t<decltype(&Handler::operator())>>
  : traits<decltype(&Handler::operator())>
{
};

template <typename Return, typename Class, typename Tag, typename ...Args>
struct traits<Return(Class::*)(const code&, Tag, Args...) const NOEXCEPT>
{
    using tag = Tag;
    using args = std::tuple<Args...>;
};

/// Type helpers (default parameter values).
/// ---------------------------------------------------------------------------
/// array_t, and object_t do not have defaults (just empty), null_t is N/A.

template <auto Default>
struct option;

/// number_t  : option<4.2>
/// boolean_t : option<true>
template <auto Default> requires
    std::same_as<decltype(Default), number_t> ||
    std::same_as<decltype(Default), boolean_t>
struct option<Default>
{
    using type = decltype(Default);
    static constexpr type value = Default;
};

/// string_t : option<"hello world!"_t>
template <size_t Size, std::array<char, Size> Default>
struct option<Default>
{
    using type = string_t;
    static constexpr std::string_view value{ Default.data(), Size };
};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
