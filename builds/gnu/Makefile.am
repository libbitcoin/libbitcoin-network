###############################################################################
#  Copyright (c) 2014-2026 libbitcoin-network developers (see COPYING).
#
#         GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
#
###############################################################################

# Automake settings.
#==============================================================================
# Look for macros in the 'm4' subdirectory.
#------------------------------------------------------------------------------
ACLOCAL_AMFLAGS = -I m4

# Distribute data.
#==============================================================================
# files => ${pkgconfigdir}
#------------------------------------------------------------------------------
pkgconfig_DATA = \
    libbitcoin-network.pc

# files => ${docdir}
#------------------------------------------------------------------------------
doc_DATA = \
    AUTHORS \
    ../../COPYING \
    ChangeLog \
    INSTALL \
    NEWS \
    README

# Libraries.
#==============================================================================
# Target library 'src/libbitcoin-network.la'
#------------------------------------------------------------------------------
lib_LTLIBRARIES = src/libbitcoin-network.la

src_libbitcoin_network_la_LIBS = src/libbitcoin-network.la

src_libbitcoin_network_la_CPPFLAGS = \
    -I${top_builddir}/include \
    -I${srcdir}/../../include \
    -I${top_builddir}/include/bitcoin/network/ssl \
    -I${srcdir}/../../include/bitcoin/network/ssl \
    ${libbitcoin_system_BUILD_CPPFLAGS}

src_libbitcoin_network_la_LDFLAGS = \
    ${libbitcoin_system_LDFLAGS}

src_libbitcoin_network_la_LIBADD = \
    ${libbitcoin_system_LIBS}

src_libbitcoin_network_la_SOURCES = \
    ${srcdir}/../../src/asio.cpp \
    ${srcdir}/../../src/error.cpp \
    ${srcdir}/../../src/memory.cpp \
    ${srcdir}/../../src/net.cpp \
    ${srcdir}/../../src/settings.cpp \
    ${srcdir}/../../src/async/thread.cpp \
    ${srcdir}/../../src/async/threadpool.cpp \
    ${srcdir}/../../src/async/time.cpp \
    ${srcdir}/../../src/broadcast/diagnostics.cpp \
    ${srcdir}/../../src/broadcast/terminator.cpp \
    ${srcdir}/../../src/channels/channel.cpp \
    ${srcdir}/../../src/channels/channel_http.cpp \
    ${srcdir}/../../src/channels/channel_peer.cpp \
    ${srcdir}/../../src/config/address.cpp \
    ${srcdir}/../../src/config/authority.cpp \
    ${srcdir}/../../src/config/credential.cpp \
    ${srcdir}/../../src/config/endpoint.cpp \
    ${srcdir}/../../src/config/utilities.cpp \
    ${srcdir}/../../src/log/capture.cpp \
    ${srcdir}/../../src/log/logger.cpp \
    ${srcdir}/../../src/log/reporter.cpp \
    ${srcdir}/../../src/messages/http_body.cpp \
    ${srcdir}/../../src/messages/http/fields.cpp \
    ${srcdir}/../../src/messages/http/enums/media_type.cpp \
    ${srcdir}/../../src/messages/http/enums/target.cpp \
    ${srcdir}/../../src/messages/peer/body.cpp \
    ${srcdir}/../../src/messages/peer/message.cpp \
    ${srcdir}/../../src/messages/peer/detail/address.cpp \
    ${srcdir}/../../src/messages/peer/detail/address_item.cpp \
    ${srcdir}/../../src/messages/peer/detail/address_v2.cpp \
    ${srcdir}/../../src/messages/peer/detail/alert.cpp \
    ${srcdir}/../../src/messages/peer/detail/alert_item.cpp \
    ${srcdir}/../../src/messages/peer/detail/block.cpp \
    ${srcdir}/../../src/messages/peer/detail/bloom_filter_add.cpp \
    ${srcdir}/../../src/messages/peer/detail/bloom_filter_clear.cpp \
    ${srcdir}/../../src/messages/peer/detail/bloom_filter_load.cpp \
    ${srcdir}/../../src/messages/peer/detail/client_filter.cpp \
    ${srcdir}/../../src/messages/peer/detail/client_filter_checkpoint.cpp \
    ${srcdir}/../../src/messages/peer/detail/client_filter_headers.cpp \
    ${srcdir}/../../src/messages/peer/detail/compact_block.cpp \
    ${srcdir}/../../src/messages/peer/detail/compact_block_item.cpp \
    ${srcdir}/../../src/messages/peer/detail/compact_transactions.cpp \
    ${srcdir}/../../src/messages/peer/detail/fee_filter.cpp \
    ${srcdir}/../../src/messages/peer/detail/get_address.cpp \
    ${srcdir}/../../src/messages/peer/detail/get_blocks.cpp \
    ${srcdir}/../../src/messages/peer/detail/get_client_filter_checkpoint.cpp \
    ${srcdir}/../../src/messages/peer/detail/get_client_filter_headers.cpp \
    ${srcdir}/../../src/messages/peer/detail/get_client_filters.cpp \
    ${srcdir}/../../src/messages/peer/detail/get_compact_transactions.cpp \
    ${srcdir}/../../src/messages/peer/detail/get_data.cpp \
    ${srcdir}/../../src/messages/peer/detail/get_headers.cpp \
    ${srcdir}/../../src/messages/peer/detail/headers.cpp \
    ${srcdir}/../../src/messages/peer/detail/heading.cpp \
    ${srcdir}/../../src/messages/peer/detail/inventory.cpp \
    ${srcdir}/../../src/messages/peer/detail/inventory_item.cpp \
    ${srcdir}/../../src/messages/peer/detail/memory_pool.cpp \
    ${srcdir}/../../src/messages/peer/detail/merkle_block.cpp \
    ${srcdir}/../../src/messages/peer/detail/not_found.cpp \
    ${srcdir}/../../src/messages/peer/detail/ping.cpp \
    ${srcdir}/../../src/messages/peer/detail/pong.cpp \
    ${srcdir}/../../src/messages/peer/detail/reject.cpp \
    ${srcdir}/../../src/messages/peer/detail/send_address_v2.cpp \
    ${srcdir}/../../src/messages/peer/detail/send_compact.cpp \
    ${srcdir}/../../src/messages/peer/detail/send_headers.cpp \
    ${srcdir}/../../src/messages/peer/detail/send_transaction_reconciliation.cpp \
    ${srcdir}/../../src/messages/peer/detail/transaction.cpp \
    ${srcdir}/../../src/messages/peer/detail/version.cpp \
    ${srcdir}/../../src/messages/peer/detail/version_acknowledge.cpp \
    ${srcdir}/../../src/messages/peer/detail/witness_tx_id_relay.cpp \
    ${srcdir}/../../src/messages/rpc/body.cpp \
    ${srcdir}/../../src/messages/rpc/model.cpp \
    ${srcdir}/../../src/messages/rpc/size.cpp \
    ${srcdir}/../../src/net/acceptor.cpp \
    ${srcdir}/../../src/net/acceptor_sam.cpp \
    ${srcdir}/../../src/net/connector.cpp \
    ${srcdir}/../../src/net/connector_socks.cpp \
    ${srcdir}/../../src/net/deadline.cpp \
    ${srcdir}/../../src/net/hosts.cpp \
    ${srcdir}/../../src/net/proxy.cpp \
    ${srcdir}/../../src/net/proxy_actions.cpp \
    ${srcdir}/../../src/net/proxy_queue.cpp \
    ${srcdir}/../../src/net/socket.cpp \
    ${srcdir}/../../src/net/socket_body.cpp \
    ${srcdir}/../../src/net/socket_connect.cpp \
    ${srcdir}/../../src/net/socket_http.cpp \
    ${srcdir}/../../src/net/socket_peer.cpp \
    ${srcdir}/../../src/net/socket_rpc.cpp \
    ${srcdir}/../../src/net/socket_stop.cpp \
    ${srcdir}/../../src/net/socket_tcp.cpp \
    ${srcdir}/../../src/net/socket_ws.cpp \
    ${srcdir}/../../src/net/socket_zmtp.cpp \
    ${srcdir}/../../src/p2ps/cipher.cpp \
    ${srcdir}/../../src/p2ps/stream.cpp \
    ${srcdir}/../../src/protocols/protocol.cpp \
    ${srcdir}/../../src/protocols/protocol_address_in_209.cpp \
    ${srcdir}/../../src/protocols/protocol_address_in_70016.cpp \
    ${srcdir}/../../src/protocols/protocol_address_out_209.cpp \
    ${srcdir}/../../src/protocols/protocol_address_out_70016.cpp \
    ${srcdir}/../../src/protocols/protocol_alert_311.cpp \
    ${srcdir}/../../src/protocols/protocol_http.cpp \
    ${srcdir}/../../src/protocols/protocol_peer.cpp \
    ${srcdir}/../../src/protocols/protocol_ping_106.cpp \
    ${srcdir}/../../src/protocols/protocol_ping_60001.cpp \
    ${srcdir}/../../src/protocols/protocol_reject_70002.cpp \
    ${srcdir}/../../src/protocols/protocol_seed_209.cpp \
    ${srcdir}/../../src/protocols/protocol_seed_70016.cpp \
    ${srcdir}/../../src/protocols/protocol_version_106.cpp \
    ${srcdir}/../../src/protocols/protocol_version_70001.cpp \
    ${srcdir}/../../src/protocols/protocol_version_70002.cpp \
    ${srcdir}/../../src/protocols/protocol_version_70014.cpp \
    ${srcdir}/../../src/protocols/protocol_version_70016.cpp \
    ${srcdir}/../../src/sessions/session.cpp \
    ${srcdir}/../../src/sessions/session_inbound.cpp \
    ${srcdir}/../../src/sessions/session_manual.cpp \
    ${srcdir}/../../src/sessions/session_outbound.cpp \
    ${srcdir}/../../src/sessions/session_peer.cpp \
    ${srcdir}/../../src/sessions/session_seed.cpp \
    ${srcdir}/../../src/sessions/session_server.cpp \
    ${srcdir}/../../src/tls/codec.cpp \
    ${srcdir}/../../src/tls/context.cpp \
    ${srcdir}/../../src/tls/record.cpp \
    ${srcdir}/../../src/tls/schedule.cpp \
    ${srcdir}/../../src/tls/server.cpp \
    ${srcdir}/../../src/zmtp/cipher.cpp \
    ${srcdir}/../../src/zmtp/context.cpp \
    ${srcdir}/../../src/zmtp/stream.cpp

if WITH_SSL

src_libbitcoin_network_la_SOURCES += \
    ${srcdir}/../../src/ssl/openssl.cpp

endif WITH_SSL

include_bitcoindir = \
    ${includedir}/bitcoin

include_bitcoin_HEADERS = \
    ${srcdir}/../../include/bitcoin/network.hpp

include_bitcoin_networkdir = \
    ${includedir}/bitcoin/network

include_bitcoin_network_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/asio.hpp \
    ${srcdir}/../../include/bitcoin/network/beast.hpp \
    ${srcdir}/../../include/bitcoin/network/boost.hpp \
    ${srcdir}/../../include/bitcoin/network/define.hpp \
    ${srcdir}/../../include/bitcoin/network/error.hpp \
    ${srcdir}/../../include/bitcoin/network/have.hpp \
    ${srcdir}/../../include/bitcoin/network/memory.hpp \
    ${srcdir}/../../include/bitcoin/network/net.hpp \
    ${srcdir}/../../include/bitcoin/network/preprocessor.hpp \
    ${srcdir}/../../include/bitcoin/network/settings.hpp \
    include/bitcoin/network/version.hpp

include_bitcoin_network_asyncdir = \
    ${includedir}/bitcoin/network/async

include_bitcoin_network_async_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/async/async.hpp \
    ${srcdir}/../../include/bitcoin/network/async/desubscriber.hpp \
    ${srcdir}/../../include/bitcoin/network/async/enable_shared_from_base.hpp \
    ${srcdir}/../../include/bitcoin/network/async/handlers.hpp \
    ${srcdir}/../../include/bitcoin/network/async/subscriber.hpp \
    ${srcdir}/../../include/bitcoin/network/async/thread.hpp \
    ${srcdir}/../../include/bitcoin/network/async/threadpool.hpp \
    ${srcdir}/../../include/bitcoin/network/async/time.hpp \
    ${srcdir}/../../include/bitcoin/network/async/unsubscriber.hpp

include_bitcoin_network_async_racesdir = \
    ${includedir}/bitcoin/network/async/races

include_bitcoin_network_async_races_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/async/races/race_all.hpp \
    ${srcdir}/../../include/bitcoin/network/async/races/race_any.hpp \
    ${srcdir}/../../include/bitcoin/network/async/races/race_quality.hpp \
    ${srcdir}/../../include/bitcoin/network/async/races/race_speed.hpp \
    ${srcdir}/../../include/bitcoin/network/async/races/race_unity.hpp \
    ${srcdir}/../../include/bitcoin/network/async/races/race_volume.hpp \
    ${srcdir}/../../include/bitcoin/network/async/races/races.hpp

include_bitcoin_network_broadcastdir = \
    ${includedir}/bitcoin/network/broadcast

include_bitcoin_network_broadcast_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/broadcast/broadcast.hpp \
    ${srcdir}/../../include/bitcoin/network/broadcast/diagnostics.hpp \
    ${srcdir}/../../include/bitcoin/network/broadcast/terminator.hpp

include_bitcoin_network_channelsdir = \
    ${includedir}/bitcoin/network/channels

include_bitcoin_network_channels_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/channels/channel.hpp \
    ${srcdir}/../../include/bitcoin/network/channels/channel_http.hpp \
    ${srcdir}/../../include/bitcoin/network/channels/channel_peer.hpp \
    ${srcdir}/../../include/bitcoin/network/channels/channel_rpc.hpp \
    ${srcdir}/../../include/bitcoin/network/channels/channels.hpp

include_bitcoin_network_configdir = \
    ${includedir}/bitcoin/network/config

include_bitcoin_network_config_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/config/address.hpp \
    ${srcdir}/../../include/bitcoin/network/config/address_type.hpp \
    ${srcdir}/../../include/bitcoin/network/config/authority.hpp \
    ${srcdir}/../../include/bitcoin/network/config/config.hpp \
    ${srcdir}/../../include/bitcoin/network/config/credential.hpp \
    ${srcdir}/../../include/bitcoin/network/config/endpoint.hpp \
    ${srcdir}/../../include/bitcoin/network/config/utilities.hpp

include_bitcoin_network_impl_asyncdir = \
    ${includedir}/bitcoin/network/impl/async

include_bitcoin_network_impl_async_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/impl/async/desubscriber.ipp \
    ${srcdir}/../../include/bitcoin/network/impl/async/enable_shared_from_base.ipp \
    ${srcdir}/../../include/bitcoin/network/impl/async/subscriber.ipp \
    ${srcdir}/../../include/bitcoin/network/impl/async/unsubscriber.ipp

include_bitcoin_network_impl_async_racesdir = \
    ${includedir}/bitcoin/network/impl/async/races

include_bitcoin_network_impl_async_races_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/impl/async/races/race_all.ipp \
    ${srcdir}/../../include/bitcoin/network/impl/async/races/race_any.ipp \
    ${srcdir}/../../include/bitcoin/network/impl/async/races/race_quality.ipp \
    ${srcdir}/../../include/bitcoin/network/impl/async/races/race_speed.ipp \
    ${srcdir}/../../include/bitcoin/network/impl/async/races/race_unity.ipp \
    ${srcdir}/../../include/bitcoin/network/impl/async/races/race_volume.ipp

include_bitcoin_network_impl_channelsdir = \
    ${includedir}/bitcoin/network/impl/channels

include_bitcoin_network_impl_channels_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/impl/channels/channel_rpc.ipp

include_bitcoin_network_impl_messagesdir = \
    ${includedir}/bitcoin/network/impl/messages

include_bitcoin_network_impl_messages_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/impl/messages/json_body.ipp

include_bitcoin_network_impl_messages_rpcdir = \
    ${includedir}/bitcoin/network/impl/messages/rpc

include_bitcoin_network_impl_messages_rpc_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/impl/messages/rpc/broadcaster.ipp \
    ${srcdir}/../../include/bitcoin/network/impl/messages/rpc/dispatcher.ipp

include_bitcoin_network_impl_protocolsdir = \
    ${includedir}/bitcoin/network/impl/protocols

include_bitcoin_network_impl_protocols_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/impl/protocols/protocol_rpc.ipp

include_bitcoin_network_interfacesdir = \
    ${includedir}/bitcoin/network/interfaces

include_bitcoin_network_interfaces_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/interfaces/http.hpp \
    ${srcdir}/../../include/bitcoin/network/interfaces/interfaces.hpp \
    ${srcdir}/../../include/bitcoin/network/interfaces/peer_broadcast.hpp \
    ${srcdir}/../../include/bitcoin/network/interfaces/peer_dispatch.hpp

include_bitcoin_network_logdir = \
    ${includedir}/bitcoin/network/log

include_bitcoin_network_log_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/log/capture.hpp \
    ${srcdir}/../../include/bitcoin/network/log/levels.hpp \
    ${srcdir}/../../include/bitcoin/network/log/log.hpp \
    ${srcdir}/../../include/bitcoin/network/log/logger.hpp \
    ${srcdir}/../../include/bitcoin/network/log/reporter.hpp \
    ${srcdir}/../../include/bitcoin/network/log/timer.hpp \
    ${srcdir}/../../include/bitcoin/network/log/tracker.hpp

include_bitcoin_network_messagesdir = \
    ${includedir}/bitcoin/network/messages

include_bitcoin_network_messages_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/messages/http_body.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/http_method.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/json_body.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/messages.hpp

include_bitcoin_network_messages_httpdir = \
    ${includedir}/bitcoin/network/messages/http

include_bitcoin_network_messages_http_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/messages/http/fields.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/http/file_body.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/http/http.hpp

include_bitcoin_network_messages_http_enumsdir = \
    ${includedir}/bitcoin/network/messages/http/enums

include_bitcoin_network_messages_http_enums_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/messages/http/enums/magic_numbers.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/http/enums/media_type.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/http/enums/status.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/http/enums/target.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/http/enums/verb.hpp

include_bitcoin_network_messages_peerdir = \
    ${includedir}/bitcoin/network/messages/peer

include_bitcoin_network_messages_peer_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/messages/peer/body.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/heading.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/message.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/peer.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/registry.hpp

include_bitcoin_network_messages_peer_detaildir = \
    ${includedir}/bitcoin/network/messages/peer/detail

include_bitcoin_network_messages_peer_detail_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/address.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/address_item.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/address_v2.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/alert.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/alert_item.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/block.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/bloom_filter_add.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/bloom_filter_clear.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/bloom_filter_load.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/client_filter.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/client_filter_checkpoint.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/client_filter_headers.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/compact_block.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/compact_block_item.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/compact_transactions.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/fee_filter.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/get_address.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/get_blocks.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/get_client_filter_checkpoint.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/get_client_filter_headers.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/get_client_filters.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/get_compact_transactions.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/get_data.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/get_headers.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/headers.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/inventory.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/inventory_item.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/memory_pool.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/merkle_block.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/not_found.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/ping.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/pong.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/reject.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/send_address_v2.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/send_compact.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/send_headers.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/send_transaction_reconciliation.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/transaction.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/version.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/version_acknowledge.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/witness_tx_id_relay.hpp

include_bitcoin_network_messages_peer_enumsdir = \
    ${includedir}/bitcoin/network/messages/peer/enums

include_bitcoin_network_messages_peer_enums_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/messages/peer/enums/identifiers.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/enums/level.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/enums/magic_numbers.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/enums/service.hpp

include_bitcoin_network_messages_rpcdir = \
    ${includedir}/bitcoin/network/messages/rpc

include_bitcoin_network_messages_rpc_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/messages/rpc/any.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/rpc/body.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/rpc/broadcaster.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/rpc/dispatcher.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/rpc/method.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/rpc/model.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/rpc/publish.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/rpc/rpc.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/rpc/size.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/rpc/types.hpp

include_bitcoin_network_messages_rpc_enumsdir = \
    ${includedir}/bitcoin/network/messages/rpc/enums

include_bitcoin_network_messages_rpc_enums_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/messages/rpc/enums/grouping.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/rpc/enums/version.hpp

include_bitcoin_network_netdir = \
    ${includedir}/bitcoin/network/net

include_bitcoin_network_net_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/net/acceptor.hpp \
    ${srcdir}/../../include/bitcoin/network/net/acceptor_sam.hpp \
    ${srcdir}/../../include/bitcoin/network/net/connector.hpp \
    ${srcdir}/../../include/bitcoin/network/net/connector_socks.hpp \
    ${srcdir}/../../include/bitcoin/network/net/deadline.hpp \
    ${srcdir}/../../include/bitcoin/network/net/hosts.hpp \
    ${srcdir}/../../include/bitcoin/network/net/net.hpp \
    ${srcdir}/../../include/bitcoin/network/net/proxy.hpp \
    ${srcdir}/../../include/bitcoin/network/net/socket.hpp

include_bitcoin_network_p2psdir = \
    ${includedir}/bitcoin/network/p2ps

include_bitcoin_network_p2ps_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/p2ps/cipher.hpp \
    ${srcdir}/../../include/bitcoin/network/p2ps/context.hpp \
    ${srcdir}/../../include/bitcoin/network/p2ps/p2ps.hpp \
    ${srcdir}/../../include/bitcoin/network/p2ps/stream.hpp

include_bitcoin_network_protocolsdir = \
    ${includedir}/bitcoin/network/protocols

include_bitcoin_network_protocols_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_address_in_209.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_address_in_70016.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_address_out_209.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_address_out_70016.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_alert_311.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_http.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_peer.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_ping_106.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_ping_60001.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_reject_70002.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_rpc.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_seed_209.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_seed_70016.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_version_106.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_version_70001.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_version_70002.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_version_70014.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_version_70016.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocols.hpp

include_bitcoin_network_sessionsdir = \
    ${includedir}/bitcoin/network/sessions

include_bitcoin_network_sessions_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/sessions/session.hpp \
    ${srcdir}/../../include/bitcoin/network/sessions/session_inbound.hpp \
    ${srcdir}/../../include/bitcoin/network/sessions/session_manual.hpp \
    ${srcdir}/../../include/bitcoin/network/sessions/session_outbound.hpp \
    ${srcdir}/../../include/bitcoin/network/sessions/session_peer.hpp \
    ${srcdir}/../../include/bitcoin/network/sessions/session_seed.hpp \
    ${srcdir}/../../include/bitcoin/network/sessions/session_server.hpp \
    ${srcdir}/../../include/bitcoin/network/sessions/sessions.hpp

if WITH_SSL

include_bitcoin_network_ssl_openssldir = \
    ${includedir}/bitcoin/network/ssl/openssl

include_bitcoin_network_ssl_openssl_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/ssl/openssl/conf.h \
    ${srcdir}/../../include/bitcoin/network/ssl/openssl/dh.h \
    ${srcdir}/../../include/bitcoin/network/ssl/openssl/engine.h \
    ${srcdir}/../../include/bitcoin/network/ssl/openssl/err.h \
    ${srcdir}/../../include/bitcoin/network/ssl/openssl/openssl.h \
    ${srcdir}/../../include/bitcoin/network/ssl/openssl/rsa.h \
    ${srcdir}/../../include/bitcoin/network/ssl/openssl/ssl.h \
    ${srcdir}/../../include/bitcoin/network/ssl/openssl/x509.h \
    ${srcdir}/../../include/bitcoin/network/ssl/openssl/x509v3.h

endif WITH_SSL

include_bitcoin_network_tlsdir = \
    ${includedir}/bitcoin/network/tls

include_bitcoin_network_tls_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/tls/codec.hpp \
    ${srcdir}/../../include/bitcoin/network/tls/constants.hpp \
    ${srcdir}/../../include/bitcoin/network/tls/context.hpp \
    ${srcdir}/../../include/bitcoin/network/tls/record.hpp \
    ${srcdir}/../../include/bitcoin/network/tls/schedule.hpp \
    ${srcdir}/../../include/bitcoin/network/tls/server.hpp \
    ${srcdir}/../../include/bitcoin/network/tls/tls.hpp

include_bitcoin_network_zmtpdir = \
    ${includedir}/bitcoin/network/zmtp

include_bitcoin_network_zmtp_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/zmtp/cipher.hpp \
    ${srcdir}/../../include/bitcoin/network/zmtp/context.hpp \
    ${srcdir}/../../include/bitcoin/network/zmtp/role.hpp \
    ${srcdir}/../../include/bitcoin/network/zmtp/stream.hpp \
    ${srcdir}/../../include/bitcoin/network/zmtp/zmtp.hpp

# Tests.
#==============================================================================
# Target test 'test/libbitcoin-network-test'
#------------------------------------------------------------------------------
if WITH_TESTS

check_PROGRAMS = test/libbitcoin-network-test

test_libbitcoin_network_test_CPPFLAGS = \
    -I${top_builddir}/test \
    -I${srcdir}/../../test \
    ${boost_BUILD_CPPFLAGS} \
    ${src_libbitcoin_network_la_CPPFLAGS}

test_libbitcoin_network_test_LDFLAGS = \
    ${boost_LDFLAGS} \
    ${src_libbitcoin_network_la_LDFLAGS}

test_libbitcoin_network_test_LDADD = \
    ${boost_LIBS} \
    ${src_libbitcoin_network_la_LIBS} \
    ${src_libbitcoin_network_la_LIBADD}

test_libbitcoin_network_test_SOURCES = \
    ${srcdir}/../../test/asio.cpp \
    ${srcdir}/../../test/error.cpp \
    ${srcdir}/../../test/main.cpp \
    ${srcdir}/../../test/memory.cpp \
    ${srcdir}/../../test/net.cpp \
    ${srcdir}/../../test/settings.cpp \
    ${srcdir}/../../test/test.cpp \
    ${srcdir}/../../test/async/desubscriber.cpp \
    ${srcdir}/../../test/async/enable_shared_from_base.cpp \
    ${srcdir}/../../test/async/subscriber.cpp \
    ${srcdir}/../../test/async/thread.cpp \
    ${srcdir}/../../test/async/threadpool.cpp \
    ${srcdir}/../../test/async/time.cpp \
    ${srcdir}/../../test/async/unsubscriber.cpp \
    ${srcdir}/../../test/async/races/race_all.cpp \
    ${srcdir}/../../test/async/races/race_any.cpp \
    ${srcdir}/../../test/async/races/race_quality.cpp \
    ${srcdir}/../../test/async/races/race_speed.cpp \
    ${srcdir}/../../test/async/races/race_unity.cpp \
    ${srcdir}/../../test/async/races/race_volume.cpp \
    ${srcdir}/../../test/broadcast/terminator.cpp \
    ${srcdir}/../../test/channels/channel.cpp \
    ${srcdir}/../../test/channels/channel_http.cpp \
    ${srcdir}/../../test/channels/channel_peer.cpp \
    ${srcdir}/../../test/channels/channel_rpc.cpp \
    ${srcdir}/../../test/config/address.cpp \
    ${srcdir}/../../test/config/address_type.cpp \
    ${srcdir}/../../test/config/authority.cpp \
    ${srcdir}/../../test/config/credential.cpp \
    ${srcdir}/../../test/config/endpoint.cpp \
    ${srcdir}/../../test/config/utilities.cpp \
    ${srcdir}/../../test/functional/peer.cpp \
    ${srcdir}/../../test/functional/peer_setup_fixture.cpp \
    ${srcdir}/../../test/log/timer.cpp \
    ${srcdir}/../../test/log/tracker.cpp \
    ${srcdir}/../../test/messages/http_body_reader.cpp \
    ${srcdir}/../../test/messages/http_body_writer.cpp \
    ${srcdir}/../../test/messages/json_body_reader.cpp \
    ${srcdir}/../../test/messages/json_body_writer.cpp \
    ${srcdir}/../../test/messages/http/fields.cpp \
    ${srcdir}/../../test/messages/http/method.cpp \
    ${srcdir}/../../test/messages/http/enums/magic_numbers.cpp \
    ${srcdir}/../../test/messages/http/enums/media_type.cpp \
    ${srcdir}/../../test/messages/http/enums/target.cpp \
    ${srcdir}/../../test/messages/peer/body.cpp \
    ${srcdir}/../../test/messages/peer/heading.cpp \
    ${srcdir}/../../test/messages/peer/message.cpp \
    ${srcdir}/../../test/messages/peer/detail/address.cpp \
    ${srcdir}/../../test/messages/peer/detail/address_item.cpp \
    ${srcdir}/../../test/messages/peer/detail/address_v2.cpp \
    ${srcdir}/../../test/messages/peer/detail/alert.cpp \
    ${srcdir}/../../test/messages/peer/detail/alert_item.cpp \
    ${srcdir}/../../test/messages/peer/detail/block.cpp \
    ${srcdir}/../../test/messages/peer/detail/bloom_filter_add.cpp \
    ${srcdir}/../../test/messages/peer/detail/bloom_filter_clear.cpp \
    ${srcdir}/../../test/messages/peer/detail/bloom_filter_load.cpp \
    ${srcdir}/../../test/messages/peer/detail/client_filter.cpp \
    ${srcdir}/../../test/messages/peer/detail/client_filter_checkpoint.cpp \
    ${srcdir}/../../test/messages/peer/detail/client_filter_headers.cpp \
    ${srcdir}/../../test/messages/peer/detail/compact_block.cpp \
    ${srcdir}/../../test/messages/peer/detail/compact_block_item.cpp \
    ${srcdir}/../../test/messages/peer/detail/compact_transactions.cpp \
    ${srcdir}/../../test/messages/peer/detail/fee_filter.cpp \
    ${srcdir}/../../test/messages/peer/detail/get_address.cpp \
    ${srcdir}/../../test/messages/peer/detail/get_blocks.cpp \
    ${srcdir}/../../test/messages/peer/detail/get_client_filter_checkpoint.cpp \
    ${srcdir}/../../test/messages/peer/detail/get_client_filter_headers.cpp \
    ${srcdir}/../../test/messages/peer/detail/get_client_filters.cpp \
    ${srcdir}/../../test/messages/peer/detail/get_compact_transactions.cpp \
    ${srcdir}/../../test/messages/peer/detail/get_data.cpp \
    ${srcdir}/../../test/messages/peer/detail/get_headers.cpp \
    ${srcdir}/../../test/messages/peer/detail/headers.cpp \
    ${srcdir}/../../test/messages/peer/detail/inventory.cpp \
    ${srcdir}/../../test/messages/peer/detail/inventory_item.cpp \
    ${srcdir}/../../test/messages/peer/detail/memory_pool.cpp \
    ${srcdir}/../../test/messages/peer/detail/merkle_block.cpp \
    ${srcdir}/../../test/messages/peer/detail/not_found.cpp \
    ${srcdir}/../../test/messages/peer/detail/ping.cpp \
    ${srcdir}/../../test/messages/peer/detail/pong.cpp \
    ${srcdir}/../../test/messages/peer/detail/reject.cpp \
    ${srcdir}/../../test/messages/peer/detail/send_address_v2.cpp \
    ${srcdir}/../../test/messages/peer/detail/send_compact.cpp \
    ${srcdir}/../../test/messages/peer/detail/send_headers.cpp \
    ${srcdir}/../../test/messages/peer/detail/send_transaction_reconciliation.cpp \
    ${srcdir}/../../test/messages/peer/detail/transaction.cpp \
    ${srcdir}/../../test/messages/peer/detail/version.cpp \
    ${srcdir}/../../test/messages/peer/detail/version_acknowledge.cpp \
    ${srcdir}/../../test/messages/peer/detail/witness_tx_id_relay.cpp \
    ${srcdir}/../../test/messages/peer/enums/identifier.cpp \
    ${srcdir}/../../test/messages/peer/enums/level.cpp \
    ${srcdir}/../../test/messages/peer/enums/magic_numbers.cpp \
    ${srcdir}/../../test/messages/peer/enums/service.cpp \
    ${srcdir}/../../test/messages/rpc/any.cpp \
    ${srcdir}/../../test/messages/rpc/body_reader.cpp \
    ${srcdir}/../../test/messages/rpc/body_writer.cpp \
    ${srcdir}/../../test/messages/rpc/broadcaster.cpp \
    ${srcdir}/../../test/messages/rpc/dispatcher.cpp \
    ${srcdir}/../../test/messages/rpc/method.cpp \
    ${srcdir}/../../test/messages/rpc/publish.cpp \
    ${srcdir}/../../test/messages/rpc/size.cpp \
    ${srcdir}/../../test/messages/rpc/types.cpp \
    ${srcdir}/../../test/net/acceptor.cpp \
    ${srcdir}/../../test/net/acceptor_sam.cpp \
    ${srcdir}/../../test/net/connector.cpp \
    ${srcdir}/../../test/net/connector_socks.cpp \
    ${srcdir}/../../test/net/deadline.cpp \
    ${srcdir}/../../test/net/hosts.cpp \
    ${srcdir}/../../test/net/proxy.cpp \
    ${srcdir}/../../test/net/socket.cpp \
    ${srcdir}/../../test/p2ps/cipher.cpp \
    ${srcdir}/../../test/p2ps/stream.cpp \
    ${srcdir}/../../test/protocols/protocol.cpp \
    ${srcdir}/../../test/protocols/protocol_address_in_209.cpp \
    ${srcdir}/../../test/protocols/protocol_address_in_70016.cpp \
    ${srcdir}/../../test/protocols/protocol_address_out_209.cpp \
    ${srcdir}/../../test/protocols/protocol_address_out_70016.cpp \
    ${srcdir}/../../test/protocols/protocol_alert_311.cpp \
    ${srcdir}/../../test/protocols/protocol_http.cpp \
    ${srcdir}/../../test/protocols/protocol_peer.cpp \
    ${srcdir}/../../test/protocols/protocol_ping_106.cpp \
    ${srcdir}/../../test/protocols/protocol_ping_60001.cpp \
    ${srcdir}/../../test/protocols/protocol_reject_70002.cpp \
    ${srcdir}/../../test/protocols/protocol_seed_209.cpp \
    ${srcdir}/../../test/protocols/protocol_seed_70016.cpp \
    ${srcdir}/../../test/protocols/protocol_version_106.cpp \
    ${srcdir}/../../test/protocols/protocol_version_70001.cpp \
    ${srcdir}/../../test/protocols/protocol_version_70002.cpp \
    ${srcdir}/../../test/protocols/protocol_version_70014.cpp \
    ${srcdir}/../../test/protocols/protocol_version_70016.cpp \
    ${srcdir}/../../test/sessions/session.cpp \
    ${srcdir}/../../test/sessions/session_html.cpp \
    ${srcdir}/../../test/sessions/session_inbound.cpp \
    ${srcdir}/../../test/sessions/session_manual.cpp \
    ${srcdir}/../../test/sessions/session_outbound.cpp \
    ${srcdir}/../../test/sessions/session_seed.cpp \
    ${srcdir}/../../test/sessions/session_server.cpp \
    ${srcdir}/../../test/tls/client.cpp \
    ${srcdir}/../../test/tls/codec.cpp \
    ${srcdir}/../../test/tls/context.cpp \
    ${srcdir}/../../test/tls/record.cpp \
    ${srcdir}/../../test/tls/schedule.cpp \
    ${srcdir}/../../test/tls/server.cpp \
    ${srcdir}/../../test/tls/server_hello.cpp \
    ${srcdir}/../../test/tls/socket.cpp \
    ${srcdir}/../../test/zmtp/cipher.cpp \
    ${srcdir}/../../test/zmtp/proxy.cpp \
    ${srcdir}/../../test/zmtp/role.cpp \
    ${srcdir}/../../test/zmtp/socket.cpp \
    ${srcdir}/../../test/zmtp/stream.cpp \
    ${srcdir}/../../test/zmtp/zmtp_setup_fixture.cpp

TESTS = test_runner.sh

endif WITH_TESTS
