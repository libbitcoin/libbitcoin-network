###############################################################################
#  Copyright (c) 2014-2025 libbitcoin-network developers (see COPYING).
#
#         GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
#
###############################################################################

# Automake settings.
#==============================================================================
# Look for macros in the m4 subdirectory.
#------------------------------------------------------------------------------
ACLOCAL_AMFLAGS = -I m4


# Distribute, make and install products.
#==============================================================================
# files => ${pkgconfigdir}
#------------------------------------------------------------------------------
pkgconfig_DATA = \
    libbitcoin-network.pc

# files => ${docdir}
#------------------------------------------------------------------------------
doc_DATA = \
    AUTHORS \
    COPYING \
    ChangeLog \
    INSTALL \
    NEWS \
    README

# src/libbitcoin-network.la => ${libdir}
#------------------------------------------------------------------------------
lib_LTLIBRARIES = src/libbitcoin-network.la
src_libbitcoin_network_la_CPPFLAGS = -I${srcdir}/include ${bitcoin_system_BUILD_CPPFLAGS}
src_libbitcoin_network_la_LIBADD = ${bitcoin_system_LIBS}
src_libbitcoin_network_la_SOURCES = \
    src/error.cpp \
    src/memory.cpp \
    src/net.cpp \
    src/settings.cpp \
    src/async/thread.cpp \
    src/async/threadpool.cpp \
    src/async/time.cpp \
    src/channels/channel.cpp \
    src/channels/channel_http.cpp \
    src/channels/channel_peer.cpp \
    src/channels/channel_tcp.cpp \
    src/channels/channel_ws.cpp \
    src/config/address.cpp \
    src/config/authority.cpp \
    src/config/endpoint.cpp \
    src/config/utilities.cpp \
    src/log/capture.cpp \
    src/log/logger.cpp \
    src/log/reporter.cpp \
    src/messages/http/fields.cpp \
    src/messages/http/enums/mime_type.cpp \
    src/messages/http/enums/target.cpp \
    src/messages/json/body.cpp \
    src/messages/peer/message.cpp \
    src/messages/peer/detail/address.cpp \
    src/messages/peer/detail/address_item.cpp \
    src/messages/peer/detail/alert.cpp \
    src/messages/peer/detail/alert_item.cpp \
    src/messages/peer/detail/block.cpp \
    src/messages/peer/detail/bloom_filter_add.cpp \
    src/messages/peer/detail/bloom_filter_clear.cpp \
    src/messages/peer/detail/bloom_filter_load.cpp \
    src/messages/peer/detail/client_filter.cpp \
    src/messages/peer/detail/client_filter_checkpoint.cpp \
    src/messages/peer/detail/client_filter_headers.cpp \
    src/messages/peer/detail/compact_block.cpp \
    src/messages/peer/detail/compact_block_item.cpp \
    src/messages/peer/detail/compact_transactions.cpp \
    src/messages/peer/detail/fee_filter.cpp \
    src/messages/peer/detail/get_address.cpp \
    src/messages/peer/detail/get_blocks.cpp \
    src/messages/peer/detail/get_client_filter_checkpoint.cpp \
    src/messages/peer/detail/get_client_filter_headers.cpp \
    src/messages/peer/detail/get_client_filters.cpp \
    src/messages/peer/detail/get_compact_transactions.cpp \
    src/messages/peer/detail/get_data.cpp \
    src/messages/peer/detail/get_headers.cpp \
    src/messages/peer/detail/headers.cpp \
    src/messages/peer/detail/heading.cpp \
    src/messages/peer/detail/inventory.cpp \
    src/messages/peer/detail/inventory_item.cpp \
    src/messages/peer/detail/memory_pool.cpp \
    src/messages/peer/detail/merkle_block.cpp \
    src/messages/peer/detail/not_found.cpp \
    src/messages/peer/detail/ping.cpp \
    src/messages/peer/detail/pong.cpp \
    src/messages/peer/detail/reject.cpp \
    src/messages/peer/detail/send_address_v2.cpp \
    src/messages/peer/detail/send_compact.cpp \
    src/messages/peer/detail/send_headers.cpp \
    src/messages/peer/detail/transaction.cpp \
    src/messages/peer/detail/version.cpp \
    src/messages/peer/detail/version_acknowledge.cpp \
    src/messages/peer/detail/witness_tx_id_relay.cpp \
    src/messages/variant/body.cpp \
    src/messages/variant/head.cpp \
    src/net/acceptor.cpp \
    src/net/broadcaster.cpp \
    src/net/connector.cpp \
    src/net/deadline.cpp \
    src/net/hosts.cpp \
    src/net/proxy.cpp \
    src/net/socket.cpp \
    src/protocols/protocol.cpp \
    src/protocols/protocol_address_in_209.cpp \
    src/protocols/protocol_address_out_209.cpp \
    src/protocols/protocol_alert_311.cpp \
    src/protocols/protocol_http.cpp \
    src/protocols/protocol_peer.cpp \
    src/protocols/protocol_ping_106.cpp \
    src/protocols/protocol_ping_60001.cpp \
    src/protocols/protocol_reject_70002.cpp \
    src/protocols/protocol_seed_209.cpp \
    src/protocols/protocol_version_106.cpp \
    src/protocols/protocol_version_70001.cpp \
    src/protocols/protocol_version_70002.cpp \
    src/protocols/protocol_version_70016.cpp \
    src/rpc/model.cpp \
    src/sessions/session.cpp \
    src/sessions/session_inbound.cpp \
    src/sessions/session_manual.cpp \
    src/sessions/session_outbound.cpp \
    src/sessions/session_peer.cpp \
    src/sessions/session_seed.cpp \
    src/sessions/session_tcp.cpp

# local: test/libbitcoin-network-test
#------------------------------------------------------------------------------
if WITH_TESTS

TESTS = libbitcoin-network-test_runner.sh

check_PROGRAMS = test/libbitcoin-network-test
test_libbitcoin_network_test_CPPFLAGS = -I${srcdir}/include ${bitcoin_system_BUILD_CPPFLAGS}
test_libbitcoin_network_test_LDADD = src/libbitcoin-network.la ${boost_unit_test_framework_LIBS} ${bitcoin_system_LIBS}
test_libbitcoin_network_test_SOURCES = \
    test/error.cpp \
    test/main.cpp \
    test/memory.cpp \
    test/net.cpp \
    test/settings.cpp \
    test/test.cpp \
    test/test.hpp \
    test/async/desubscriber.cpp \
    test/async/enable_shared_from_base.cpp \
    test/async/subscriber.cpp \
    test/async/thread.cpp \
    test/async/threadpool.cpp \
    test/async/time.cpp \
    test/async/unsubscriber.cpp \
    test/async/races/race_all.cpp \
    test/async/races/race_quality.cpp \
    test/async/races/race_speed.cpp \
    test/async/races/race_unity.cpp \
    test/async/races/race_volume.cpp \
    test/channels/channel.cpp \
    test/channels/channel_http.cpp \
    test/channels/channel_peer.cpp \
    test/channels/channel_tcp.cpp \
    test/channels/channel_ws.cpp \
    test/config/address.cpp \
    test/config/authority.cpp \
    test/config/endpoint.cpp \
    test/config/utilities.cpp \
    test/log/timer.cpp \
    test/log/tracker.cpp \
    test/messages/http/fields.cpp \
    test/messages/http/head_reader.cpp \
    test/messages/http/head_writer.cpp \
    test/messages/http/enums/magic_numbers.cpp \
    test/messages/http/enums/mime_type.cpp \
    test/messages/http/enums/target.cpp \
    test/messages/json/body_reader.cpp \
    test/messages/json/body_writer.cpp \
    test/messages/peer/heading.cpp \
    test/messages/peer/message.cpp \
    test/messages/peer/detail/address.cpp \
    test/messages/peer/detail/address_item.cpp \
    test/messages/peer/detail/alert.cpp \
    test/messages/peer/detail/alert_item.cpp \
    test/messages/peer/detail/block.cpp \
    test/messages/peer/detail/bloom_filter_add.cpp \
    test/messages/peer/detail/bloom_filter_clear.cpp \
    test/messages/peer/detail/bloom_filter_load.cpp \
    test/messages/peer/detail/client_filter.cpp \
    test/messages/peer/detail/client_filter_checkpoint.cpp \
    test/messages/peer/detail/client_filter_headers.cpp \
    test/messages/peer/detail/compact_block.cpp \
    test/messages/peer/detail/compact_block_item.cpp \
    test/messages/peer/detail/compact_transactions.cpp \
    test/messages/peer/detail/fee_filter.cpp \
    test/messages/peer/detail/get_address.cpp \
    test/messages/peer/detail/get_blocks.cpp \
    test/messages/peer/detail/get_client_filter_checkpoint.cpp \
    test/messages/peer/detail/get_client_filter_headers.cpp \
    test/messages/peer/detail/get_client_filters.cpp \
    test/messages/peer/detail/get_compact_transactions.cpp \
    test/messages/peer/detail/get_data.cpp \
    test/messages/peer/detail/get_headers.cpp \
    test/messages/peer/detail/headers.cpp \
    test/messages/peer/detail/inventory.cpp \
    test/messages/peer/detail/inventory_item.cpp \
    test/messages/peer/detail/memory_pool.cpp \
    test/messages/peer/detail/merkle_block.cpp \
    test/messages/peer/detail/not_found.cpp \
    test/messages/peer/detail/ping.cpp \
    test/messages/peer/detail/pong.cpp \
    test/messages/peer/detail/reject.cpp \
    test/messages/peer/detail/send_address_v2.cpp \
    test/messages/peer/detail/send_compact.cpp \
    test/messages/peer/detail/send_headers.cpp \
    test/messages/peer/detail/transaction.cpp \
    test/messages/peer/detail/version.cpp \
    test/messages/peer/detail/version_acknowledge.cpp \
    test/messages/peer/detail/witness_tx_id_relay.cpp \
    test/messages/peer/enums/identifier.cpp \
    test/messages/peer/enums/level.cpp \
    test/messages/peer/enums/magic_numbers.cpp \
    test/messages/peer/enums/service.cpp \
    test/messages/variant/body_reader.cpp \
    test/messages/variant/body_writer.cpp \
    test/messages/variant/head_reader.cpp \
    test/messages/variant/head_writer.cpp \
    test/messages/variant/method.cpp \
    test/net/acceptor.cpp \
    test/net/broadcaster.cpp \
    test/net/connector.cpp \
    test/net/deadline.cpp \
    test/net/hosts.cpp \
    test/net/proxy.cpp \
    test/net/socket.cpp \
    test/protocols/protocol.cpp \
    test/protocols/protocol_address_in_209.cpp \
    test/protocols/protocol_address_out_209.cpp \
    test/protocols/protocol_alert_311.cpp \
    test/protocols/protocol_http.cpp \
    test/protocols/protocol_peer.cpp \
    test/protocols/protocol_ping_106.cpp \
    test/protocols/protocol_ping_60001.cpp \
    test/protocols/protocol_reject_70002.cpp \
    test/protocols/protocol_seed_209.cpp \
    test/protocols/protocol_tcp.cpp \
    test/protocols/protocol_version_106.cpp \
    test/protocols/protocol_version_70001.cpp \
    test/protocols/protocol_version_70002.cpp \
    test/protocols/protocol_version_70016.cpp \
    test/rpc/any.cpp \
    test/rpc/dispatcher.cpp \
    test/rpc/method.cpp \
    test/rpc/types.cpp \
    test/sessions/session.cpp \
    test/sessions/session_html.cpp \
    test/sessions/session_inbound.cpp \
    test/sessions/session_manual.cpp \
    test/sessions/session_outbound.cpp \
    test/sessions/session_seed.cpp \
    test/sessions/session_tcp.cpp

endif WITH_TESTS

# files => ${includedir}/bitcoin
#------------------------------------------------------------------------------
include_bitcoindir = ${includedir}/bitcoin
include_bitcoin_HEADERS = \
    include/bitcoin/network.hpp

include_bitcoin_networkdir = ${includedir}/bitcoin/network
include_bitcoin_network_HEADERS = \
    include/bitcoin/network/beast.hpp \
    include/bitcoin/network/boost.hpp \
    include/bitcoin/network/define.hpp \
    include/bitcoin/network/error.hpp \
    include/bitcoin/network/memory.hpp \
    include/bitcoin/network/net.hpp \
    include/bitcoin/network/settings.hpp \
    include/bitcoin/network/version.hpp

include_bitcoin_network_asyncdir = ${includedir}/bitcoin/network/async
include_bitcoin_network_async_HEADERS = \
    include/bitcoin/network/async/asio.hpp \
    include/bitcoin/network/async/async.hpp \
    include/bitcoin/network/async/desubscriber.hpp \
    include/bitcoin/network/async/enable_shared_from_base.hpp \
    include/bitcoin/network/async/handlers.hpp \
    include/bitcoin/network/async/subscriber.hpp \
    include/bitcoin/network/async/thread.hpp \
    include/bitcoin/network/async/threadpool.hpp \
    include/bitcoin/network/async/time.hpp \
    include/bitcoin/network/async/unsubscriber.hpp

include_bitcoin_network_async_racesdir = ${includedir}/bitcoin/network/async/races
include_bitcoin_network_async_races_HEADERS = \
    include/bitcoin/network/async/races/race_all.hpp \
    include/bitcoin/network/async/races/race_quality.hpp \
    include/bitcoin/network/async/races/race_speed.hpp \
    include/bitcoin/network/async/races/race_unity.hpp \
    include/bitcoin/network/async/races/race_volume.hpp \
    include/bitcoin/network/async/races/races.hpp

include_bitcoin_network_channelsdir = ${includedir}/bitcoin/network/channels
include_bitcoin_network_channels_HEADERS = \
    include/bitcoin/network/channels/channel.hpp \
    include/bitcoin/network/channels/channel_http.hpp \
    include/bitcoin/network/channels/channel_peer.hpp \
    include/bitcoin/network/channels/channel_tcp.hpp \
    include/bitcoin/network/channels/channel_ws.hpp \
    include/bitcoin/network/channels/channels.hpp

include_bitcoin_network_configdir = ${includedir}/bitcoin/network/config
include_bitcoin_network_config_HEADERS = \
    include/bitcoin/network/config/address.hpp \
    include/bitcoin/network/config/authority.hpp \
    include/bitcoin/network/config/config.hpp \
    include/bitcoin/network/config/endpoint.hpp \
    include/bitcoin/network/config/utilities.hpp

include_bitcoin_network_impl_asyncdir = ${includedir}/bitcoin/network/impl/async
include_bitcoin_network_impl_async_HEADERS = \
    include/bitcoin/network/impl/async/desubscriber.ipp \
    include/bitcoin/network/impl/async/enable_shared_from_base.ipp \
    include/bitcoin/network/impl/async/subscriber.ipp \
    include/bitcoin/network/impl/async/unsubscriber.ipp

include_bitcoin_network_impl_async_racesdir = ${includedir}/bitcoin/network/impl/async/races
include_bitcoin_network_impl_async_races_HEADERS = \
    include/bitcoin/network/impl/async/races/race_all.ipp \
    include/bitcoin/network/impl/async/races/race_quality.ipp \
    include/bitcoin/network/impl/async/races/race_speed.ipp \
    include/bitcoin/network/impl/async/races/race_unity.ipp \
    include/bitcoin/network/impl/async/races/race_volume.ipp

include_bitcoin_network_impl_messages_httpdir = ${includedir}/bitcoin/network/impl/messages/http
include_bitcoin_network_impl_messages_http_HEADERS = \
    include/bitcoin/network/impl/messages/http/head.ipp

include_bitcoin_network_impl_rpcdir = ${includedir}/bitcoin/network/impl/rpc
include_bitcoin_network_impl_rpc_HEADERS = \
    include/bitcoin/network/impl/rpc/dispatcher.ipp

include_bitcoin_network_logdir = ${includedir}/bitcoin/network/log
include_bitcoin_network_log_HEADERS = \
    include/bitcoin/network/log/capture.hpp \
    include/bitcoin/network/log/levels.hpp \
    include/bitcoin/network/log/log.hpp \
    include/bitcoin/network/log/logger.hpp \
    include/bitcoin/network/log/reporter.hpp \
    include/bitcoin/network/log/timer.hpp \
    include/bitcoin/network/log/tracker.hpp

include_bitcoin_network_messages_httpdir = ${includedir}/bitcoin/network/messages/http
include_bitcoin_network_messages_http_HEADERS = \
    include/bitcoin/network/messages/http/fields.hpp \
    include/bitcoin/network/messages/http/head.hpp \
    include/bitcoin/network/messages/http/http.hpp

include_bitcoin_network_messages_http_enumsdir = ${includedir}/bitcoin/network/messages/http/enums
include_bitcoin_network_messages_http_enums_HEADERS = \
    include/bitcoin/network/messages/http/enums/magic_numbers.hpp \
    include/bitcoin/network/messages/http/enums/mime_type.hpp \
    include/bitcoin/network/messages/http/enums/status.hpp \
    include/bitcoin/network/messages/http/enums/target.hpp \
    include/bitcoin/network/messages/http/enums/verb.hpp

include_bitcoin_network_messages_jsondir = ${includedir}/bitcoin/network/messages/json
include_bitcoin_network_messages_json_HEADERS = \
    include/bitcoin/network/messages/json/body.hpp \
    include/bitcoin/network/messages/json/json.hpp

include_bitcoin_network_messages_peerdir = ${includedir}/bitcoin/network/messages/peer
include_bitcoin_network_messages_peer_HEADERS = \
    include/bitcoin/network/messages/peer/heading.hpp \
    include/bitcoin/network/messages/peer/message.hpp \
    include/bitcoin/network/messages/peer/peer.hpp

include_bitcoin_network_messages_peer_detaildir = ${includedir}/bitcoin/network/messages/peer/detail
include_bitcoin_network_messages_peer_detail_HEADERS = \
    include/bitcoin/network/messages/peer/detail/address.hpp \
    include/bitcoin/network/messages/peer/detail/address_item.hpp \
    include/bitcoin/network/messages/peer/detail/alert.hpp \
    include/bitcoin/network/messages/peer/detail/alert_item.hpp \
    include/bitcoin/network/messages/peer/detail/block.hpp \
    include/bitcoin/network/messages/peer/detail/bloom_filter_add.hpp \
    include/bitcoin/network/messages/peer/detail/bloom_filter_clear.hpp \
    include/bitcoin/network/messages/peer/detail/bloom_filter_load.hpp \
    include/bitcoin/network/messages/peer/detail/client_filter.hpp \
    include/bitcoin/network/messages/peer/detail/client_filter_checkpoint.hpp \
    include/bitcoin/network/messages/peer/detail/client_filter_headers.hpp \
    include/bitcoin/network/messages/peer/detail/compact_block.hpp \
    include/bitcoin/network/messages/peer/detail/compact_block_item.hpp \
    include/bitcoin/network/messages/peer/detail/compact_transactions.hpp \
    include/bitcoin/network/messages/peer/detail/fee_filter.hpp \
    include/bitcoin/network/messages/peer/detail/get_address.hpp \
    include/bitcoin/network/messages/peer/detail/get_blocks.hpp \
    include/bitcoin/network/messages/peer/detail/get_client_filter_checkpoint.hpp \
    include/bitcoin/network/messages/peer/detail/get_client_filter_headers.hpp \
    include/bitcoin/network/messages/peer/detail/get_client_filters.hpp \
    include/bitcoin/network/messages/peer/detail/get_compact_transactions.hpp \
    include/bitcoin/network/messages/peer/detail/get_data.hpp \
    include/bitcoin/network/messages/peer/detail/get_headers.hpp \
    include/bitcoin/network/messages/peer/detail/headers.hpp \
    include/bitcoin/network/messages/peer/detail/inventory.hpp \
    include/bitcoin/network/messages/peer/detail/inventory_item.hpp \
    include/bitcoin/network/messages/peer/detail/memory_pool.hpp \
    include/bitcoin/network/messages/peer/detail/merkle_block.hpp \
    include/bitcoin/network/messages/peer/detail/not_found.hpp \
    include/bitcoin/network/messages/peer/detail/ping.hpp \
    include/bitcoin/network/messages/peer/detail/pong.hpp \
    include/bitcoin/network/messages/peer/detail/reject.hpp \
    include/bitcoin/network/messages/peer/detail/send_address_v2.hpp \
    include/bitcoin/network/messages/peer/detail/send_compact.hpp \
    include/bitcoin/network/messages/peer/detail/send_headers.hpp \
    include/bitcoin/network/messages/peer/detail/transaction.hpp \
    include/bitcoin/network/messages/peer/detail/version.hpp \
    include/bitcoin/network/messages/peer/detail/version_acknowledge.hpp \
    include/bitcoin/network/messages/peer/detail/witness_tx_id_relay.hpp

include_bitcoin_network_messages_peer_enumsdir = ${includedir}/bitcoin/network/messages/peer/enums
include_bitcoin_network_messages_peer_enums_HEADERS = \
    include/bitcoin/network/messages/peer/enums/identifier.hpp \
    include/bitcoin/network/messages/peer/enums/level.hpp \
    include/bitcoin/network/messages/peer/enums/magic_numbers.hpp \
    include/bitcoin/network/messages/peer/enums/service.hpp

include_bitcoin_network_messages_variantdir = ${includedir}/bitcoin/network/messages/variant
include_bitcoin_network_messages_variant_HEADERS = \
    include/bitcoin/network/messages/variant/body.hpp \
    include/bitcoin/network/messages/variant/head.hpp \
    include/bitcoin/network/messages/variant/method.hpp \
    include/bitcoin/network/messages/variant/request.hpp \
    include/bitcoin/network/messages/variant/response.hpp \
    include/bitcoin/network/messages/variant/variant.hpp

include_bitcoin_network_netdir = ${includedir}/bitcoin/network/net
include_bitcoin_network_net_HEADERS = \
    include/bitcoin/network/net/acceptor.hpp \
    include/bitcoin/network/net/broadcaster.hpp \
    include/bitcoin/network/net/connector.hpp \
    include/bitcoin/network/net/deadline.hpp \
    include/bitcoin/network/net/hosts.hpp \
    include/bitcoin/network/net/net.hpp \
    include/bitcoin/network/net/proxy.hpp \
    include/bitcoin/network/net/socket.hpp

include_bitcoin_network_protocolsdir = ${includedir}/bitcoin/network/protocols
include_bitcoin_network_protocols_HEADERS = \
    include/bitcoin/network/protocols/protocol.hpp \
    include/bitcoin/network/protocols/protocol_address_in_209.hpp \
    include/bitcoin/network/protocols/protocol_address_out_209.hpp \
    include/bitcoin/network/protocols/protocol_alert_311.hpp \
    include/bitcoin/network/protocols/protocol_http.hpp \
    include/bitcoin/network/protocols/protocol_peer.hpp \
    include/bitcoin/network/protocols/protocol_ping_106.hpp \
    include/bitcoin/network/protocols/protocol_ping_60001.hpp \
    include/bitcoin/network/protocols/protocol_reject_70002.hpp \
    include/bitcoin/network/protocols/protocol_seed_209.hpp \
    include/bitcoin/network/protocols/protocol_tcp.hpp \
    include/bitcoin/network/protocols/protocol_version_106.hpp \
    include/bitcoin/network/protocols/protocol_version_70001.hpp \
    include/bitcoin/network/protocols/protocol_version_70002.hpp \
    include/bitcoin/network/protocols/protocol_version_70016.hpp \
    include/bitcoin/network/protocols/protocol_ws.hpp \
    include/bitcoin/network/protocols/protocols.hpp

include_bitcoin_network_rpcdir = ${includedir}/bitcoin/network/rpc
include_bitcoin_network_rpc_HEADERS = \
    include/bitcoin/network/rpc/any.hpp \
    include/bitcoin/network/rpc/dispatcher.hpp \
    include/bitcoin/network/rpc/method.hpp \
    include/bitcoin/network/rpc/model.hpp \
    include/bitcoin/network/rpc/publish.hpp \
    include/bitcoin/network/rpc/rpc.hpp \
    include/bitcoin/network/rpc/types.hpp

include_bitcoin_network_rpc_enumsdir = ${includedir}/bitcoin/network/rpc/enums
include_bitcoin_network_rpc_enums_HEADERS = \
    include/bitcoin/network/rpc/enums/grouping.hpp \
    include/bitcoin/network/rpc/enums/version.hpp

include_bitcoin_network_rpc_interfacesdir = ${includedir}/bitcoin/network/rpc/interfaces
include_bitcoin_network_rpc_interfaces_HEADERS = \
    include/bitcoin/network/rpc/interfaces/bitcoind.hpp \
    include/bitcoin/network/rpc/interfaces/electrum.hpp \
    include/bitcoin/network/rpc/interfaces/explore.hpp \
    include/bitcoin/network/rpc/interfaces/http.hpp \
    include/bitcoin/network/rpc/interfaces/interfaces.hpp \
    include/bitcoin/network/rpc/interfaces/peer.hpp \
    include/bitcoin/network/rpc/interfaces/stratum_v1.hpp \
    include/bitcoin/network/rpc/interfaces/stratum_v2.hpp

include_bitcoin_network_sessionsdir = ${includedir}/bitcoin/network/sessions
include_bitcoin_network_sessions_HEADERS = \
    include/bitcoin/network/sessions/session.hpp \
    include/bitcoin/network/sessions/session_inbound.hpp \
    include/bitcoin/network/sessions/session_manual.hpp \
    include/bitcoin/network/sessions/session_outbound.hpp \
    include/bitcoin/network/sessions/session_peer.hpp \
    include/bitcoin/network/sessions/session_seed.hpp \
    include/bitcoin/network/sessions/session_tcp.hpp \
    include/bitcoin/network/sessions/sessions.hpp

