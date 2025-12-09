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
#ifndef LIBBITCOIN_NETWORK_RPC_INTERFACES_HPP
#define LIBBITCOIN_NETWORK_RPC_INTERFACES_HPP

#include <bitcoin/network/rpc/interfaces/http.hpp>
#include <bitcoin/network/rpc/interfaces/peer_broadcast.hpp>
#include <bitcoin/network/rpc/interfaces/peer_dispatch.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {
namespace interface {

using http = publish<http_methods, grouping::positional>;

namespace peer {
using dispatch = publish<peer_dispatch, grouping::positional>;
using broadcast = publish<peer_broadcast, grouping::positional>;
} // namespace peer

} // namespace interface
} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
