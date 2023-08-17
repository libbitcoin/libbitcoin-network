/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_ENUMS_SERVICE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_ENUMS_SERVICE_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

enum service: uint64_t
{
    /// The node exposes no defined services.
    /// At least version, address, ping/pong, and reject protocols.
    node_none = 0,

    /// The node is capable of serving the block chain (full node).
    node_network = system::bit_right<uint32_t>(0),

    /// Requires version.value >= level::bip64 (BIP64 is obsolete).
    /// The node is capable of responding to the getutxo protocol request.
    node_utxo = system::bit_right<uint32_t>(1),

    /// Requires version.value >= level::bip111
    /// The node is capable and willing to handle bloom-filtered connections.
    node_bloom = system::bit_right<uint32_t>(2),

    /// Independent of network protocol level.
    /// The node is capable of responding to witness inventory requests.
    node_witness = system::bit_right<uint32_t>(3),

    /// Independent of network protocol level.
    /// The node is capable of responding to getcfilters, getcfheaders,
    /// and getcfcheckpt protocol requests.
    node_client_filters = system::bit_right<uint32_t>(6),

    /// Serves only the last 288 (2 day) blocks.
    node_xnetwork_limited = system::bit_right<uint32_t>(10),

    /// The minimum supported capability.
    minimum_services = node_none,

    /// The maximum supported capability.
    maximum_services = node_network | node_witness | node_client_filters
};

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
