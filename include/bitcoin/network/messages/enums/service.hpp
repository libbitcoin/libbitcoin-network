/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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

#include <cstdint>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

enum service: uint64_t
{
    // The node exposes no services.
    node_none = 0,

    // The node is capable of serving the block chain (full node).
    node_network = system::bit_right<uint32_t>(0),

    // Requires version.value >= level::bip64 (BIP64 is draft only).
    // The node is capable of responding to the getutxo protocol request.
    node_utxo = system::bit_right<uint32_t>(1),

    // Requires version.value >= level::bip111
    // The node is capable and willing to handle bloom-filtered connections.
    node_bloom = system::bit_right<uint32_t>(2),

    // Independent of network protocol level.
    // The node is capable of responding to witness inventory requests.
    node_witness = system::bit_right<uint32_t>(3),

    // The node is capable of responding to getcfilters, getcfheaders,
    // and getcfcheckpt protocol requests.
    node_client_filters = system::bit_right<uint32_t>(6)
};

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
