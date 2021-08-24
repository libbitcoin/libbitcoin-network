###############################################################################
#  Copyright (c) 2014-2020 libbitcoin-network developers (see COPYING).
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
    src/acceptor.cpp \
    src/channel.cpp \
    src/connector.cpp \
    src/hosts.cpp \
    src/message_subscriber.cpp \
    src/p2p.cpp \
    src/proxy.cpp \
    src/settings.cpp \
    src/concurrent/conditional_lock.cpp \
    src/concurrent/deadline.cpp \
    src/concurrent/dispatcher.cpp \
    src/concurrent/file_lock.cpp \
    src/concurrent/flush_lock.cpp \
    src/concurrent/interprocess_lock.cpp \
    src/concurrent/monitor.cpp \
    src/concurrent/prioritized_mutex.cpp \
    src/concurrent/scope_lock.cpp \
    src/concurrent/sequencer.cpp \
    src/concurrent/sequential_lock.cpp \
    src/concurrent/socket.cpp \
    src/concurrent/thread.cpp \
    src/concurrent/threadpool.cpp \
    src/concurrent/work.cpp \
    src/log/file_collector.cpp \
    src/log/file_collector_repository.cpp \
    src/log/file_counter_formatter.cpp \
    src/log/sink.cpp \
    src/log/statsd_sink.cpp \
    src/log/udp_client_sink.cpp \
    src/protocols/protocol.cpp \
    src/protocols/protocol_address_31402.cpp \
    src/protocols/protocol_events.cpp \
    src/protocols/protocol_ping_31402.cpp \
    src/protocols/protocol_ping_60001.cpp \
    src/protocols/protocol_reject_70002.cpp \
    src/protocols/protocol_seed_31402.cpp \
    src/protocols/protocol_timer.cpp \
    src/protocols/protocol_version_31402.cpp \
    src/protocols/protocol_version_70002.cpp \
    src/sessions/session.cpp \
    src/sessions/session_batch.cpp \
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
test_libbitcoin_network_test_LDADD = src/libbitcoin-network.la ${boost_unit_test_framework_LIBS} ${bitcoin_system_LIBS}
test_libbitcoin_network_test_SOURCES = \
    test/main.cpp \
    test/p2p.cpp \
    test/test.cpp \
    test/test.hpp \
    test/concurrent/file_lock.cpp \
    test/concurrent/flush_lock.cpp \
    test/concurrent/interprocess_lock.cpp \
    test/concurrent/thread.cpp

endif WITH_TESTS

# files => ${includedir}/bitcoin
#------------------------------------------------------------------------------
include_bitcoindir = ${includedir}/bitcoin
include_bitcoin_HEADERS = \
    include/bitcoin/network.hpp

include_bitcoin_networkdir = ${includedir}/bitcoin/network
include_bitcoin_network_HEADERS = \
    include/bitcoin/network/acceptor.hpp \
    include/bitcoin/network/channel.hpp \
    include/bitcoin/network/connector.hpp \
    include/bitcoin/network/define.hpp \
    include/bitcoin/network/hosts.hpp \
    include/bitcoin/network/message_subscriber.hpp \
    include/bitcoin/network/p2p.hpp \
    include/bitcoin/network/proxy.hpp \
    include/bitcoin/network/settings.hpp \
    include/bitcoin/network/version.hpp

include_bitcoin_network_concurrentdir = ${includedir}/bitcoin/network/concurrent
include_bitcoin_network_concurrent_HEADERS = \
    include/bitcoin/network/concurrent/asio.hpp \
    include/bitcoin/network/concurrent/atomic.hpp \
    include/bitcoin/network/concurrent/concurrent.hpp \
    include/bitcoin/network/concurrent/conditional_lock.hpp \
    include/bitcoin/network/concurrent/deadline.hpp \
    include/bitcoin/network/concurrent/decorator.hpp \
    include/bitcoin/network/concurrent/delegates.hpp \
    include/bitcoin/network/concurrent/dispatcher.hpp \
    include/bitcoin/network/concurrent/enable_shared_from_base.hpp \
    include/bitcoin/network/concurrent/file_lock.hpp \
    include/bitcoin/network/concurrent/flush_lock.hpp \
    include/bitcoin/network/concurrent/handlers.hpp \
    include/bitcoin/network/concurrent/interprocess_lock.hpp \
    include/bitcoin/network/concurrent/monitor.hpp \
    include/bitcoin/network/concurrent/pending.hpp \
    include/bitcoin/network/concurrent/prioritized_mutex.hpp \
    include/bitcoin/network/concurrent/resubscriber.hpp \
    include/bitcoin/network/concurrent/scope_lock.hpp \
    include/bitcoin/network/concurrent/sequencer.hpp \
    include/bitcoin/network/concurrent/sequential_lock.hpp \
    include/bitcoin/network/concurrent/socket.hpp \
    include/bitcoin/network/concurrent/subscriber.hpp \
    include/bitcoin/network/concurrent/synchronizer.hpp \
    include/bitcoin/network/concurrent/thread.hpp \
    include/bitcoin/network/concurrent/threadpool.hpp \
    include/bitcoin/network/concurrent/timer.hpp \
    include/bitcoin/network/concurrent/track.hpp \
    include/bitcoin/network/concurrent/work.hpp

include_bitcoin_network_impl_concurrentdir = ${includedir}/bitcoin/network/impl/concurrent
include_bitcoin_network_impl_concurrent_HEADERS = \
    include/bitcoin/network/impl/concurrent/pending.ipp \
    include/bitcoin/network/impl/concurrent/resubscriber.ipp \
    include/bitcoin/network/impl/concurrent/subscriber.ipp \
    include/bitcoin/network/impl/concurrent/track.ipp

include_bitcoin_network_impl_log_featuresdir = ${includedir}/bitcoin/network/impl/log/features
include_bitcoin_network_impl_log_features_HEADERS = \
    include/bitcoin/network/impl/log/features/counter.ipp \
    include/bitcoin/network/impl/log/features/gauge.ipp \
    include/bitcoin/network/impl/log/features/metric.ipp \
    include/bitcoin/network/impl/log/features/rate.ipp \
    include/bitcoin/network/impl/log/features/timer.ipp

include_bitcoin_network_logdir = ${includedir}/bitcoin/network/log
include_bitcoin_network_log_HEADERS = \
    include/bitcoin/network/log/attributes.hpp \
    include/bitcoin/network/log/file_char_traits.hpp \
    include/bitcoin/network/log/file_collector.hpp \
    include/bitcoin/network/log/file_collector_repository.hpp \
    include/bitcoin/network/log/file_counter_formatter.hpp \
    include/bitcoin/network/log/log.hpp \
    include/bitcoin/network/log/rotable_file.hpp \
    include/bitcoin/network/log/severity.hpp \
    include/bitcoin/network/log/sink.hpp \
    include/bitcoin/network/log/source.hpp \
    include/bitcoin/network/log/statsd_sink.hpp \
    include/bitcoin/network/log/statsd_source.hpp \
    include/bitcoin/network/log/udp_client_sink.hpp

include_bitcoin_network_log_featuresdir = ${includedir}/bitcoin/network/log/features
include_bitcoin_network_log_features_HEADERS = \
    include/bitcoin/network/log/features/counter.hpp \
    include/bitcoin/network/log/features/gauge.hpp \
    include/bitcoin/network/log/features/metric.hpp \
    include/bitcoin/network/log/features/rate.hpp \
    include/bitcoin/network/log/features/timer.hpp

include_bitcoin_network_protocolsdir = ${includedir}/bitcoin/network/protocols
include_bitcoin_network_protocols_HEADERS = \
    include/bitcoin/network/protocols/protocol.hpp \
    include/bitcoin/network/protocols/protocol_address_31402.hpp \
    include/bitcoin/network/protocols/protocol_events.hpp \
    include/bitcoin/network/protocols/protocol_ping_31402.hpp \
    include/bitcoin/network/protocols/protocol_ping_60001.hpp \
    include/bitcoin/network/protocols/protocol_reject_70002.hpp \
    include/bitcoin/network/protocols/protocol_seed_31402.hpp \
    include/bitcoin/network/protocols/protocol_timer.hpp \
    include/bitcoin/network/protocols/protocol_version_31402.hpp \
    include/bitcoin/network/protocols/protocol_version_70002.hpp

include_bitcoin_network_sessionsdir = ${includedir}/bitcoin/network/sessions
include_bitcoin_network_sessions_HEADERS = \
    include/bitcoin/network/sessions/session.hpp \
    include/bitcoin/network/sessions/session_batch.hpp \
    include/bitcoin/network/sessions/session_inbound.hpp \
    include/bitcoin/network/sessions/session_manual.hpp \
    include/bitcoin/network/sessions/session_outbound.hpp \
    include/bitcoin/network/sessions/session_seed.hpp

