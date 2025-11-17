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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_TYPES_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_TYPES_HPP

#include <concepts>
#include <optional>
#include <tuple>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/model.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

/// Handler traits extraction.
/// ---------------------------------------------------------------------------

template <typename Handler, typename = void>
struct traits;

template <typename Handler>
struct traits<Handler, std::void_t<decltype(&Handler::operator())>>
  : traits<decltype(&Handler::operator())> {};

template <typename Return, typename Class, typename Tag, typename ...Args>
struct traits<Return(Class::*)(const code&, Tag, Args...) const NOEXCEPT>
{
    using tag = Tag;
    using args = std::tuple<Args...>;
};

/// Types and traits for optional/nullable method parameter descriptors.
/// ---------------------------------------------------------------------------

struct optional_tag {};
struct nullable_tag {};
enum class empty { array, object };

/// Partial specializations for optional (values).
template <auto Default>
struct optional;

/// array_t : optional<array>
template <auto Default>
    requires std::same_as<decltype(Default), empty> &&
        (Default == empty::array)
struct optional<Default> {
    using tag = optional_tag;
    using type = array_t;

    /// array_t optional default is only/always empty.
    static constexpr type default_value() NOEXCEPT { return {}; }
};

/// object_t : optional<object>
template <auto Default>
    requires std::same_as<decltype(Default), empty> &&
        (Default == empty::object)
struct optional<Default> {
    using tag = optional_tag;
    using type = object_t;

    /// std::unordered_map{} is not constexpr (ok).
    /// object_t optional default is only/always empty.
    static const type default_value() NOEXCEPT  { return {}; }
};

/// number_t  : optional<4.2>
/// boolean_t : optional<true>
template <auto Default> requires
    std::same_as<decltype(Default), boolean_t> ||
    std::same_as<decltype(Default), number_t>
struct optional<Default>
{
    using tag = optional_tag;
    using type = decltype(Default);
    static consteval type default_value() NOEXCEPT { return Default; }
};

/// string_t : optional<"hello world!"_t>
template <size_t Size, std::array<char, Size> Default>
struct optional<Default>
{
    using tag = optional_tag;
    using type = string_t;

    /// NTTPs of structural types have static storage duration.
    static constexpr type default_value() NOEXCEPT
    {
        return type{ std::string_view{ Default.data(), Size } };
    }
};

/// Parameter is typed as std::optional<Type> with !has_value() when null_t.
template <typename Type> requires
    std::same_as<Type, boolean_t> || std::same_as<Type, number_t> ||
    std::same_as<Type, string_t> || std::same_as<Type, object_t> ||
    std::same_as<Type, array_t>
struct nullable
{
    using tag = nullable_tag;
    using type = Type;
};

/// Parse type traits.
/// ---------------------------------------------------------------------------

template <typename Type, typename = void>
struct is_optional
  : std::false_type {};

template <typename Type>
struct is_optional<Type, std::void_t<typename Type::tag>>
  : std::is_same<typename Type::tag, optional_tag> {};

template <typename Type, typename = void>
struct is_nullable : std::false_type {};

template <typename Type>
struct is_nullable<Type, std::void_t<typename Type::tag>>
  : std::is_same<typename Type::tag, nullable_tag> {};

template <typename Type>
struct is_required
  : std::bool_constant< !is_optional<Type>::value &&
        !is_nullable<Type>::value> {};

template <typename Argument, typename = void>
struct internal
{
    using type = Argument;
};

template <typename Argument>
struct internal<Argument, std::void_t<typename Argument::type>>
{
    using type = typename Argument::type;
};

template <typename Argument>
using internal_t = typename internal<Argument>::type;

template <typename Argument>
using external_t = iif<is_nullable<Argument>::value,
    std::optional<internal_t<Argument>>, internal_t<Argument>>;

template <typename Arguments>
    struct externals;

template <typename... Args>
struct externals<std::tuple<Args...>>
{
    using type = std::tuple<external_t<Args>...>;
};

template <typename Arguments>
using externals_t = typename externals<Arguments>::type;

/// Detect non-trailing optional positions in methods.
/// ---------------------------------------------------------------------------

template <typename... Args>
struct is_trailing_optionals;

template <>
struct is_trailing_optionals<> : std::true_type {};

template <typename... Args>
constexpr bool no_required() noexcept
{
    return ((!is_required<Args>::value) && ...);
}

template <typename First, typename... Rest>
struct is_trailing_optionals<First, Rest...>
  : iif<is_required<First>::value, is_trailing_optionals<Rest...>,
        std::bool_constant<no_required<Rest...>()>> {};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
