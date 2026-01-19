///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2025 libbitcoin-network developers (see COPYING).
//
//        GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
//
///////////////////////////////////////////////////////////////////////////////
#ifndef LIBBITCOIN_NETWORK_HPP
#define LIBBITCOIN_NETWORK_HPP

/**
 * API Users: Include only this header. Direct use of other headers is fragile
 * and unsupported as header organization is subject to change.
 *
 * Maintainers: Do not include this header internal to this library.
 */

#include <bitcoin/system.hpp>
#include <bitcoin/network/beast.hpp>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/have.hpp>
#include <bitcoin/network/memory.hpp>
#include <bitcoin/network/net.hpp>
#include <bitcoin/network/preprocessor.hpp>
#include <bitcoin/network/settings.hpp>
#include <bitcoin/network/version.hpp>
#include <bitcoin/network/async/asio.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/async/desubscriber.hpp>
#include <bitcoin/network/async/enable_shared_from_base.hpp>
#include <bitcoin/network/async/handlers.hpp>
#include <bitcoin/network/async/subscriber.hpp>
#include <bitcoin/network/async/thread.hpp>
#include <bitcoin/network/async/threadpool.hpp>
#include <bitcoin/network/async/time.hpp>
#include <bitcoin/network/async/unsubscriber.hpp>
#include <bitcoin/network/async/races/race_all.hpp>
#include <bitcoin/network/async/races/race_quality.hpp>
#include <bitcoin/network/async/races/race_speed.hpp>
#include <bitcoin/network/async/races/race_unity.hpp>
#include <bitcoin/network/async/races/race_volume.hpp>
#include <bitcoin/network/async/races/races.hpp>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/channels/channel_http.hpp>
#include <bitcoin/network/channels/channel_peer.hpp>
#include <bitcoin/network/channels/channel_rpc.hpp>
#include <bitcoin/network/channels/channel_ws.hpp>
#include <bitcoin/network/channels/channels.hpp>
#include <bitcoin/network/config/address.hpp>
#include <bitcoin/network/config/authority.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/config/endpoint.hpp>
#include <bitcoin/network/config/utilities.hpp>
#include <bitcoin/network/interfaces/http.hpp>
#include <bitcoin/network/interfaces/interfaces.hpp>
#include <bitcoin/network/interfaces/peer_broadcast.hpp>
#include <bitcoin/network/interfaces/peer_dispatch.hpp>
#include <bitcoin/network/log/capture.hpp>
#include <bitcoin/network/log/levels.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/log/logger.hpp>
#include <bitcoin/network/log/reporter.hpp>
#include <bitcoin/network/log/timer.hpp>
#include <bitcoin/network/log/tracker.hpp>
#include <bitcoin/network/messages/http_body.hpp>
#include <bitcoin/network/messages/http_method.hpp>
#include <bitcoin/network/messages/json_body.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/messages/http/fields.hpp>
#include <bitcoin/network/messages/http/http.hpp>
#include <bitcoin/network/messages/http/enums/magic_numbers.hpp>
#include <bitcoin/network/messages/http/enums/media_type.hpp>
#include <bitcoin/network/messages/http/enums/status.hpp>
#include <bitcoin/network/messages/http/enums/target.hpp>
#include <bitcoin/network/messages/http/enums/verb.hpp>
#include <bitcoin/network/messages/peer/heading.hpp>
#include <bitcoin/network/messages/peer/message.hpp>
#include <bitcoin/network/messages/peer/peer.hpp>
#include <bitcoin/network/messages/peer/detail/address.hpp>
#include <bitcoin/network/messages/peer/detail/address_item.hpp>
#include <bitcoin/network/messages/peer/detail/alert.hpp>
#include <bitcoin/network/messages/peer/detail/alert_item.hpp>
#include <bitcoin/network/messages/peer/detail/block.hpp>
#include <bitcoin/network/messages/peer/detail/bloom_filter_add.hpp>
#include <bitcoin/network/messages/peer/detail/bloom_filter_clear.hpp>
#include <bitcoin/network/messages/peer/detail/bloom_filter_load.hpp>
#include <bitcoin/network/messages/peer/detail/client_filter.hpp>
#include <bitcoin/network/messages/peer/detail/client_filter_checkpoint.hpp>
#include <bitcoin/network/messages/peer/detail/client_filter_headers.hpp>
#include <bitcoin/network/messages/peer/detail/compact_block.hpp>
#include <bitcoin/network/messages/peer/detail/compact_block_item.hpp>
#include <bitcoin/network/messages/peer/detail/compact_transactions.hpp>
#include <bitcoin/network/messages/peer/detail/fee_filter.hpp>
#include <bitcoin/network/messages/peer/detail/get_address.hpp>
#include <bitcoin/network/messages/peer/detail/get_blocks.hpp>
#include <bitcoin/network/messages/peer/detail/get_client_filter_checkpoint.hpp>
#include <bitcoin/network/messages/peer/detail/get_client_filter_headers.hpp>
#include <bitcoin/network/messages/peer/detail/get_client_filters.hpp>
#include <bitcoin/network/messages/peer/detail/get_compact_transactions.hpp>
#include <bitcoin/network/messages/peer/detail/get_data.hpp>
#include <bitcoin/network/messages/peer/detail/get_headers.hpp>
#include <bitcoin/network/messages/peer/detail/headers.hpp>
#include <bitcoin/network/messages/peer/detail/inventory.hpp>
#include <bitcoin/network/messages/peer/detail/inventory_item.hpp>
#include <bitcoin/network/messages/peer/detail/memory_pool.hpp>
#include <bitcoin/network/messages/peer/detail/merkle_block.hpp>
#include <bitcoin/network/messages/peer/detail/not_found.hpp>
#include <bitcoin/network/messages/peer/detail/ping.hpp>
#include <bitcoin/network/messages/peer/detail/pong.hpp>
#include <bitcoin/network/messages/peer/detail/reject.hpp>
#include <bitcoin/network/messages/peer/detail/send_address_v2.hpp>
#include <bitcoin/network/messages/peer/detail/send_compact.hpp>
#include <bitcoin/network/messages/peer/detail/send_headers.hpp>
#include <bitcoin/network/messages/peer/detail/transaction.hpp>
#include <bitcoin/network/messages/peer/detail/version.hpp>
#include <bitcoin/network/messages/peer/detail/version_acknowledge.hpp>
#include <bitcoin/network/messages/peer/detail/witness_tx_id_relay.hpp>
#include <bitcoin/network/messages/peer/enums/identifier.hpp>
#include <bitcoin/network/messages/peer/enums/level.hpp>
#include <bitcoin/network/messages/peer/enums/magic_numbers.hpp>
#include <bitcoin/network/messages/peer/enums/service.hpp>
#include <bitcoin/network/messages/rpc/any.hpp>
#include <bitcoin/network/messages/rpc/body.hpp>
#include <bitcoin/network/messages/rpc/broadcaster.hpp>
#include <bitcoin/network/messages/rpc/dispatcher.hpp>
#include <bitcoin/network/messages/rpc/method.hpp>
#include <bitcoin/network/messages/rpc/model.hpp>
#include <bitcoin/network/messages/rpc/publish.hpp>
#include <bitcoin/network/messages/rpc/rpc.hpp>
#include <bitcoin/network/messages/rpc/types.hpp>
#include <bitcoin/network/messages/rpc/enums/grouping.hpp>
#include <bitcoin/network/messages/rpc/enums/version.hpp>
#include <bitcoin/network/net/acceptor.hpp>
#include <bitcoin/network/net/connector.hpp>
#include <bitcoin/network/net/connector_socks.hpp>
#include <bitcoin/network/net/deadline.hpp>
#include <bitcoin/network/net/hosts.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/net/socket.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/protocols/protocol_address_in_209.hpp>
#include <bitcoin/network/protocols/protocol_address_out_209.hpp>
#include <bitcoin/network/protocols/protocol_alert_311.hpp>
#include <bitcoin/network/protocols/protocol_http.hpp>
#include <bitcoin/network/protocols/protocol_peer.hpp>
#include <bitcoin/network/protocols/protocol_ping_106.hpp>
#include <bitcoin/network/protocols/protocol_ping_60001.hpp>
#include <bitcoin/network/protocols/protocol_reject_70002.hpp>
#include <bitcoin/network/protocols/protocol_rpc.hpp>
#include <bitcoin/network/protocols/protocol_seed_209.hpp>
#include <bitcoin/network/protocols/protocol_version_106.hpp>
#include <bitcoin/network/protocols/protocol_version_70001.hpp>
#include <bitcoin/network/protocols/protocol_version_70002.hpp>
#include <bitcoin/network/protocols/protocol_version_70016.hpp>
#include <bitcoin/network/protocols/protocol_ws.hpp>
#include <bitcoin/network/protocols/protocols.hpp>
#include <bitcoin/network/sessions/session.hpp>
#include <bitcoin/network/sessions/session_inbound.hpp>
#include <bitcoin/network/sessions/session_manual.hpp>
#include <bitcoin/network/sessions/session_outbound.hpp>
#include <bitcoin/network/sessions/session_peer.hpp>
#include <bitcoin/network/sessions/session_seed.hpp>
#include <bitcoin/network/sessions/session_server.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

#endif
