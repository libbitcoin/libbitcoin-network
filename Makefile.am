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
    src/p2p.cpp \
    src/settings.cpp \
    src/async/deadline.cpp \
    src/async/thread.cpp \
    src/async/threadpool.cpp \
    src/async/time.cpp \
    src/log/file_collector.cpp \
    src/log/file_collector_repository.cpp \
    src/log/file_counter_formatter.cpp \
    src/log/sink.cpp \
    src/log/statsd_sink.cpp \
    src/log/udp_client_sink.cpp \
    src/net/acceptor.cpp \
    src/net/channel.cpp \
    src/net/connector.cpp \
    src/net/hosts.cpp \
    src/net/proxy.cpp \
    src/net/pump.cpp \
    src/net/socket.cpp \
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
    test/async/atomic.cpp \
    test/async/deadline.cpp \
    test/async/enable_shared_from_base.cpp \
    test/async/subscriber.cpp \
    test/async/thread.cpp \
    test/async/threadpool.cpp \
    test/async/time.cpp \
    test/async/timer.cpp \
    test/async/track.cpp \
    test/net/acceptor.cpp \
    test/net/channel.cpp \
    test/net/connector.cpp \
    test/net/hosts.cpp \
    test/net/pending.cpp \
    test/net/proxy.cpp \
    test/net/pump.cpp \
    test/net/socket.cpp \
    test/protocols/protocol.cpp \
    test/sessions/session.cpp

endif WITH_TESTS

# files => ${includedir}/bitcoin
#------------------------------------------------------------------------------
include_bitcoindir = ${includedir}/bitcoin
include_bitcoin_HEADERS = \
    include/bitcoin/network.hpp

include_bitcoin_networkdir = ${includedir}/bitcoin/network
include_bitcoin_network_HEADERS = \
    include/bitcoin/network/define.hpp \
    include/bitcoin/network/p2p.hpp \
    include/bitcoin/network/settings.hpp \
    include/bitcoin/network/version.hpp

include_bitcoin_network_asyncdir = ${includedir}/bitcoin/network/async
include_bitcoin_network_async_HEADERS = \
    include/bitcoin/network/async/asio.hpp \
    include/bitcoin/network/async/async.hpp \
    include/bitcoin/network/async/atomic.hpp \
    include/bitcoin/network/async/deadline.hpp \
    include/bitcoin/network/async/enable_shared_from_base.hpp \
    include/bitcoin/network/async/handlers.hpp \
    include/bitcoin/network/async/subscriber.hpp \
    include/bitcoin/network/async/thread.hpp \
    include/bitcoin/network/async/threadpool.hpp \
    include/bitcoin/network/async/time.hpp \
    include/bitcoin/network/async/timer.hpp \
    include/bitcoin/network/async/track.hpp

include_bitcoin_network_impl_asyncdir = ${includedir}/bitcoin/network/impl/async
include_bitcoin_network_impl_async_HEADERS = \
    include/bitcoin/network/impl/async/subscriber.ipp \
    include/bitcoin/network/impl/async/track.ipp

include_bitcoin_network_impl_log_featuresdir = ${includedir}/bitcoin/network/impl/log/features
include_bitcoin_network_impl_log_features_HEADERS = \
    include/bitcoin/network/impl/log/features/counter.ipp \
    include/bitcoin/network/impl/log/features/gauge.ipp \
    include/bitcoin/network/impl/log/features/metric.ipp \
    include/bitcoin/network/impl/log/features/rate.ipp \
    include/bitcoin/network/impl/log/features/timer.ipp

include_bitcoin_network_impl_netdir = ${includedir}/bitcoin/network/impl/net
include_bitcoin_network_impl_net_HEADERS = \
    include/bitcoin/network/impl/net/pending.ipp

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

include_bitcoin_network_netdir = ${includedir}/bitcoin/network/net
include_bitcoin_network_net_HEADERS = \
    include/bitcoin/network/net/acceptor.hpp \
    include/bitcoin/network/net/channel.hpp \
    include/bitcoin/network/net/connector.hpp \
    include/bitcoin/network/net/hosts.hpp \
    include/bitcoin/network/net/net.hpp \
    include/bitcoin/network/net/pending.hpp \
    include/bitcoin/network/net/proxy.hpp \
    include/bitcoin/network/net/pump.hpp \
    include/bitcoin/network/net/socket.hpp

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

