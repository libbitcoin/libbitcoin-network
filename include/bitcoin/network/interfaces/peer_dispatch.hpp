/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_NETWORK_INTERFACES_PEER_DISPATCH_HPP
#define LIBBITCOIN_NETWORK_INTERFACES_PEER_DISPATCH_HPP

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

struct peer_dispatch
{
    /// Unsubscriber requires bool handlers, injects `code` parameter.
    template <typename... Args>
    using subscriber = network::unsubscriber<Args...>;

    /// dispatcher.subscribe(std::forward<signature>(handler));
    template <class Message>
    using signature = std::function<bool(const code&,
        const typename Message::cptr&)>;

    static constexpr std::tuple methods
    {
        method<"addr", messages::peer::address::cptr>{},
        method<"alert", messages::peer::alert::cptr>{},
        method<"block", messages::peer::block::cptr>{},
        method<"filteradd", messages::peer::bloom_filter_add::cptr>{},
        method<"filterclear", messages::peer::bloom_filter_clear::cptr>{},
        method<"filterload", messages::peer::bloom_filter_load::cptr>{},
        method<"cfilter", messages::peer::client_filter::cptr>{},
        method<"cfcheckpt", messages::peer::client_filter_checkpoint::cptr>{},
        method<"cfheaders", messages::peer::client_filter_headers::cptr>{},
        method<"cmpctblock", messages::peer::compact_block::cptr>{},
        method<"blocktxn", messages::peer::compact_transactions::cptr>{},
        method<"feefilter", messages::peer::fee_filter::cptr>{},
        method<"getaddr", messages::peer::get_address::cptr>{},
        method<"getblocks", messages::peer::get_blocks::cptr>{},
        method<"getcfcheckpt", messages::peer::get_client_filter_checkpoint::cptr>{},
        method<"getcfheaders", messages::peer::get_client_filter_headers::cptr>{},
        method<"getcfilters", messages::peer::get_client_filters::cptr>{},
        method<"getblocktxn", messages::peer::get_compact_transactions::cptr>{},
        method<"getdata", messages::peer::get_data::cptr>{},
        method<"getheaders", messages::peer::get_headers::cptr>{},
        method<"headers", messages::peer::headers::cptr>{},
        method<"inv", messages::peer::inventory::cptr>{},
        method<"mempool", messages::peer::memory_pool::cptr>{},
        method<"merkleblock", messages::peer::merkle_block::cptr>{},
        method<"notfound", messages::peer::not_found::cptr>{},
        method<"ping", messages::peer::ping::cptr>{},
        method<"pong", messages::peer::pong::cptr>{},
        method<"reject", messages::peer::reject::cptr>{},
        method<"sendaddrv2", messages::peer::send_address_v2::cptr>{},
        method<"sendcmpct", messages::peer::send_compact::cptr>{},
        method<"sendheaders", messages::peer::send_headers::cptr>{},
        method<"tx", messages::peer::transaction::cptr>{},
        method<"version", messages::peer::version::cptr>{},
        method<"verack", messages::peer::version_acknowledge::cptr>{},
        method<"wtxidrelay", messages::peer::witness_tx_id_relay::cptr>{}
    };
};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
