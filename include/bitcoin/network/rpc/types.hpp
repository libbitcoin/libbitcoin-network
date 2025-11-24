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
#ifndef LIBBITCOIN_NETWORK_RPC_TYPES_HPP
#define LIBBITCOIN_NETWORK_RPC_TYPES_HPP

#include <optional>
#include <tuple>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/rpc/model.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

/// optional<>
/// ---------------------------------------------------------------------------

struct optional_tag {};
enum class empty { array, object };

/// Partial specializations for optional (values).
template <auto Default>
struct optional;

/// array_t : optional<empty::array>
template <auto Default> requires
    is_same_type<decltype(Default), empty> && (Default == empty::array)
struct optional<Default>
{
    using tag = optional_tag;
    using type = array_t;

    /// array_t optional default is only/always empty.
    static constexpr type default_value() NOEXCEPT { return {}; }
};

/// object_t : optional<empty::object>
template <auto Default> requires
    is_same_type<decltype(Default), empty> && (Default == empty::object)
struct optional<Default>
 {
    using tag = optional_tag;
    using type = object_t;

    /// std::unordered_map{} is not constexpr (ok).
    /// object_t optional default is only/always empty.
    static const type default_value() NOEXCEPT  { return {}; }
};

/// int8_t   : optional<42_i8>  (int8_t)
/// int16_t  : optional<42_i16> (int16_t)
/// int32_t  : optional<42_i32> (int32_t)
/// int64_t  : optional<42_i64> (int64_t)
/// uint8_t  : optional<42_u8>  (uint8_t)
/// uint16_t : optional<42_u16> (uint16_t)
/// uint32_t : optional<42_u32> (uint32_t)
/// uint64_t : optional<42_u64> (uint64_t)
template <auto Default> requires 
    is_same_type<decltype(Default), int8_t> ||
    is_same_type<decltype(Default), int16_t> ||
    is_same_type<decltype(Default), int32_t> ||
    is_same_type<decltype(Default), int64_t> ||
    is_same_type<decltype(Default), uint8_t> ||
    is_same_type<decltype(Default), uint16_t> ||
    is_same_type<decltype(Default), uint32_t> ||
    is_same_type<decltype(Default), uint64_t>
struct optional<Default>
{
    using tag = optional_tag;
    using type = decltype(Default);
    static constexpr type value = static_cast<type>(Default);
    static consteval type default_value() NOEXCEPT { return Default; }
};

/// number_t : optional<4.2> (double)
/// number_t : optional<4.2f> (float)
template <auto Default> requires
    is_same_type<decltype(Default), double> ||
    is_same_type<decltype(Default), float>
struct optional<Default>
{
    using tag = optional_tag;
    using type = number_t;
    static constexpr type value = Default;
    static consteval type default_value() NOEXCEPT { return Default; }
};

/// boolean_t : optional<true>
template <auto Default> requires
    is_same_type<decltype(Default), boolean_t>
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

/// nullable<>
/// ---------------------------------------------------------------------------

struct nullable_tag {};

/// Parameter is typed as std::optional<Type> with !has_value() when null_t.
template <typename Type> requires
    is_same_type<Type, object_t> || is_same_type<Type, array_t> ||
    is_same_type<Type, string_t> || is_same_type<Type, boolean_t> ||
    is_same_type<Type, number_t> || is_integral_integer<Type>
struct nullable
{
    using tag = nullable_tag;
    using type = Type;
};

/// is_optional<>, is_nullable<>, is_required<>
/// ---------------------------------------------------------------------------

template <typename Argument, typename = void>
struct is_optional_t
  : std::false_type {};

template <typename Argument>
struct is_optional_t<Argument, std::void_t<typename Argument::tag>>
  : std::is_same<typename Argument::tag, optional_tag> {};

template <typename Argument, typename = void>
struct is_nullable_t : std::false_type {};

template <typename Argument>
struct is_nullable_t<Argument, std::void_t<typename Argument::tag>>
  : std::is_same<typename Argument::tag, nullable_tag> {};

template <typename Argument>
constexpr bool is_optional = is_optional_t<Argument>::value;
template <typename Argument>
constexpr bool is_nullable = is_nullable_t<Argument>::value;
template <typename Argument>
constexpr bool is_required = !is_optional<Argument> && !is_nullable<Argument>;

/// internal_t (strip nullable<> and optional<>, wrap shared_ptr<> in any_t).
/// ---------------------------------------------------------------------------
/// Specializations are const sensitive, strip the const from method declares.

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
using internal_t = std::remove_cv_t<typename internal<Argument>::type>;

/// pointer_t (strip std::shared_ptr<>).
/// ---------------------------------------------------------------------------

template <typename Pointer, typename = void>
struct pointer {};

template <typename Type>
struct pointer<std::shared_ptr<Type>, void>
{
    using type = Type;
};

template <typename Type>
struct pointer<std::shared_ptr<const Type>, void>
{
    using type = const Type;
};

template <typename Pointer>
using pointer_t = typename pointer<Pointer>::type;

/// external_t (nullable<Type> -> std::optional<Type>, optional<Type> -> Type)
/// ---------------------------------------------------------------------------

template <typename Argument>
using external_t = iif<is_nullable<Argument>,
    std::optional<internal_t<Argument>>, internal_t<Argument>>;

/// externals_t (convert tuple to tuple of external_t)
/// ---------------------------------------------------------------------------

template <typename Arguments>
struct externals;

template <typename... Args>
struct externals<std::tuple<Args...>>
{
    using type = std::tuple<external_t<Args>...>;
};

template <typename Arguments>
using externals_t = typename externals<Arguments>::type;

/// apply_t<Template, ...Args> (apply template to external_t<Args>...)
/// ---------------------------------------------------------------------------

template <template <typename...> class Template, typename Arguments>
struct apply;

template <template <typename...> class Template, typename ...Args>
struct apply<Template, std::tuple<Args...>>
{
    using type = Template<external_t<Args>...>;
};

template <template <typename...> class Template, typename ...Args>
using apply_t = typename apply<Template, Args...>::type;

/// only_trailing_optionals<> : detect non-trailing optionals in arguments.
/// ---------------------------------------------------------------------------

template <typename ...Args>
struct only_trailing_optionals_t;

template <>
struct only_trailing_optionals_t<> : std::true_type {};

template <typename ...Args>
constexpr bool all_optional() noexcept
{
    return ((is_optional<Args>) && ...);
}

template <typename First, typename ...Rest>
struct only_trailing_optionals_t<First, Rest...>
  : iif<is_optional<First>,
        std::bool_constant<all_optional<Rest...>()>,
            only_trailing_optionals_t<Rest...>> {};

template <typename ...Args>
constexpr bool only_trailing_optionals = 
    only_trailing_optionals_t<Args...>::value;

/// is_tagged<Tuple> : detect native tag (first arg is shared_ptr).
/// ---------------------------------------------------------------------------
/// shared_ptr<Type> is wrapped in std::any<shared_ptr<Type>> within value_t.

template <typename Tuple, typename = void>
struct is_tagged_t : std::false_type {};

template <typename Tuple>
struct is_tagged_t<Tuple, std::enable_if_t<!is_empty_tuple<Tuple>>>
  : std::bool_constant<is_shared_ptr<std::tuple_element_t<zero, Tuple>>> {};

template <typename Tuple>
constexpr bool is_tagged = is_tagged_t<Tuple>::value;

/// handler_args_t<Handler> : get args (strip code, set args).
/// ---------------------------------------------------------------------------
/// Specializations are const sensitive, overrides provided.

template <typename Handler, typename = void>
struct handler_args;

template <typename Handler>
struct handler_args<Handler, std::void_t<decltype(&Handler::operator())>>
  : handler_args<decltype(&Handler::operator())> {};

template <typename Return, typename Class, typename Code, typename ...Args>
struct handler_args<Return(Class::*)(Code, Args...),
    std::enable_if_t<is_same_type<Code, code>>>
{
    using args = std::tuple<Args...>;
};

template <typename Return, typename Class, typename Code, typename ...Args>
struct handler_args<Return(Class::*)(Code, Args...) const,
    std::enable_if_t<is_same_type<Code, code>>>
{
    using args = std::tuple<Args...>;
};

template <typename Handler>
using handler_args_t = typename handler_args<Handler>::args;

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
