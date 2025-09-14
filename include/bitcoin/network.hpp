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
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/client.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/memory.hpp>
#include <bitcoin/network/net.hpp>
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
#include <bitcoin/network/config/address.hpp>
#include <bitcoin/network/config/authority.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/config/endpoint.hpp>
#include <bitcoin/network/config/utilities.hpp>
#include <bitcoin/network/log/capture.hpp>
#include <bitcoin/network/log/levels.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/log/logger.hpp>
#include <bitcoin/network/log/reporter.hpp>
#include <bitcoin/network/log/timer.hpp>
#include <bitcoin/network/log/tracker.hpp>
#include <bitcoin/network/messages/p2p/address.hpp>
#include <bitcoin/network/messages/p2p/address_item.hpp>
#include <bitcoin/network/messages/p2p/alert.hpp>
#include <bitcoin/network/messages/p2p/alert_item.hpp>
#include <bitcoin/network/messages/p2p/block.hpp>
#include <bitcoin/network/messages/p2p/bloom_filter_add.hpp>
#include <bitcoin/network/messages/p2p/bloom_filter_clear.hpp>
#include <bitcoin/network/messages/p2p/bloom_filter_load.hpp>
#include <bitcoin/network/messages/p2p/client_filter.hpp>
#include <bitcoin/network/messages/p2p/client_filter_checkpoint.hpp>
#include <bitcoin/network/messages/p2p/client_filter_headers.hpp>
#include <bitcoin/network/messages/p2p/compact_block.hpp>
#include <bitcoin/network/messages/p2p/compact_block_item.hpp>
#include <bitcoin/network/messages/p2p/compact_transactions.hpp>
#include <bitcoin/network/messages/p2p/fee_filter.hpp>
#include <bitcoin/network/messages/p2p/get_address.hpp>
#include <bitcoin/network/messages/p2p/get_blocks.hpp>
#include <bitcoin/network/messages/p2p/get_client_filter_checkpoint.hpp>
#include <bitcoin/network/messages/p2p/get_client_filter_headers.hpp>
#include <bitcoin/network/messages/p2p/get_client_filters.hpp>
#include <bitcoin/network/messages/p2p/get_compact_transactions.hpp>
#include <bitcoin/network/messages/p2p/get_data.hpp>
#include <bitcoin/network/messages/p2p/get_headers.hpp>
#include <bitcoin/network/messages/p2p/headers.hpp>
#include <bitcoin/network/messages/p2p/heading.hpp>
#include <bitcoin/network/messages/p2p/inventory.hpp>
#include <bitcoin/network/messages/p2p/inventory_item.hpp>
#include <bitcoin/network/messages/p2p/memory_pool.hpp>
#include <bitcoin/network/messages/p2p/merkle_block.hpp>
#include <bitcoin/network/messages/p2p/message.hpp>
#include <bitcoin/network/messages/p2p/messages.hpp>
#include <bitcoin/network/messages/p2p/not_found.hpp>
#include <bitcoin/network/messages/p2p/ping.hpp>
#include <bitcoin/network/messages/p2p/pong.hpp>
#include <bitcoin/network/messages/p2p/reject.hpp>
#include <bitcoin/network/messages/p2p/send_address_v2.hpp>
#include <bitcoin/network/messages/p2p/send_compact.hpp>
#include <bitcoin/network/messages/p2p/send_headers.hpp>
#include <bitcoin/network/messages/p2p/transaction.hpp>
#include <bitcoin/network/messages/p2p/version.hpp>
#include <bitcoin/network/messages/p2p/version_acknowledge.hpp>
#include <bitcoin/network/messages/p2p/witness_tx_id_relay.hpp>
#include <bitcoin/network/messages/p2p/enums/identifier.hpp>
#include <bitcoin/network/messages/p2p/enums/level.hpp>
#include <bitcoin/network/messages/p2p/enums/magic_numbers.hpp>
#include <bitcoin/network/messages/p2p/enums/service.hpp>
#include <bitcoin/network/messages/rpc/heading.hpp>
#include <bitcoin/network/messages/rpc/message.hpp>
#include <bitcoin/network/messages/rpc/messages.hpp>
#include <bitcoin/network/messages/rpc/ping.hpp>
#include <bitcoin/network/messages/rpc/enums/identifier.hpp>
#include <bitcoin/network/messages/rpc/enums/magic_numbers.hpp>
#include <bitcoin/network/net/acceptor.hpp>
#include <bitcoin/network/net/broadcaster.hpp>
#include <bitcoin/network/net/channel.hpp>
#include <bitcoin/network/net/channel_client.hpp>
#include <bitcoin/network/net/channel_peer.hpp>
#include <bitcoin/network/net/connector.hpp>
#include <bitcoin/network/net/deadline.hpp>
#include <bitcoin/network/net/distributor_client.hpp>
#include <bitcoin/network/net/distributor_peer.hpp>
#include <bitcoin/network/net/hosts.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/net/socket.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/protocols/protocol_address_in_209.hpp>
#include <bitcoin/network/protocols/protocol_address_out_209.hpp>
#include <bitcoin/network/protocols/protocol_alert_311.hpp>
#include <bitcoin/network/protocols/protocol_client.hpp>
#include <bitcoin/network/protocols/protocol_peer.hpp>
#include <bitcoin/network/protocols/protocol_ping_106.hpp>
#include <bitcoin/network/protocols/protocol_ping_60001.hpp>
#include <bitcoin/network/protocols/protocol_reject_70002.hpp>
#include <bitcoin/network/protocols/protocol_seed_209.hpp>
#include <bitcoin/network/protocols/protocol_version_106.hpp>
#include <bitcoin/network/protocols/protocol_version_70001.hpp>
#include <bitcoin/network/protocols/protocol_version_70002.hpp>
#include <bitcoin/network/protocols/protocol_version_70016.hpp>
#include <bitcoin/network/protocols/protocols.hpp>
#include <bitcoin/network/sessions/session.hpp>
#include <bitcoin/network/sessions/session_client.hpp>
#include <bitcoin/network/sessions/session_inbound.hpp>
#include <bitcoin/network/sessions/session_inbound_client.hpp>
#include <bitcoin/network/sessions/session_manual.hpp>
#include <bitcoin/network/sessions/session_outbound.hpp>
#include <bitcoin/network/sessions/session_peer.hpp>
#include <bitcoin/network/sessions/session_seed.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

#endif
