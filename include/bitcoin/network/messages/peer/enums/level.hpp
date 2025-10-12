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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_PEER_ENUMS_LEVEL_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_PEER_ENUMS_LEVEL_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {
    
// Minimum current libbitcoin protocol version: 31402
// Minimum current satoshi    protocol version: 31800

// * sendaddrv2 is unversioned in BIP155 but requires version 70016 in Sataoshi:
// "BIP155 defines addrv2 and sendaddrv2 for all protocol versions, but some
// implementations reject messages they don't know. As a courtesy, don't send
// it to nodes with a version before 70016, as no software is known to support
// BIP155 that doesn't announce at least that protocol version number."
// ** TODO: these should be based solely on NODE_COMPACT_FILTERS signal, but we
// may associate the protocol version at which it was deployed (70015).
// *** BIP330 is not versioned, but states "Since sketches are based on the
// WTXIDs, the negotiation and support of Erlay should be enabled only if both
// peers signal BIP-339 support." Therefore it requires version 70016.

// libbitcoin-network
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// version      v1        106   N/A
// verack       v1        106   N/A
// ping         v1        106   N/A
// addr         v1        106   N/A
// addr         v1      31402   N/A     timestamp field added to addr message
// ----------------------------------------------------------------------------
// getaddr      v1        209   N/A
// checkorder   --        209   N/A     obsolete
// reply        --        209   N/A     obsolete
// submitorder  --        209   N/A     obsolete
// alert        v4        311   N/A     disabled by default, deprecated
// ----------------------------------------------------------------------------
// ping         v2      60001   BIP031  added nonce field
// pong         v1      60001   BIP031
// reject       v3      70002   BIP061  disabled by default, deprecated
// ----------------------------------------------------------------------------
// sendaddrv2   --      70016   BIP155  in-handshake, single (*)
// addrv2       --      70016   BIP155
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// libbitcoin-node
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// getblocks    v1        106   N/A
// inv          v1        106   N/A
// getdata      v1        106   N/A
// block        v1        106   N/A
// tx           v1        106   N/A
// ----------------------------------------------------------------------------
// getheaders   v3      31800   N/A     "headers first" added in v4
// headers      v3      31800   N/A     "headers first" added in v4
// mempool      v3      60002   BIP035
// ----------------------------------------------------------------------------
// merkleblock  --      70001   BIP037  deprecated (bip111)
// getdata      --      70001   BIP037  deprecated (bip111)
// filterload   --      70001   BIP037  deprecated (bip111)
// filteradd    --      70001   BIP037  deprecated (bip111)
// filterclear  --      70001   BIP037  deprecated (bip111)
// notfound     v2      70001   N/A     added at the same version as bip37
// version      v2      70001   N/A     added (optional) relay field in bip37
// ----------------------------------------------------------------------------
// mempool      v3      70002   N/A     allow multiple inv reply
// sendheaders  v3      70012   BIP130  post-handshake, single
// feefilter    v3      70013   BIP133
// ----------------------------------------------------------------------------
// blocktxn     v4      70014   BIP152
// cmpctblock   v4      70014   BIP152
// getblocktxn  v4      70014   BIP152
// sendcmpct    v4      70014   BIP152  post-handshake, multiple (versioned)
// ----------------------------------------------------------------------------
// cfilter      v4      70015   BIP157  not BIP-associated to net version (**)
// getcfilters  v4      70015   BIP157  not BIP-associated to net version (**)
// cfcheckpt    v4      70015   BIP157  not BIP-associated to net version (**)
// getcfcheckpt v4      70015   BIP157  not BIP-associated to net version (**)
// cfheaders    v4      70015   BIP157  not BIP-associated to net version (**)
// getcfheaders v4      70015   BIP157  not BIP-associated to net version (**)
// ----------------------------------------------------------------------------
// wtxidrelay   v4      70016   BIP339  in-handshake, single
// sendtxrcncl  --      70016   BIP330  no intent to support (***)
// reqrecon     --      70016   BIP330  no intent to support (***)
// sketch       --      70016   BIP330  no intent to support (***)
// reqsketchext --      70016   BIP330  no intent to support (***)
// reconcildiff --      70016   BIP330  no intent to support (***)
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

enum level: uint32_t
{
    /// Used to generate canonical size required by consensus checks.
    canonical = 0,

    /// -----------------------------------------------------------------------

    /// This is the first public release protocol version.
    /// Added verack, version.address_sender, version.nonce, version.user_agent.
    version_message = 106,

    /// Added getaddr message, also version.start_height and heading.checksum.
    /// Checksum was added to version after a delay, breaking earlier clients.
    get_address_message = 209,

    /// Added alert message.
    alert_message = 311,

    /// Added address.timestamp field to addresses.
    address_timestamp = 31402,

    /// This preceded the BIP system.
    headers_protocol = 31800,

    /// Don't request blocks from nodes of versions 32000-32400 (bitcoind hack).
    no_blocks_start = 32000,

    /// Don't request blocks from nodes of versions 32000-32400 (bitcoind hack).
    no_blocks_end = 32400,

    /// -----------------------------------------------------------------------

    /// ping.nonce, pong
    bip31 = 60001,

    /// memory_pool
    bip35 = 60002,

    /// version.relay, bloom filters, merkle_block, not_found
    bip37 = 70001,

    /// reject (satoshi node writes version.relay starting here)
    bip61 = 70002,

    /////// node_utxo service bit (draft)
    ////bip64 = 70004,

    /// node_bloom service bit (disables bloom filtering if not set)
    bip111 = 70011,

    /// send_headers
    bip130 = 70012,

    /// fee_filter
    bip133 = 70013,

    /// compact blocks protocol
    bip152 = 70014,

    /// client filters protocol
    bip157 = 70015,

    /// send_address_v2
    bip155 = 70016,

    /// wtxidrelay
    bip339 = 70016,

    /// erlay [not supported]
    bip330 = 70016,

    /// -----------------------------------------------------------------------

    /// We require at least this of peers (for current address structure).
    minimum_protocol = address_timestamp,

    /// We support at most this internally (bound to settings default).
    maximum_protocol = bip157
};

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
