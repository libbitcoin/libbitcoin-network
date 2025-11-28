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
#ifndef LIBBITCOIN_NETWORK_RPC_INTERFACES_PEER_HPP
#define LIBBITCOIN_NETWORK_RPC_INTERFACES_PEER_HPP

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/peer/peer.hpp>
#include <bitcoin/network/rpc/publish.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

#define RETURN_DESERIALIZED_PTR(message, ...) \
case messages::peer::identifier::message: \
{ \
    return { messages::peer::message::deserialize(version_, data \
        __VA_OPT__(,) __VA_ARGS__) }; \
}

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

    /// TODO: this moves to peer::body::reader
    /// Type-erased implementation of peer::deserialize<Message>.
    static inline any_t deserialize(memory& allocator,
        messages::peer::identifier identifier, const system::data_chunk& data,
        uint32_t version_, bool witness) NOEXCEPT
    {
        switch (identifier)
        {
            RETURN_DESERIALIZED_PTR(address)
            RETURN_DESERIALIZED_PTR(alert)
            RETURN_DESERIALIZED_PTR(block, witness, *allocator.get_arena())
            RETURN_DESERIALIZED_PTR(bloom_filter_add)
            RETURN_DESERIALIZED_PTR(bloom_filter_clear)
            RETURN_DESERIALIZED_PTR(bloom_filter_load)
            RETURN_DESERIALIZED_PTR(client_filter)
            RETURN_DESERIALIZED_PTR(client_filter_checkpoint)
            RETURN_DESERIALIZED_PTR(client_filter_headers)
            RETURN_DESERIALIZED_PTR(compact_block, witness)
            RETURN_DESERIALIZED_PTR(compact_transactions, witness)
            RETURN_DESERIALIZED_PTR(fee_filter)
            RETURN_DESERIALIZED_PTR(get_address)
            RETURN_DESERIALIZED_PTR(get_blocks);
            RETURN_DESERIALIZED_PTR(get_client_filter_checkpoint)
            RETURN_DESERIALIZED_PTR(get_client_filter_headers)
            RETURN_DESERIALIZED_PTR(get_client_filters)
            RETURN_DESERIALIZED_PTR(get_compact_transactions)
            RETURN_DESERIALIZED_PTR(get_data)
            RETURN_DESERIALIZED_PTR(get_headers)
            RETURN_DESERIALIZED_PTR(headers)
            RETURN_DESERIALIZED_PTR(inventory)
            RETURN_DESERIALIZED_PTR(memory_pool)
            RETURN_DESERIALIZED_PTR(merkle_block)
            RETURN_DESERIALIZED_PTR(not_found)
            RETURN_DESERIALIZED_PTR(ping)
            RETURN_DESERIALIZED_PTR(pong)
            RETURN_DESERIALIZED_PTR(reject)
            RETURN_DESERIALIZED_PTR(send_address_v2)
            RETURN_DESERIALIZED_PTR(send_compact)
            RETURN_DESERIALIZED_PTR(send_headers)
            RETURN_DESERIALIZED_PTR(transaction, witness)
            RETURN_DESERIALIZED_PTR(version)
            RETURN_DESERIALIZED_PTR(version_acknowledge)
            RETURN_DESERIALIZED_PTR(witness_tx_id_relay)
            default: return {};
        }
    }
};

#undef RETURN_DESERIALIZED_PTR

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
