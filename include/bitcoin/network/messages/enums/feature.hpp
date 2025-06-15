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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_ENUMS_FEATURE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_ENUMS_FEATURE_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {

using feature_t = uint32_t;

/// Qualify namespace (avoids class enum because flags).
namespace feature {

/// Features maintain version-negotiated channel state. State configured after
/// version handshake is set by individual protocols. Flags are stored by the
/// channel and carry negotiated features forward from handshake. Flags are a
/// consequence of configured version, configured services, configured options,
/// peer version, peer services, and peer options (requested via messages or
/// version.relay).
enum : feature_t
{
    no_flags = 0,
    all_flags = system::bit_all<uint32_t>,

    /// -----------------------------------------------------------------------

    /// Set at 106 (n/a) [version, verack].
    version = system::bit_right<uint32_t>(0),

    /// Set at 106 (n/a) [ping].
    ping = system::bit_right<uint32_t>(1),

    /// Set at 106 (n/a), disabled by config.enable_address [addr].
    address = system::bit_right<uint32_t>(2),

    /// Set at 209 (n/a), disabled by config.enable_address [getaddr].
    get_address = system::bit_right<uint32_t>(3),

    /// Set at 311 (n/a), deprecated, enabled by config.enable_alert [alert].
    alert = system::bit_right<uint32_t>(4),

    /// Set at 31402 (n/a), disabled by enable_address [addr.timestamp].
    address_timestamp = system::bit_right<uint32_t>(5),

    /// Set at 31800 (n/a) [headers, getheaders].
    headers = system::bit_right<uint32_t>(6),

    /// Set at 60001 (bip31), nonce and pong messages added [ping.nonce, pong].
    ping_pong = system::bit_right<uint32_t>(7),

    /// Set at 70001 (bip37) [mempool].
    mempool = system::bit_right<uint32_t>(8),

    /// Set at 70001 (n/a) [notfound].
    not_found = system::bit_right<uint32_t>(9),

    /// Set at 70002 (n/a) [multiple inv response].
    mempool_multiple = system::bit_right<uint32_t>(10),

    /// Set at 70002 (bip61), deprecated, enabled by config.enable_reject [reject].
    reject = system::bit_right<uint32_t>(11),

    /// Set at 70013 (bip133), enabled by non-zero config.minimum_fee [feefilter].
    fee_filter = system::bit_right<uint32_t>(12),

    /// -----------------------------------------------------------------------

    /// Set by handshake message negotiation, at 70012 (bip130) [sendheaders].
    send_headers = system::bit_right<uint32_t>(15),

    /// Disabled by config.enable_compact.
    /// [sendcmpct, blocktxn, cmpctblock, getblocktxn]
    /// Set by post-handshake message negotiations, at 70014 (bip152).
    /// Negotiation can establishes multiple versions (only one is defined).
    /// Version information is maintained by the compact blocks protocol(s).
    send_compact = system::bit_right<uint32_t>(16),

    /// Disabled by config.enable_address_v2 (independent of enable_address).
    /// Set by handshake message negotiation, at 70016 (bip155) [sendaddrv2, addrv2].
    send_address_v2 = system::bit_right<uint32_t>(17),

    /// Disabled by config.enable_witness_tx.
    /// Set by handshake message negotiation, at 70016 (bip339) [wtxidrelay].
    send_witness_tx = system::bit_right<uint32_t>(18),

    /// [sendtxrcncl, reqrecon, sketch, reqsketchext, reconcildiff].
    /// Disabled by config.enable_erlay, requires wtxidrelay [not supported].
    /// Set by handshake message negotiation, at 70016 (bip330).
    send_reconcile = system::bit_right<uint32_t>(19),

    /// -----------------------------------------------------------------------

    /// [getblocks, inv, getdata, block, tx] 
    /// Set by service::node_network [0], at 106 (excluding 32000 - 32400).
    blocks = system::bit_right<uint32_t>(13),

    /// Disabled by config.enable_relay.
    /// Set by service::node_network [0], at 106.
    /// Negated by the version.relay flag, at 70001 (bip37) [version.relay].
    transactions = system::bit_right<uint32_t>(14),

    /// [merkleblock, filterload, filteradd, filterclear]
    /// Set at 70001 (bip37) [not supported, queries allowed but ignored].
    /// Set by service::node_bloom [2], at 70011 [otherwise false] (bip111).
    bloom = system::bit_right<uint32_t>(20),

    /// Set by service::node_witness [3], adds MSG_WITNESS_* inv (bip144).
    witness = system::bit_right<uint32_t>(21),

    /// Set by service::node_client_filters [6], at 70015 (bip157).
    filters = system::bit_right<uint32_t>(22)
};

} // namespace feature
} // namespace network
} // namespace libbitcoin

#endif
