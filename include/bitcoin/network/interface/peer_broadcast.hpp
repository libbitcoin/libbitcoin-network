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
#ifndef LIBBITCOIN_NETWORK_INTERFACE_PEER_BROADCAST_HPP
#define LIBBITCOIN_NETWORK_INTERFACE_PEER_BROADCAST_HPP

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

struct peer_broadcast
{
    /// Required for use of network::desubscriber.
    using key = uint64_t;

    /// Desubscriber requires bool handlers, injects `code` parameter.
    template <typename... Args>
    using subscriber = network::desubscriber<key, Args...>;

    /// dispatcher.subscribe(std::forward<signature>(handler));
    /// `key` parameter is passed by message issuer to self-identify.
    template <class Message>
    using signature = std::function<bool(const code&,
        const typename Message::cptr&, const key&)>;

    static constexpr std::tuple methods
    {
        method<"addr", messages::peer::address::cptr, key>{},
        method<"alert", messages::peer::alert::cptr, key>{},
        method<"block", messages::peer::block::cptr, key>{},
        method<"filteradd", messages::peer::bloom_filter_add::cptr, key>{},
        method<"filterclear", messages::peer::bloom_filter_clear::cptr, key>{},
        method<"filterload", messages::peer::bloom_filter_load::cptr, key>{},
        method<"cfilter", messages::peer::client_filter::cptr, key>{},
        method<"cfcheckpt", messages::peer::client_filter_checkpoint::cptr, key>{},
        method<"cfheaders", messages::peer::client_filter_headers::cptr, key>{},
        method<"cmpctblock", messages::peer::compact_block::cptr, key>{},
        method<"blocktxn", messages::peer::compact_transactions::cptr, key>{},
        method<"feefilter", messages::peer::fee_filter::cptr, key>{},
        method<"getaddr", messages::peer::get_address::cptr, key>{},
        method<"getblocks", messages::peer::get_blocks::cptr, key>{},
        method<"getcfcheckpt", messages::peer::get_client_filter_checkpoint::cptr, key>{},
        method<"getcfheaders", messages::peer::get_client_filter_headers::cptr, key>{},
        method<"getcfilters", messages::peer::get_client_filters::cptr, key>{},
        method<"getblocktxn", messages::peer::get_compact_transactions::cptr, key>{},
        method<"getdata", messages::peer::get_data::cptr, key>{},
        method<"getheaders", messages::peer::get_headers::cptr, key>{},
        method<"headers", messages::peer::headers::cptr, key>{},
        method<"inv", messages::peer::inventory::cptr, key>{},
        method<"mempool", messages::peer::memory_pool::cptr, key>{},
        method<"merkleblock", messages::peer::merkle_block::cptr, key>{},
        method<"notfound", messages::peer::not_found::cptr, key>{},
        method<"ping", messages::peer::ping::cptr, key>{},
        method<"pong", messages::peer::pong::cptr, key>{},
        method<"reject", messages::peer::reject::cptr, key>{},
        method<"sendaddrv2", messages::peer::send_address_v2::cptr, key>{},
        method<"sendcmpct", messages::peer::send_compact::cptr, key>{},
        method<"sendheaders", messages::peer::send_headers::cptr, key>{},
        method<"tx", messages::peer::transaction::cptr, key>{},
        method<"version", messages::peer::version::cptr, key>{},
        method<"verack", messages::peer::version_acknowledge::cptr, key>{},
        method<"wtxidrelay", messages::peer::witness_tx_id_relay::cptr, key>{}
    };
};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
