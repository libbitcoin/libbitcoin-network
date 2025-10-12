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
#include <bitcoin/network/async/beast.hpp>
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
#include <bitcoin/network/messages/peer/address.hpp>
#include <bitcoin/network/messages/peer/address_item.hpp>
#include <bitcoin/network/messages/peer/alert.hpp>
#include <bitcoin/network/messages/peer/alert_item.hpp>
#include <bitcoin/network/messages/peer/block.hpp>
#include <bitcoin/network/messages/peer/bloom_filter_add.hpp>
#include <bitcoin/network/messages/peer/bloom_filter_clear.hpp>
#include <bitcoin/network/messages/peer/bloom_filter_load.hpp>
#include <bitcoin/network/messages/peer/client_filter.hpp>
#include <bitcoin/network/messages/peer/client_filter_checkpoint.hpp>
#include <bitcoin/network/messages/peer/client_filter_headers.hpp>
#include <bitcoin/network/messages/peer/compact_block.hpp>
#include <bitcoin/network/messages/peer/compact_block_item.hpp>
#include <bitcoin/network/messages/peer/compact_transactions.hpp>
#include <bitcoin/network/messages/peer/fee_filter.hpp>
#include <bitcoin/network/messages/peer/get_address.hpp>
#include <bitcoin/network/messages/peer/get_blocks.hpp>
#include <bitcoin/network/messages/peer/get_client_filter_checkpoint.hpp>
#include <bitcoin/network/messages/peer/get_client_filter_headers.hpp>
#include <bitcoin/network/messages/peer/get_client_filters.hpp>
#include <bitcoin/network/messages/peer/get_compact_transactions.hpp>
#include <bitcoin/network/messages/peer/get_data.hpp>
#include <bitcoin/network/messages/peer/get_headers.hpp>
#include <bitcoin/network/messages/peer/headers.hpp>
#include <bitcoin/network/messages/peer/heading.hpp>
#include <bitcoin/network/messages/peer/inventory.hpp>
#include <bitcoin/network/messages/peer/inventory_item.hpp>
#include <bitcoin/network/messages/peer/memory_pool.hpp>
#include <bitcoin/network/messages/peer/merkle_block.hpp>
#include <bitcoin/network/messages/peer/message.hpp>
#include <bitcoin/network/messages/peer/messages.hpp>
#include <bitcoin/network/messages/peer/not_found.hpp>
#include <bitcoin/network/messages/peer/ping.hpp>
#include <bitcoin/network/messages/peer/pong.hpp>
#include <bitcoin/network/messages/peer/reject.hpp>
#include <bitcoin/network/messages/peer/send_address_v2.hpp>
#include <bitcoin/network/messages/peer/send_compact.hpp>
#include <bitcoin/network/messages/peer/send_headers.hpp>
#include <bitcoin/network/messages/peer/transaction.hpp>
#include <bitcoin/network/messages/peer/version.hpp>
#include <bitcoin/network/messages/peer/version_acknowledge.hpp>
#include <bitcoin/network/messages/peer/witness_tx_id_relay.hpp>
#include <bitcoin/network/messages/peer/enums/identifier.hpp>
#include <bitcoin/network/messages/peer/enums/level.hpp>
#include <bitcoin/network/messages/peer/enums/magic_numbers.hpp>
#include <bitcoin/network/messages/peer/enums/service.hpp>
#include <bitcoin/network/messages/client/messages.hpp>
#include <bitcoin/network/messages/client/method.hpp>
#include <bitcoin/network/messages/client/enums/magic_numbers.hpp>
#include <bitcoin/network/messages/client/enums/mime_type.hpp>
#include <bitcoin/network/messages/client/enums/status.hpp>
#include <bitcoin/network/messages/client/enums/target.hpp>
#include <bitcoin/network/messages/client/enums/verb.hpp>
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
#include <bitcoin/network/sessions/session_client_inbound.hpp>
#include <bitcoin/network/sessions/session_inbound.hpp>
#include <bitcoin/network/sessions/session_manual.hpp>
#include <bitcoin/network/sessions/session_outbound.hpp>
#include <bitcoin/network/sessions/session_peer.hpp>
#include <bitcoin/network/sessions/session_seed.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

#endif
