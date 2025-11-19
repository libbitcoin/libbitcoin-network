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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_INTERFACES_ELECTRUM_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_INTERFACES_ELECTRUM_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/interface.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

struct electrum_methods
{
    static constexpr std::tuple methods
    {
        /// Blockchain methods.
        method<"blockchain.block.header", number_t, optional<0>>{ "height", "cp_height" },
        method<"blockchain.block.headers", number_t, number_t, optional<0>>{ "start_height", "count", "cp_height" },
        method<"blockchain.estimatefee", number_t>{ "number" },
        method<"blockchain.headers.subscribe">{},
        method<"blockchain.relayfee">{},
        method<"blockchain.scripthash.get_balance", string_t>{ "scripthash" },
        method<"blockchain.scripthash.get_history", string_t>{ "scripthash" },
        method<"blockchain.scripthash.get_mempool", string_t>{ "scripthash" },
        method<"blockchain.scripthash.listunspent", string_t>{ "scripthash" },
        method<"blockchain.scripthash.subscribe", string_t>{ "scripthash" },
        method<"blockchain.scripthash.unsubscribe", string_t>{ "scripthash" },
        method<"blockchain.transaction.broadcast", string_t>{ "raw_tx" },
        method<"blockchain.transaction.get", string_t, optional<false>>{ "tx_hash", "verbose" },
        method<"blockchain.transaction.get_merkle", string_t, number_t>{ "tx_hash", "height" },
        method<"blockchain.transaction.id_from_pos", number_t, number_t, optional<false>>{ "height", "tx_pos", "merkle" },

        /// Server methods.
        method<"server.add_peer", object_t>{ "features" },
        method<"server.banner">{},
        method<"server.donation_address">{},
        method<"server.features">{},
        method<"server.peers.subscribe">{},
        method<"server.ping">{},
        method<"server.version", optional<""_t>, optional<"1.4"_t>>{ "client_name", "protocol_version" }
    };

    // Derive this from above in c++26 using reflection.
    using blockchain_block_header = at<0, decltype(methods)>;
    using blockchain_block_headers = at<1, decltype(methods)>;
    using blockchain_estimatefee = at<2, decltype(methods)>;
    using blockchain_headers_subscribe = at<3, decltype(methods)>;
    using blockchain_relayfee = at<4, decltype(methods)>;
    using blockchain_scripthash_get_balance = at<5, decltype(methods)>;
    using blockchain_scripthash_get_history = at<6, decltype(methods)>;
    using blockchain_scripthash_get_mempool = at<7, decltype(methods)>;
    using blockchain_scripthash_listunspent = at<8, decltype(methods)>;
    using blockchain_scripthash_subscribe = at<9, decltype(methods)>;
    using blockchain_scripthash_unsubscribe = at<10, decltype(methods)>;
    using blockchain_transaction_broadcast = at<11, decltype(methods)>;
    using blockchain_transaction_get = at<12, decltype(methods)>;
    using blockchain_transaction_get_merkle = at<13, decltype(methods)>;
    using blockchain_transaction_id_from_pos = at<14, decltype(methods)>;
    using server_add_peer = at<15, decltype(methods)>;
    using server_banner = at<16, decltype(methods)>;
    using server_donation_address = at<17, decltype(methods)>;
    using server_features = at<18, decltype(methods)>;
    using server_peers_subscribe = at<19, decltype(methods)>;
    using server_ping = at<20, decltype(methods)>;
    using server_version = at<21, decltype(methods)>;
};

using electrum = interface<electrum_methods>;

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
