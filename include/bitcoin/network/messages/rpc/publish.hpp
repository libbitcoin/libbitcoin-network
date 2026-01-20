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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_PUBLISH_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_PUBLISH_HPP

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

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
