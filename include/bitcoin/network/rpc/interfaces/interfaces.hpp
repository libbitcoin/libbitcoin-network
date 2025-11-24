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

#include <bitcoin/network/rpc/interfaces/bitcoind.hpp>
#include <bitcoin/network/rpc/interfaces/electrum.hpp>
#include <bitcoin/network/rpc/interfaces/explore.hpp>
#include <bitcoin/network/rpc/interfaces/http.hpp>
#include <bitcoin/network/rpc/interfaces/peer.hpp>
#include <bitcoin/network/rpc/interfaces/stratum_v1.hpp>
#include <bitcoin/network/rpc/interfaces/stratum_v2.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {
namespace interface {

using bitcoind   = publish<bitcoind_methods>;
using electrum   = publish<electrum_methods>;
using explore    = publish<explore_methods>;
using http       = publish<http_methods, grouping::positional>;
using peer       = publish<peer_methods, grouping::positional>;
using stratum_v1 = publish<stratum_v1_methods>;
using stratum_v2 = publish<stratum_v2_methods>;

} // namespace interface
} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
