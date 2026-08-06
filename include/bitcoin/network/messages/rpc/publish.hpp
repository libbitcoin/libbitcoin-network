/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_PUBLISH_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_PUBLISH_HPP

#include <array>
#include <tuple>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/enums/grouping.hpp>
#include <bitcoin/network/messages/rpc/method.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

/// Methods are a std::tuple of rpc::method<name, args>.
/// Defines a published interface for use with rpc::dispatcher<>.
template <typename Methods, grouping Mode = grouping::either>
struct publish
  : public Methods
{
    using type = decltype(Methods::methods);
    static constexpr auto size = std::tuple_size_v<type>;
    static constexpr grouping mode = Mode;
};

template <auto& Methods, size_t Index>
using method_at = std::tuple_element_t<Index,
    std::remove_reference_t<decltype(Methods)>>;

BC_PUSH_WARNING(NO_ARRAY_INDEXING)

/// The implemented method names, space delimited, in interface order.
/// Unimplemented methods are dispatchable but not published.
template <auto& Methods>
constexpr auto method_names() NOEXCEPT
{
    constexpr auto size = []() NOEXCEPT
    {
        size_t total{};
        std::apply([&](const auto&... items) NOEXCEPT
        {
            ((total += items.implemented() ? add1(items.name.size()) :
                zero), ...);
        }, Methods);

        return is_zero(total) ? total : sub1(total);
    }();

    size_t at{};
    std::array<char, size> out{};
    std::apply([&](const auto&... items) NOEXCEPT
    {
        const auto append = [&](const auto& item) NOEXCEPT
        {
            if (!item.implemented())
                return;

            if (!is_zero(at))
                out[at++] = ' ';

            for (const auto character: item.name)
                out[at++] = character;
        };

        (append(items), ...);
    }, Methods);

    return out;
}

BC_POP_WARNING()

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
