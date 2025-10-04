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
    src/client.cpp \
    src/error.cpp \
    src/memory.cpp \
    src/net.cpp \
    src/settings.cpp \
    src/async/thread.cpp \
    src/async/threadpool.cpp \
    src/async/time.cpp \
    src/config/address.cpp \
    src/config/authority.cpp \
    src/config/endpoint.cpp \
    src/config/utilities.cpp \
    src/log/capture.cpp \
    src/log/logger.cpp \
    src/log/reporter.cpp \
    src/messages/p2p/address.cpp \
    src/messages/p2p/address_item.cpp \
    src/messages/p2p/alert.cpp \
    src/messages/p2p/alert_item.cpp \
    src/messages/p2p/block.cpp \
    src/messages/p2p/bloom_filter_add.cpp \
    src/messages/p2p/bloom_filter_clear.cpp \
    src/messages/p2p/bloom_filter_load.cpp \
    src/messages/p2p/client_filter.cpp \
    src/messages/p2p/client_filter_checkpoint.cpp \
    src/messages/p2p/client_filter_headers.cpp \
    src/messages/p2p/compact_block.cpp \
    src/messages/p2p/compact_block_item.cpp \
    src/messages/p2p/compact_transactions.cpp \
    src/messages/p2p/fee_filter.cpp \
    src/messages/p2p/get_address.cpp \
    src/messages/p2p/get_blocks.cpp \
    src/messages/p2p/get_client_filter_checkpoint.cpp \
    src/messages/p2p/get_client_filter_headers.cpp \
    src/messages/p2p/get_client_filters.cpp \
    src/messages/p2p/get_compact_transactions.cpp \
    src/messages/p2p/get_data.cpp \
    src/messages/p2p/get_headers.cpp \
    src/messages/p2p/headers.cpp \
    src/messages/p2p/heading.cpp \
    src/messages/p2p/inventory.cpp \
    src/messages/p2p/inventory_item.cpp \
    src/messages/p2p/memory_pool.cpp \
    src/messages/p2p/merkle_block.cpp \
    src/messages/p2p/message.cpp \
    src/messages/p2p/not_found.cpp \
    src/messages/p2p/ping.cpp \
    src/messages/p2p/pong.cpp \
    src/messages/p2p/reject.cpp \
    src/messages/p2p/send_address_v2.cpp \
    src/messages/p2p/send_compact.cpp \
    src/messages/p2p/send_headers.cpp \
    src/messages/p2p/transaction.cpp \
    src/messages/p2p/version.cpp \
    src/messages/p2p/version_acknowledge.cpp \
    src/messages/p2p/witness_tx_id_relay.cpp \
    src/messages/rpc/enums/target.cpp \
    src/net/acceptor.cpp \
    src/net/broadcaster.cpp \
    src/net/channel.cpp \
    src/net/channel_client.cpp \
    src/net/channel_peer.cpp \
    src/net/connector.cpp \
    src/net/deadline.cpp \
    src/net/distributor_client.cpp \
    src/net/distributor_peer.cpp \
    src/net/hosts.cpp \
    src/net/proxy.cpp \
    src/net/socket.cpp \
    src/protocols/protocol.cpp \
    src/protocols/protocol_address_in_209.cpp \
    src/protocols/protocol_address_out_209.cpp \
    src/protocols/protocol_alert_311.cpp \
    src/protocols/protocol_client.cpp \
    src/protocols/protocol_peer.cpp \
    src/protocols/protocol_ping_106.cpp \
    src/protocols/protocol_ping_60001.cpp \
    src/protocols/protocol_reject_70002.cpp \
    src/protocols/protocol_seed_209.cpp \
    src/protocols/protocol_version_106.cpp \
    src/protocols/protocol_version_70001.cpp \
    src/protocols/protocol_version_70002.cpp \
    src/protocols/protocol_version_70016.cpp \
    src/sessions/session.cpp \
    src/sessions/session_client.cpp \
    src/sessions/session_inbound.cpp \
    src/sessions/session_inbound_client.cpp \
    src/sessions/session_manual.cpp \
    src/sessions/session_outbound.cpp \
    src/sessions/session_peer.cpp \
    src/sessions/session_seed.cpp

# local: test/libbitcoin-network-test
#------------------------------------------------------------------------------
if WITH_TESTS

TESTS = libbitcoin-network-test_runner.sh

check_PROGRAMS = test/libbitcoin-network-test
test_libbitcoin_network_test_CPPFLAGS = -I${srcdir}/include ${bitcoin_system_BUILD_CPPFLAGS}
test_libbitcoin_network_test_LDADD = src/libbitcoin-network.la ${boost_unit_test_framework_LIBS} ${bitcoin_system_LIBS}
test_libbitcoin_network_test_SOURCES = \
    test/client.cpp \
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
    test/async/unsubscriber.cpp \
    test/async/races/race_all.cpp \
    test/async/races/race_quality.cpp \
    test/async/races/race_speed.cpp \
    test/async/races/race_unity.cpp \
    test/async/races/race_volume.cpp \
    test/config/address.cpp \
    test/config/authority.cpp \
    test/config/endpoint.cpp \
    test/config/utilities.cpp \
    test/log/timer.cpp \
    test/log/tracker.cpp \
    test/messages/p2p/address.cpp \
    test/messages/p2p/address_item.cpp \
    test/messages/p2p/alert.cpp \
    test/messages/p2p/alert_item.cpp \
    test/messages/p2p/block.cpp \
    test/messages/p2p/bloom_filter_add.cpp \
    test/messages/p2p/bloom_filter_clear.cpp \
    test/messages/p2p/bloom_filter_load.cpp \
    test/messages/p2p/client_filter.cpp \
    test/messages/p2p/client_filter_checkpoint.cpp \
    test/messages/p2p/client_filter_headers.cpp \
    test/messages/p2p/compact_block.cpp \
    test/messages/p2p/compact_block_item.cpp \
    test/messages/p2p/compact_transactions.cpp \
    test/messages/p2p/fee_filter.cpp \
    test/messages/p2p/get_address.cpp \
    test/messages/p2p/get_blocks.cpp \
    test/messages/p2p/get_client_filter_checkpoint.cpp \
    test/messages/p2p/get_client_filter_headers.cpp \
    test/messages/p2p/get_client_filters.cpp \
    test/messages/p2p/get_compact_transactions.cpp \
    test/messages/p2p/get_data.cpp \
    test/messages/p2p/get_headers.cpp \
    test/messages/p2p/headers.cpp \
    test/messages/p2p/heading.cpp \
    test/messages/p2p/inventory.cpp \
    test/messages/p2p/inventory_item.cpp \
    test/messages/p2p/memory_pool.cpp \
    test/messages/p2p/merkle_block.cpp \
    test/messages/p2p/message.cpp \
    test/messages/p2p/not_found.cpp \
    test/messages/p2p/ping.cpp \
    test/messages/p2p/pong.cpp \
    test/messages/p2p/reject.cpp \
    test/messages/p2p/send_address_v2.cpp \
    test/messages/p2p/send_compact.cpp \
    test/messages/p2p/send_headers.cpp \
    test/messages/p2p/transaction.cpp \
    test/messages/p2p/version.cpp \
    test/messages/p2p/version_acknowledge.cpp \
    test/messages/p2p/witness_tx_id_relay.cpp \
    test/messages/p2p/enums/identifier.cpp \
    test/messages/p2p/enums/level.cpp \
    test/messages/p2p/enums/magic_numbers.cpp \
    test/messages/p2p/enums/service.cpp \
    test/messages/rpc/method.cpp \
    test/messages/rpc/enums/magic_numbers.cpp \
    test/messages/rpc/enums/target.cpp \
    test/net/acceptor.cpp \
    test/net/broadcaster.cpp \
    test/net/channel.cpp \
    test/net/channel_client.cpp \
    test/net/channel_peer.cpp \
    test/net/connector.cpp \
    test/net/deadline.cpp \
    test/net/distributor_client.cpp \
    test/net/distributor_peer.cpp \
    test/net/hosts.cpp \
    test/net/proxy.cpp \
    test/net/socket.cpp \
    test/protocols/protocol.cpp \
    test/protocols/protocol_address_in_209.cpp \
    test/protocols/protocol_address_out_209.cpp \
    test/protocols/protocol_alert_311.cpp \
    test/protocols/protocol_ping_106.cpp \
    test/protocols/protocol_ping_60001.cpp \
    test/protocols/protocol_reject_70002.cpp \
    test/protocols/protocol_seed_209.cpp \
    test/protocols/protocol_version_106.cpp \
    test/protocols/protocol_version_70001.cpp \
    test/protocols/protocol_version_70002.cpp \
    test/protocols/protocol_version_70016.cpp \
    test/sessions/session.cpp \
    test/sessions/session_inbound.cpp \
    test/sessions/session_manual.cpp \
    test/sessions/session_outbound.cpp \
    test/sessions/session_seed.cpp

endif WITH_TESTS

# files => ${includedir}/bitcoin
#------------------------------------------------------------------------------
include_bitcoindir = ${includedir}/bitcoin
include_bitcoin_HEADERS = \
    include/bitcoin/network.hpp

include_bitcoin_networkdir = ${includedir}/bitcoin/network
include_bitcoin_network_HEADERS = \
    include/bitcoin/network/boost.hpp \
    include/bitcoin/network/client.hpp \
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

include_bitcoin_network_logdir = ${includedir}/bitcoin/network/log
include_bitcoin_network_log_HEADERS = \
    include/bitcoin/network/log/capture.hpp \
    include/bitcoin/network/log/levels.hpp \
    include/bitcoin/network/log/log.hpp \
    include/bitcoin/network/log/logger.hpp \
    include/bitcoin/network/log/reporter.hpp \
    include/bitcoin/network/log/timer.hpp \
    include/bitcoin/network/log/tracker.hpp

include_bitcoin_network_messages_p2pdir = ${includedir}/bitcoin/network/messages/p2p
include_bitcoin_network_messages_p2p_HEADERS = \
    include/bitcoin/network/messages/p2p/address.hpp \
    include/bitcoin/network/messages/p2p/address_item.hpp \
    include/bitcoin/network/messages/p2p/alert.hpp \
    include/bitcoin/network/messages/p2p/alert_item.hpp \
    include/bitcoin/network/messages/p2p/block.hpp \
    include/bitcoin/network/messages/p2p/bloom_filter_add.hpp \
    include/bitcoin/network/messages/p2p/bloom_filter_clear.hpp \
    include/bitcoin/network/messages/p2p/bloom_filter_load.hpp \
    include/bitcoin/network/messages/p2p/client_filter.hpp \
    include/bitcoin/network/messages/p2p/client_filter_checkpoint.hpp \
    include/bitcoin/network/messages/p2p/client_filter_headers.hpp \
    include/bitcoin/network/messages/p2p/compact_block.hpp \
    include/bitcoin/network/messages/p2p/compact_block_item.hpp \
    include/bitcoin/network/messages/p2p/compact_transactions.hpp \
    include/bitcoin/network/messages/p2p/fee_filter.hpp \
    include/bitcoin/network/messages/p2p/get_address.hpp \
    include/bitcoin/network/messages/p2p/get_blocks.hpp \
    include/bitcoin/network/messages/p2p/get_client_filter_checkpoint.hpp \
    include/bitcoin/network/messages/p2p/get_client_filter_headers.hpp \
    include/bitcoin/network/messages/p2p/get_client_filters.hpp \
    include/bitcoin/network/messages/p2p/get_compact_transactions.hpp \
    include/bitcoin/network/messages/p2p/get_data.hpp \
    include/bitcoin/network/messages/p2p/get_headers.hpp \
    include/bitcoin/network/messages/p2p/headers.hpp \
    include/bitcoin/network/messages/p2p/heading.hpp \
    include/bitcoin/network/messages/p2p/inventory.hpp \
    include/bitcoin/network/messages/p2p/inventory_item.hpp \
    include/bitcoin/network/messages/p2p/memory_pool.hpp \
    include/bitcoin/network/messages/p2p/merkle_block.hpp \
    include/bitcoin/network/messages/p2p/message.hpp \
    include/bitcoin/network/messages/p2p/messages.hpp \
    include/bitcoin/network/messages/p2p/not_found.hpp \
    include/bitcoin/network/messages/p2p/ping.hpp \
    include/bitcoin/network/messages/p2p/pong.hpp \
    include/bitcoin/network/messages/p2p/reject.hpp \
    include/bitcoin/network/messages/p2p/send_address_v2.hpp \
    include/bitcoin/network/messages/p2p/send_compact.hpp \
    include/bitcoin/network/messages/p2p/send_headers.hpp \
    include/bitcoin/network/messages/p2p/transaction.hpp \
    include/bitcoin/network/messages/p2p/version.hpp \
    include/bitcoin/network/messages/p2p/version_acknowledge.hpp \
    include/bitcoin/network/messages/p2p/witness_tx_id_relay.hpp

include_bitcoin_network_messages_p2p_enumsdir = ${includedir}/bitcoin/network/messages/p2p/enums
include_bitcoin_network_messages_p2p_enums_HEADERS = \
    include/bitcoin/network/messages/p2p/enums/identifier.hpp \
    include/bitcoin/network/messages/p2p/enums/level.hpp \
    include/bitcoin/network/messages/p2p/enums/magic_numbers.hpp \
    include/bitcoin/network/messages/p2p/enums/service.hpp

include_bitcoin_network_messages_rpcdir = ${includedir}/bitcoin/network/messages/rpc
include_bitcoin_network_messages_rpc_HEADERS = \
    include/bitcoin/network/messages/rpc/messages.hpp \
    include/bitcoin/network/messages/rpc/method.hpp

include_bitcoin_network_messages_rpc_enumsdir = ${includedir}/bitcoin/network/messages/rpc/enums
include_bitcoin_network_messages_rpc_enums_HEADERS = \
    include/bitcoin/network/messages/rpc/enums/magic_numbers.hpp \
    include/bitcoin/network/messages/rpc/enums/target.hpp

include_bitcoin_network_netdir = ${includedir}/bitcoin/network/net
include_bitcoin_network_net_HEADERS = \
    include/bitcoin/network/net/acceptor.hpp \
    include/bitcoin/network/net/broadcaster.hpp \
    include/bitcoin/network/net/channel.hpp \
    include/bitcoin/network/net/channel_client.hpp \
    include/bitcoin/network/net/channel_peer.hpp \
    include/bitcoin/network/net/connector.hpp \
    include/bitcoin/network/net/deadline.hpp \
    include/bitcoin/network/net/distributor_client.hpp \
    include/bitcoin/network/net/distributor_peer.hpp \
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
    include/bitcoin/network/protocols/protocol_client.hpp \
    include/bitcoin/network/protocols/protocol_peer.hpp \
    include/bitcoin/network/protocols/protocol_ping_106.hpp \
    include/bitcoin/network/protocols/protocol_ping_60001.hpp \
    include/bitcoin/network/protocols/protocol_reject_70002.hpp \
    include/bitcoin/network/protocols/protocol_seed_209.hpp \
    include/bitcoin/network/protocols/protocol_version_106.hpp \
    include/bitcoin/network/protocols/protocol_version_70001.hpp \
    include/bitcoin/network/protocols/protocol_version_70002.hpp \
    include/bitcoin/network/protocols/protocol_version_70016.hpp \
    include/bitcoin/network/protocols/protocols.hpp

include_bitcoin_network_sessionsdir = ${includedir}/bitcoin/network/sessions
include_bitcoin_network_sessions_HEADERS = \
    include/bitcoin/network/sessions/session.hpp \
    include/bitcoin/network/sessions/session_client.hpp \
    include/bitcoin/network/sessions/session_inbound.hpp \
    include/bitcoin/network/sessions/session_inbound_client.hpp \
    include/bitcoin/network/sessions/session_manual.hpp \
    include/bitcoin/network/sessions/session_outbound.hpp \
    include/bitcoin/network/sessions/session_peer.hpp \
    include/bitcoin/network/sessions/session_seed.hpp \
    include/bitcoin/network/sessions/sessions.hpp

