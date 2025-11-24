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
#ifndef LIBBITCOIN_NETWORK_RPC_METHOD_HPP
#define LIBBITCOIN_NETWORK_RPC_METHOD_HPP

#include <tuple>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/rpc/types.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

BC_PUSH_WARNING(NO_ARRAY_TO_POINTER_DECAY)

/// Defines methods assignable to an rpc interface.
template <text_t Text, typename ...Args>
struct method
{
    static_assert(only_trailing_optionals<Args...>, "optionals must be last");
    static constexpr std::string_view name{ Text.text.data(), Text.text.size() };
    static constexpr bool native = is_tagged<std::tuple<Args...>>;
    static constexpr auto size = sizeof...(Args);

    using tag = method;
    using args_native = std::tuple<Args...>;
    using args = iif<native, args_native, std::tuple<tag, Args...>>;
    using names = std::array<std::string_view, size>;

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

    /// The ordered set of names corresponding to parameter types.
    inline constexpr const names& parameter_names() const NOEXCEPT
    {
        return names_;
    }

private:
    const names names_;
};

/// Helpers dependent upon method.
/// ---------------------------------------------------------------------------

template <typename Method, typename = bool>
struct names {};

template <typename Method>
struct names<Method, bool_if<!is_tuple<Method>>>
{
    using type = typename Method::names;
};

template <typename Method>
struct names<Method, bool_if<is_tuple<Method>>>
{
    template <typename Tuple>
    struct unpack;

    template <typename ...Args>
    struct unpack<std::tuple<Args...>>
    {
        using type = typename method<"", Args...>::names;
    };

    using type = typename unpack<Method>::type;
};

template <typename Method>
using names_t = typename names<Method>::type;

/// args_t/method::args is used for signature matching. If method::native then
/// this is the same as args_native_t - no synthetic tag (first argument).
template <typename Method>
using args_t = typename Method::args;

/// args_native_t/method::args_native is used for parameter value extraction.
template <typename Method>
using args_native_t = typename Method::args_native;

template <typename Method>
using tag_t = typename Method::tag;

template <size_t Index, typename Methods>
using method_t = std::tuple_element_t<Index, Methods>;

/// subscribers_t<Method...>
/// ---------------------------------------------------------------------------



BC_POP_WARNING()

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
