###############################################################################
#  Copyright (c) 2014-2023 libbitcoin-network developers (see COPYING).
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
src_libbitcoin_network_la_LIBADD = ${boost_regex_LIBS} ${bitcoin_system_LIBS}
src_libbitcoin_network_la_SOURCES = \
    src/error.cpp \
    src/p2p.cpp \
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
    src/messages/address.cpp \
    src/messages/address_item.cpp \
    src/messages/alert.cpp \
    src/messages/alert_item.cpp \
    src/messages/block.cpp \
    src/messages/bloom_filter_add.cpp \
    src/messages/bloom_filter_clear.cpp \
    src/messages/bloom_filter_load.cpp \
    src/messages/client_filter.cpp \
    src/messages/client_filter_checkpoint.cpp \
    src/messages/client_filter_headers.cpp \
    src/messages/compact_block.cpp \
    src/messages/compact_block_item.cpp \
    src/messages/compact_transactions.cpp \
    src/messages/fee_filter.cpp \
    src/messages/get_address.cpp \
    src/messages/get_blocks.cpp \
    src/messages/get_client_filter_checkpoint.cpp \
    src/messages/get_client_filter_headers.cpp \
    src/messages/get_client_filters.cpp \
    src/messages/get_compact_transactions.cpp \
    src/messages/get_data.cpp \
    src/messages/get_headers.cpp \
    src/messages/headers.cpp \
    src/messages/heading.cpp \
    src/messages/inventory.cpp \
    src/messages/inventory_item.cpp \
    src/messages/memory_pool.cpp \
    src/messages/merkle_block.cpp \
    src/messages/message.cpp \
    src/messages/not_found.cpp \
    src/messages/ping.cpp \
    src/messages/pong.cpp \
    src/messages/reject.cpp \
    src/messages/send_compact.cpp \
    src/messages/send_headers.cpp \
    src/messages/transaction.cpp \
    src/messages/version.cpp \
    src/messages/version_acknowledge.cpp \
    src/net/acceptor.cpp \
    src/net/broadcaster.cpp \
    src/net/channel.cpp \
    src/net/connector.cpp \
    src/net/deadline.cpp \
    src/net/distributor.cpp \
    src/net/hosts.cpp \
    src/net/proxy.cpp \
    src/net/socket.cpp \
    src/protocols/protocol.cpp \
    src/protocols/protocol_address_in_31402.cpp \
    src/protocols/protocol_address_out_31402.cpp \
    src/protocols/protocol_alert_31402.cpp \
    src/protocols/protocol_ping_31402.cpp \
    src/protocols/protocol_ping_60001.cpp \
    src/protocols/protocol_reject_70002.cpp \
    src/protocols/protocol_seed_31402.cpp \
    src/protocols/protocol_version_31402.cpp \
    src/protocols/protocol_version_70001.cpp \
    src/protocols/protocol_version_70002.cpp \
    src/sessions/session.cpp \
    src/sessions/session_inbound.cpp \
    src/sessions/session_manual.cpp \
    src/sessions/session_outbound.cpp \
    src/sessions/session_seed.cpp

# local: test/libbitcoin-network-test
#------------------------------------------------------------------------------
if WITH_TESTS

TESTS = libbitcoin-network-test_runner.sh

check_PROGRAMS = test/libbitcoin-network-test
test_libbitcoin_network_test_CPPFLAGS = -I${srcdir}/include ${bitcoin_system_BUILD_CPPFLAGS}
test_libbitcoin_network_test_LDADD = src/libbitcoin-network.la ${boost_unit_test_framework_LIBS} ${boost_regex_LIBS} ${bitcoin_system_LIBS}
test_libbitcoin_network_test_SOURCES = \
    test/error.cpp \
    test/main.cpp \
    test/p2p.cpp \
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
    test/messages/address.cpp \
    test/messages/address_item.cpp \
    test/messages/alert.cpp \
    test/messages/alert_item.cpp \
    test/messages/block.cpp \
    test/messages/bloom_filter_add.cpp \
    test/messages/bloom_filter_clear.cpp \
    test/messages/bloom_filter_load.cpp \
    test/messages/client_filter.cpp \
    test/messages/client_filter_checkpoint.cpp \
    test/messages/client_filter_headers.cpp \
    test/messages/compact_block.cpp \
    test/messages/compact_block_item.cpp \
    test/messages/compact_transactions.cpp \
    test/messages/fee_filter.cpp \
    test/messages/get_address.cpp \
    test/messages/get_blocks.cpp \
    test/messages/get_client_filter_checkpoint.cpp \
    test/messages/get_client_filter_headers.cpp \
    test/messages/get_client_filters.cpp \
    test/messages/get_compact_transactions.cpp \
    test/messages/get_data.cpp \
    test/messages/get_headers.cpp \
    test/messages/headers.cpp \
    test/messages/heading.cpp \
    test/messages/inventory.cpp \
    test/messages/inventory_item.cpp \
    test/messages/memory_pool.cpp \
    test/messages/merkle_block.cpp \
    test/messages/message.cpp \
    test/messages/not_found.cpp \
    test/messages/ping.cpp \
    test/messages/pong.cpp \
    test/messages/reject.cpp \
    test/messages/send_compact.cpp \
    test/messages/send_headers.cpp \
    test/messages/transaction.cpp \
    test/messages/version.cpp \
    test/messages/version_acknowledge.cpp \
    test/net/acceptor.cpp \
    test/net/broadcaster.cpp \
    test/net/channel.cpp \
    test/net/connector.cpp \
    test/net/deadline.cpp \
    test/net/distributor.cpp \
    test/net/hosts.cpp \
    test/net/proxy.cpp \
    test/net/socket.cpp \
    test/protocols/protocol.cpp \
    test/protocols/protocol_address_in_31402.cpp \
    test/protocols/protocol_address_out_31402.cpp \
    test/protocols/protocol_alert_31402.cpp \
    test/protocols/protocol_ping_31402.cpp \
    test/protocols/protocol_ping_60001.cpp \
    test/protocols/protocol_reject_70002.cpp \
    test/protocols/protocol_seed_31402.cpp \
    test/protocols/protocol_version_31402.cpp \
    test/protocols/protocol_version_70001.cpp \
    test/protocols/protocol_version_70002.cpp \
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
    include/bitcoin/network/define.hpp \
    include/bitcoin/network/error.hpp \
    include/bitcoin/network/p2p.hpp \
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

include_bitcoin_network_messagesdir = ${includedir}/bitcoin/network/messages
include_bitcoin_network_messages_HEADERS = \
    include/bitcoin/network/messages/address.hpp \
    include/bitcoin/network/messages/address_item.hpp \
    include/bitcoin/network/messages/alert.hpp \
    include/bitcoin/network/messages/alert_item.hpp \
    include/bitcoin/network/messages/block.hpp \
    include/bitcoin/network/messages/bloom_filter_add.hpp \
    include/bitcoin/network/messages/bloom_filter_clear.hpp \
    include/bitcoin/network/messages/bloom_filter_load.hpp \
    include/bitcoin/network/messages/client_filter.hpp \
    include/bitcoin/network/messages/client_filter_checkpoint.hpp \
    include/bitcoin/network/messages/client_filter_headers.hpp \
    include/bitcoin/network/messages/compact_block.hpp \
    include/bitcoin/network/messages/compact_block_item.hpp \
    include/bitcoin/network/messages/compact_transactions.hpp \
    include/bitcoin/network/messages/fee_filter.hpp \
    include/bitcoin/network/messages/get_address.hpp \
    include/bitcoin/network/messages/get_blocks.hpp \
    include/bitcoin/network/messages/get_client_filter_checkpoint.hpp \
    include/bitcoin/network/messages/get_client_filter_headers.hpp \
    include/bitcoin/network/messages/get_client_filters.hpp \
    include/bitcoin/network/messages/get_compact_transactions.hpp \
    include/bitcoin/network/messages/get_data.hpp \
    include/bitcoin/network/messages/get_headers.hpp \
    include/bitcoin/network/messages/headers.hpp \
    include/bitcoin/network/messages/heading.hpp \
    include/bitcoin/network/messages/inventory.hpp \
    include/bitcoin/network/messages/inventory_item.hpp \
    include/bitcoin/network/messages/memory_pool.hpp \
    include/bitcoin/network/messages/merkle_block.hpp \
    include/bitcoin/network/messages/message.hpp \
    include/bitcoin/network/messages/messages.hpp \
    include/bitcoin/network/messages/not_found.hpp \
    include/bitcoin/network/messages/ping.hpp \
    include/bitcoin/network/messages/pong.hpp \
    include/bitcoin/network/messages/reject.hpp \
    include/bitcoin/network/messages/send_compact.hpp \
    include/bitcoin/network/messages/send_headers.hpp \
    include/bitcoin/network/messages/transaction.hpp \
    include/bitcoin/network/messages/version.hpp \
    include/bitcoin/network/messages/version_acknowledge.hpp

include_bitcoin_network_messages_enumsdir = ${includedir}/bitcoin/network/messages/enums
include_bitcoin_network_messages_enums_HEADERS = \
    include/bitcoin/network/messages/enums/identifier.hpp \
    include/bitcoin/network/messages/enums/level.hpp \
    include/bitcoin/network/messages/enums/magic_numbers.hpp \
    include/bitcoin/network/messages/enums/service.hpp

include_bitcoin_network_netdir = ${includedir}/bitcoin/network/net
include_bitcoin_network_net_HEADERS = \
    include/bitcoin/network/net/acceptor.hpp \
    include/bitcoin/network/net/broadcaster.hpp \
    include/bitcoin/network/net/channel.hpp \
    include/bitcoin/network/net/connector.hpp \
    include/bitcoin/network/net/deadline.hpp \
    include/bitcoin/network/net/distributor.hpp \
    include/bitcoin/network/net/hosts.hpp \
    include/bitcoin/network/net/net.hpp \
    include/bitcoin/network/net/proxy.hpp \
    include/bitcoin/network/net/socket.hpp

include_bitcoin_network_protocolsdir = ${includedir}/bitcoin/network/protocols
include_bitcoin_network_protocols_HEADERS = \
    include/bitcoin/network/protocols/protocol.hpp \
    include/bitcoin/network/protocols/protocol_address_in_31402.hpp \
    include/bitcoin/network/protocols/protocol_address_out_31402.hpp \
    include/bitcoin/network/protocols/protocol_alert_31402.hpp \
    include/bitcoin/network/protocols/protocol_ping_31402.hpp \
    include/bitcoin/network/protocols/protocol_ping_60001.hpp \
    include/bitcoin/network/protocols/protocol_reject_70002.hpp \
    include/bitcoin/network/protocols/protocol_seed_31402.hpp \
    include/bitcoin/network/protocols/protocol_version_31402.hpp \
    include/bitcoin/network/protocols/protocol_version_70001.hpp \
    include/bitcoin/network/protocols/protocol_version_70002.hpp \
    include/bitcoin/network/protocols/protocols.hpp

include_bitcoin_network_sessionsdir = ${includedir}/bitcoin/network/sessions
include_bitcoin_network_sessions_HEADERS = \
    include/bitcoin/network/sessions/session.hpp \
    include/bitcoin/network/sessions/session_inbound.hpp \
    include/bitcoin/network/sessions/session_manual.hpp \
    include/bitcoin/network/sessions/session_outbound.hpp \
    include/bitcoin/network/sessions/session_seed.hpp \
    include/bitcoin/network/sessions/sessions.hpp

