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
#ifndef LIBBITCOIN_NETWORK_DISTRIBUTORS_DISTRIBUTOR_HPP
#define LIBBITCOIN_NETWORK_DISTRIBUTORS_DISTRIBUTOR_HPP

#include <algorithm>
#include <tuple>
#include <bitcoin/network/define.hpp>

// TODO: move this.

namespace libbitcoin {
namespace network {
namespace rpc {

BC_PUSH_WARNING(NO_UNSAFE_COPY_N)
BC_PUSH_WARNING(NO_ARRAY_TO_POINTER_DECAY)

enum class group { positional, named, either };

/// Non-type template parameter (NTTP) dynamically defines a name for type.
template <size_t Length>
struct method_name
{
    constexpr method_name(const char (&text)[Length])
    {
        std::copy_n(text, Length, name);
    }

    char name[Length]{};
    static constexpr auto length = sub1(Length);
};

template <method_name Unique, typename... Args>
struct method
{
    static constexpr std::string name{ Unique.name, Unique.length };

    using tag = method;
    using args = std::tuple<Args...>;
    using names_t = std::array<std::string, sizeof...(Args)>;

    /// Required for construction of tag{}.
    constexpr method() NOEXCEPT
      : names_{}
    {
    }

    template <typename ...ParameterNames,
        if_equal<sizeof...(ParameterNames), sizeof...(Args)> = true>
    constexpr method(ParameterNames&&... names) NOEXCEPT
      : names_{ std::forward<ParameterNames>(names)... }
    {
    }

    constexpr const names_t& names() const NOEXCEPT
    {
        return names_;
    }

private:
    const names_t names_;
};

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
