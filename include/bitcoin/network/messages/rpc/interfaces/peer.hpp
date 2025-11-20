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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_INTERFACES_PEER_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_INTERFACES_PEER_HPP

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/peer/peer.hpp>
#include <bitcoin/network/messages/rpc/publish.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

struct peer_methods
{
    static constexpr std::tuple methods
    {
        method<"address", messages::peer::address::cptr>{},
        method<"alert", messages::peer::alert::cptr>{},
        method<"block", messages::peer::block::cptr>{},
        method<"bloom_filter_add", messages::peer::bloom_filter_add::cptr>{},
        method<"bloom_filter_clear", messages::peer::bloom_filter_clear::cptr>{},
        method<"bloom_filter_load", messages::peer::bloom_filter_load::cptr>{},
        method<"client_filter", messages::peer::client_filter::cptr>{},
        method<"client_filter_checkpoint", messages::peer::client_filter_checkpoint::cptr>{},
        method<"client_filter_headers", messages::peer::client_filter_headers::cptr>{},
        method<"compact_block", messages::peer::compact_block::cptr>{},
        method<"compact_transactions", messages::peer::compact_transactions::cptr>{},
        method<"fee_filter", messages::peer::fee_filter::cptr>{},
        method<"get_address", messages::peer::get_address::cptr>{},
        method<"get_blocks", messages::peer::get_blocks::cptr>{},
        method<"get_client_filter_checkpoint", messages::peer::get_client_filter_checkpoint::cptr>{},
        method<"get_client_filter_headers", messages::peer::get_client_filter_headers::cptr>{},
        method<"get_client_filters", messages::peer::get_client_filters::cptr>{},
        method<"get_compact_transactions", messages::peer::get_compact_transactions::cptr>{},
        method<"get_data", messages::peer::get_data::cptr>{},
        method<"get_headers", messages::peer::get_headers::cptr>{},
        method<"headers", messages::peer::headers::cptr>{},
        method<"inventory", messages::peer::inventory::cptr>{},
        method<"memory_pool", messages::peer::memory_pool::cptr>{},
        method<"merkle_block", messages::peer::merkle_block::cptr>{},
        method<"not_found", messages::peer::not_found::cptr>{},
        method<"ping", messages::peer::ping::cptr>{},
        method<"pong", messages::peer::pong::cptr>{},
        method<"reject", messages::peer::reject::cptr>{},
        method<"send_address_v2", messages::peer::send_address_v2::cptr>{},
        method<"send_compact", messages::peer::send_compact::cptr>{},
        method<"send_headers", messages::peer::send_headers::cptr>{},
        method<"transaction", messages::peer::transaction::cptr>{},
        method<"version", messages::peer::version::cptr>{},
        method<"version_acknowledge", messages::peer::version_acknowledge::cptr>{},
        method<"witness_tx_id_relay", messages::peer::witness_tx_id_relay::cptr>{}
    };

    /// Unsubscriber requires bool handlers, injects `code` parameter.
    template <typename... Args>
    using subscriber = network::unsubscriber<Args...>;

    /// dispatcher.subscribe(std::forward<signature>(handler));
    template <class Message>
    using signature = std::function<bool(const code&,
        const typename Message::cptr&)>;
};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
