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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_INTERFACE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_INTERFACE_HPP

#include <tuple>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/enums/grouping.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {
    
template <typename Methods, grouping Mode = grouping::either>
struct interface
  : public Methods
{
    using type = decltype(Methods::methods);
    static constexpr auto size = std::tuple_size_v<type>;
    static constexpr grouping mode = Mode;
};

template <size_t Index, typename Methods>
using at = typename std::tuple_element_t<Index, Methods>::tag;

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
