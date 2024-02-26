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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_ENUMS_LEVEL_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_ENUMS_LEVEL_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
    
// Minimum current libbitcoin protocol version: 31402
// Minimum current satoshi    protocol version: 31800

// libbitcoin-network
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// version      v1
// verack       v1
// getaddr      v1
// addr         v1
// ping         v1
// ping         v2      60001   BIP031  added nonce field
// pong         v1      60001   BIP031
// reject       v3      70002   BIP061  disabled by default, deprecated
// ----------------------------------------------------------------------------
// alert        v4                      disabled by default, deprecated
// checkorder   --                      obsolete
// reply        --                      obsolete
// submitorder  --                      obsolete
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// libbitcoin-node
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// getblocks    v1
// inv          v1
// getdata      v1
// block        v1
// tx           v1
// getheaders   v3      31800           "headers first" added in v4
// headers      v3      31800           "headers first" added in v4
// mempool      v3      60002   BIP035
// ----------------------------------------------------------------------------
// merkleblock  v3      70001   BIP037  only unfiltered supported
// getdata      v3      70001   BIP037  allow filtered_block flag in bip37
// filterload   --      70001   BIP037  no intent to support, deprecated (bip111)
// filteradd    --      70001   BIP037  no intent to support, deprecated (bip111)
// filterclear  --      70001   BIP037  no intent to support, deprecated (bip111)
// notfound     v2      70001           added at the same version as bip37
// version      v2      70001           added (optional) relay field in bip37
// ----------------------------------------------------------------------------
// mempool      v3      70002           allow multiple inv messages in reply:
//                                      undocumented (satoshi v0.9.0)
// sendheaders  v3      70012   BIP130  "headers first" added in v4
// feefilter    v3      70013   BIP133
// blocktxn     v4      70014   BIP152
// cmpctblock   v4      70014   BIP152
// getblocktxn  v4      70014   BIP152
// sendcmpct    v4      70014   BIP152

// TODO: these should be based solely on NODE_COMPACT_FILTERS signal.
// TODO: but we may associate protocol version at which it was deployed.
// cfilter      v4      70015   BIP157  not BIP-associated to p2p version
// getcfilters  v4      70015   BIP157  not BIP-associated to p2p version
// cfcheckpt    v4      70015   BIP157  not BIP-associated to p2p version
// getcfcheckpt v4      70015   BIP157  not BIP-associated to p2p version
// cfheaders    v4      70015   BIP157  not BIP-associated to p2p version
// getcfheaders v4      70015   BIP157  not BIP-associated to p2p version

// sendaddrv2   --      00000   BIP155  compat break, unversioned, handshake
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

enum level: uint32_t
{
    /// Used to generate canonical size required by consensus checks.
    canonical = 0,

    /// Added version.address_sender, version.nonce, and version.user_agent.
    version_updates = 106,

    /// Added verack message, also heading.checksum and version.start_height.
    verack_message = 209,

    /// Added alert message.
    alert_message = 311,

    /// Added address.timestamp field to addresses.
    address_time = 31402,

    /// This preceded the BIP system.
    headers_protocol = 31800,

    /// Don't request blocks from nodes of versions 32000-32400 (bitcoind hack).
    no_blocks_start = 32000,

    /// Don't request blocks from nodes of versions 32000-32400 (bitcoind hack).
    no_blocks_end = 32400,

    /// Isolate protocol version from implementation version.
    bip14 = 60000,

    /// ping.nonce, pong
    bip31 = 60001,

    /// memory_pool
    bip35 = 60002,

    /// bloom filters, merkle_block, not_found, version.relay
    bip37 = 70001,

    /// reject (satoshi node writes version.relay starting here)
    bip61 = 70002,

    /// node_utxo service bit (draft)
    bip64 = 70004,

    /// node_bloom service bit
    bip111 = 70011,

    /// send_headers
    bip130 = 70012,

    /// fee_filter
    bip133 = 70013,

    /// compact blocks protocol
    bip152 = 70014,

    /// client filters protocol
    bip157 = 70015,

    /// We require at least this of peers (for current address structure).
    minimum_protocol = address_time,

    /// We support at most this internally (bound to settings default).
    maximum_protocol = bip130
};

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
