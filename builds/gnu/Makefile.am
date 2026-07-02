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
    COPYING \
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
    -I${srcdir}/../../include \
    -I${srcdir}/../../include/bitcoin/network/ssl \
    ${libbitcoin_system_BUILD_CPPFLAGS}

src_libbitcoin_network_la_LDFLAGS = \
    ${libbitcoin_system_LDFLAGS}

src_libbitcoin_network_la_LIBADD = \
    ${libbitcoin_system_LIBS}

src_libbitcoin_network_la_SOURCES = \
    ${srcdir}/../../src/error.cpp \
    ${srcdir}/../../src/memory.cpp \
    ${srcdir}/../../src/net.cpp \
    ${srcdir}/../../src/settings.cpp \
    ${srcdir}/../../src/async/thread.cpp \
    ${srcdir}/../../src/async/threadpool.cpp \
    ${srcdir}/../../src/async/time.cpp \
    ${srcdir}/../../src/channels/channel.cpp \
    ${srcdir}/../../src/channels/channel_http.cpp \
    ${srcdir}/../../src/channels/channel_peer.cpp \
    ${srcdir}/../../src/config/address.cpp \
    ${srcdir}/../../src/config/authority.cpp \
    ${srcdir}/../../src/config/endpoint.cpp \
    ${srcdir}/../../src/config/utilities.cpp \
    ${srcdir}/../../src/log/capture.cpp \
    ${srcdir}/../../src/log/logger.cpp \
    ${srcdir}/../../src/log/reporter.cpp \
    ${srcdir}/../../src/messages/http_body.cpp \
    ${srcdir}/../../src/messages/http/fields.cpp \
    ${srcdir}/../../src/messages/http/enums/media_type.cpp \
    ${srcdir}/../../src/messages/http/enums/target.cpp \
    ${srcdir}/../../src/messages/peer/message.cpp \
    ${srcdir}/../../src/messages/peer/detail/address.cpp \
    ${srcdir}/../../src/messages/peer/detail/address_item.cpp \
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
    ${srcdir}/../../src/messages/peer/detail/transaction.cpp \
    ${srcdir}/../../src/messages/peer/detail/version.cpp \
    ${srcdir}/../../src/messages/peer/detail/version_acknowledge.cpp \
    ${srcdir}/../../src/messages/peer/detail/witness_tx_id_relay.cpp \
    ${srcdir}/../../src/messages/rpc/body.cpp \
    ${srcdir}/../../src/messages/rpc/model.cpp \
    ${srcdir}/../../src/net/acceptor.cpp \
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
    ${srcdir}/../../src/net/socket_rpc.cpp \
    ${srcdir}/../../src/net/socket_stop.cpp \
    ${srcdir}/../../src/net/socket_tcp.cpp \
    ${srcdir}/../../src/net/socket_wait.cpp \
    ${srcdir}/../../src/net/socket_ws.cpp \
    ${srcdir}/../../src/protocols/protocol.cpp \
    ${srcdir}/../../src/protocols/protocol_address_in_209.cpp \
    ${srcdir}/../../src/protocols/protocol_address_out_209.cpp \
    ${srcdir}/../../src/protocols/protocol_alert_311.cpp \
    ${srcdir}/../../src/protocols/protocol_http.cpp \
    ${srcdir}/../../src/protocols/protocol_peer.cpp \
    ${srcdir}/../../src/protocols/protocol_ping_106.cpp \
    ${srcdir}/../../src/protocols/protocol_ping_60001.cpp \
    ${srcdir}/../../src/protocols/protocol_reject_70002.cpp \
    ${srcdir}/../../src/protocols/protocol_seed_209.cpp \
    ${srcdir}/../../src/protocols/protocol_version_106.cpp \
    ${srcdir}/../../src/protocols/protocol_version_70001.cpp \
    ${srcdir}/../../src/protocols/protocol_version_70002.cpp \
    ${srcdir}/../../src/protocols/protocol_version_70016.cpp \
    ${srcdir}/../../src/sessions/session.cpp \
    ${srcdir}/../../src/sessions/session_inbound.cpp \
    ${srcdir}/../../src/sessions/session_manual.cpp \
    ${srcdir}/../../src/sessions/session_outbound.cpp \
    ${srcdir}/../../src/sessions/session_peer.cpp \
    ${srcdir}/../../src/sessions/session_seed.cpp \
    ${srcdir}/../../src/sessions/session_server.cpp

if WITH_SSL

src_libbitcoin_network_la_SOURCES += \
    ${srcdir}/../../src/ssl/wolfssl/internal.c \
    ${srcdir}/../../src/ssl/wolfssl/keys.c \
    ${srcdir}/../../src/ssl/wolfssl/ssl.c \
    ${srcdir}/../../src/ssl/wolfssl/tls.c \
    ${srcdir}/../../src/ssl/wolfssl/tls13.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfio.c \
    ${srcdir}/../../src/ssl/wolfssl/src/bio.c \
    ${srcdir}/../../src/ssl/wolfssl/src/conf.c \
    ${srcdir}/../../src/ssl/wolfssl/src/evp.c \
    ${srcdir}/../../src/ssl/wolfssl/src/pk.c \
    ${srcdir}/../../src/ssl/wolfssl/src/ssl_asn1.c \
    ${srcdir}/../../src/ssl/wolfssl/src/ssl_bn.c \
    ${srcdir}/../../src/ssl/wolfssl/src/ssl_certman.c \
    ${srcdir}/../../src/ssl/wolfssl/src/ssl_crypto.c \
    ${srcdir}/../../src/ssl/wolfssl/src/ssl_load.c \
    ${srcdir}/../../src/ssl/wolfssl/src/ssl_misc.c \
    ${srcdir}/../../src/ssl/wolfssl/src/ssl_p7p12.c \
    ${srcdir}/../../src/ssl/wolfssl/src/ssl_sess.c \
    ${srcdir}/../../src/ssl/wolfssl/src/ssl_sk.c \
    ${srcdir}/../../src/ssl/wolfssl/src/x509.c \
    ${srcdir}/../../src/ssl/wolfssl/src/x509_str.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/aes.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/asn.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/chacha.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/chacha20_poly1305.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/coding.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/cryptocb.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/curve25519.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/ecc.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/ed25519.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/error.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/fe_operations.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/ge_operations.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/hash.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/hmac.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/kdf.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/logging.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/memory.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/pkcs12.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/poly1305.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/pwdbased.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/random.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/sha.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/sha256.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/sha512.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/signature.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/sp_int.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/wc_encrypt.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/wc_port.c \
    ${srcdir}/../../src/ssl/wolfssl/wolfcrypt/wolfmath.c

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
    ${srcdir}/../../include/bitcoin/network/version.hpp

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
    ${srcdir}/../../include/bitcoin/network/async/races/race_quality.hpp \
    ${srcdir}/../../include/bitcoin/network/async/races/race_speed.hpp \
    ${srcdir}/../../include/bitcoin/network/async/races/race_unity.hpp \
    ${srcdir}/../../include/bitcoin/network/async/races/race_volume.hpp \
    ${srcdir}/../../include/bitcoin/network/async/races/races.hpp

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
    ${srcdir}/../../include/bitcoin/network/config/authority.hpp \
    ${srcdir}/../../include/bitcoin/network/config/config.hpp \
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
    ${srcdir}/../../include/bitcoin/network/messages/peer/heading.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/message.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/peer.hpp

include_bitcoin_network_messages_peer_detaildir = \
    ${includedir}/bitcoin/network/messages/peer/detail

include_bitcoin_network_messages_peer_detail_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/address.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/address_item.hpp \
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
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/transaction.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/version.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/version_acknowledge.hpp \
    ${srcdir}/../../include/bitcoin/network/messages/peer/detail/witness_tx_id_relay.hpp

include_bitcoin_network_messages_peer_enumsdir = \
    ${includedir}/bitcoin/network/messages/peer/enums

include_bitcoin_network_messages_peer_enums_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/messages/peer/enums/identifier.hpp \
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
    ${srcdir}/../../include/bitcoin/network/net/connector.hpp \
    ${srcdir}/../../include/bitcoin/network/net/connector_socks.hpp \
    ${srcdir}/../../include/bitcoin/network/net/deadline.hpp \
    ${srcdir}/../../include/bitcoin/network/net/hosts.hpp \
    ${srcdir}/../../include/bitcoin/network/net/net.hpp \
    ${srcdir}/../../include/bitcoin/network/net/proxy.hpp \
    ${srcdir}/../../include/bitcoin/network/net/socket.hpp

include_bitcoin_network_protocolsdir = \
    ${includedir}/bitcoin/network/protocols

include_bitcoin_network_protocols_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_address_in_209.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_address_out_209.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_alert_311.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_http.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_peer.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_ping_106.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_ping_60001.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_reject_70002.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_rpc.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_seed_209.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_version_106.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_version_70001.hpp \
    ${srcdir}/../../include/bitcoin/network/protocols/protocol_version_70002.hpp \
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

include_bitcoin_network_ssl_wolfssldir = \
    ${includedir}/bitcoin/network/ssl/wolfssl

include_bitcoin_network_ssl_wolfssl_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/callbacks.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/certs_test.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/error-ssl.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/internal.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/ocsp.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/options.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/ssl.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/test.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/version.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfio.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfssl.h

include_bitcoin_network_ssl_wolfssl_openssldir = \
    ${includedir}/bitcoin/network/ssl/wolfssl/openssl

include_bitcoin_network_ssl_wolfssl_openssl_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/aes.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/asn1.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/bio.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/bn.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/buffer.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/cmac.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/compat_types.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/conf.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/crypto.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/des.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/dh.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/dsa.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/ec.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/ec25519.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/ec448.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/ecdh.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/ecdsa.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/ed25519.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/ed448.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/engine.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/err.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/evp.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/fips_rand.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/hmac.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/kdf.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/lhash.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/md4.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/md5.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/modes.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/obj_mac.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/objects.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/opensslv.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/pem.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/pkcs12.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/pkcs7.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/rand.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/rc4.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/ripemd.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/rsa.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/sha.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/sha3.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/ssl.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/stack.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/tls1.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/x509.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/x509_vfy.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/openssl/x509v3.h

include_bitcoin_network_ssl_wolfssl_wolfcryptdir = \
    ${includedir}/bitcoin/network/ssl/wolfssl/wolfcrypt

include_bitcoin_network_ssl_wolfssl_wolfcrypt_HEADERS = \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/aes.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/arc4.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/ascon.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/asn.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/asn_public.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/blake2.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/camellia.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/chacha.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/chacha20_poly1305.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/cmac.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/coding.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/cpuid.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/cryptocb.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/curve25519.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/curve448.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/des3.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/dh.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/dsa.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/ecc.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/ed25519.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/ed448.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/error-crypt.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/fe_operations.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/fips_test.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/ge_operations.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/hash.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/hmac.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/hpke.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/kdf.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/libwolfssl_sources.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/logging.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/md2.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/md4.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/md5.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/mem_track.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/memory.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/misc.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/oid_sum.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/pkcs12.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/pkcs7.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/poly1305.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/pwdbased.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/random.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/rc2.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/ripemd.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/rsa.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/settings.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/sha.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/sha256.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/sha3.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/sha512.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/signature.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/siphash.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/sp_int.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/srp.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/types.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/user_settings.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/visibility.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/wc_encrypt.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/wc_port.h \
    ${srcdir}/../../include/bitcoin/network/ssl/wolfssl/wolfcrypt/wolfmath.h

endif WITH_SSL

# Tests.
#==============================================================================
# Target test 'test/libbitcoin-network-test'
#------------------------------------------------------------------------------
if WITH_TESTS

check_PROGRAMS = test/libbitcoin-network-test

test_libbitcoin_network_test_CPPFLAGS = \
    -I${srcdir}/../../test \
    -I${srcdir}/../../test/ssl/wolfssl \
    ${boost_BUILD_CPPFLAGS} \
    ${boost_unit_test_framework_BUILD_CPPFLAGS} \
    ${src_libbitcoin_network_la_CPPFLAGS}

test_libbitcoin_network_test_LDFLAGS = \
    ${boost_LDFLAGS} \
    ${boost_unit_test_framework_LDFLAGS} \
    ${src_libbitcoin_network_la_LDFLAGS}

test_libbitcoin_network_test_LDADD = \
    ${boost_LIBS} \
    ${boost_unit_test_framework_LIBS} \
    ${src_libbitcoin_network_la_LIBS} \
    ${src_libbitcoin_network_la_LIBADD}

test_libbitcoin_network_test_SOURCES = \
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
    ${srcdir}/../../test/async/races/race_quality.cpp \
    ${srcdir}/../../test/async/races/race_speed.cpp \
    ${srcdir}/../../test/async/races/race_unity.cpp \
    ${srcdir}/../../test/async/races/race_volume.cpp \
    ${srcdir}/../../test/channels/channel.cpp \
    ${srcdir}/../../test/channels/channel_http.cpp \
    ${srcdir}/../../test/channels/channel_peer.cpp \
    ${srcdir}/../../test/channels/channel_rpc.cpp \
    ${srcdir}/../../test/config/address.cpp \
    ${srcdir}/../../test/config/authority.cpp \
    ${srcdir}/../../test/config/endpoint.cpp \
    ${srcdir}/../../test/config/utilities.cpp \
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
    ${srcdir}/../../test/messages/peer/heading.cpp \
    ${srcdir}/../../test/messages/peer/message.cpp \
    ${srcdir}/../../test/messages/peer/detail/address.cpp \
    ${srcdir}/../../test/messages/peer/detail/address_item.cpp \
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
    ${srcdir}/../../test/messages/rpc/types.cpp \
    ${srcdir}/../../test/net/acceptor.cpp \
    ${srcdir}/../../test/net/connector.cpp \
    ${srcdir}/../../test/net/connector_socks.cpp \
    ${srcdir}/../../test/net/deadline.cpp \
    ${srcdir}/../../test/net/hosts.cpp \
    ${srcdir}/../../test/net/proxy.cpp \
    ${srcdir}/../../test/net/socket.cpp \
    ${srcdir}/../../test/protocols/protocol.cpp \
    ${srcdir}/../../test/protocols/protocol_address_in_209.cpp \
    ${srcdir}/../../test/protocols/protocol_address_out_209.cpp \
    ${srcdir}/../../test/protocols/protocol_alert_311.cpp \
    ${srcdir}/../../test/protocols/protocol_http.cpp \
    ${srcdir}/../../test/protocols/protocol_peer.cpp \
    ${srcdir}/../../test/protocols/protocol_ping_106.cpp \
    ${srcdir}/../../test/protocols/protocol_ping_60001.cpp \
    ${srcdir}/../../test/protocols/protocol_reject_70002.cpp \
    ${srcdir}/../../test/protocols/protocol_seed_209.cpp \
    ${srcdir}/../../test/protocols/protocol_version_106.cpp \
    ${srcdir}/../../test/protocols/protocol_version_70001.cpp \
    ${srcdir}/../../test/protocols/protocol_version_70002.cpp \
    ${srcdir}/../../test/protocols/protocol_version_70016.cpp \
    ${srcdir}/../../test/sessions/session.cpp \
    ${srcdir}/../../test/sessions/session_html.cpp \
    ${srcdir}/../../test/sessions/session_inbound.cpp \
    ${srcdir}/../../test/sessions/session_manual.cpp \
    ${srcdir}/../../test/sessions/session_outbound.cpp \
    ${srcdir}/../../test/sessions/session_seed.cpp \
    ${srcdir}/../../test/sessions/session_server.cpp \
    ${srcdir}/../../test/ssl/wolfssl.cpp

if WITH_SSL

test_libbitcoin_network_test_SOURCES += \
    ${srcdir}/../../test/ssl/wolfssl/examples/client/client.c \
    ${srcdir}/../../test/ssl/wolfssl/examples/server/server.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/quic.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/srp.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/suites.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/unit.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/utils.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/w64wrapper.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_aes.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_arc4.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ascon.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_asn.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_blake2.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_camellia.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_chacha.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_chacha20_poly1305.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_cmac.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_curve25519.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_curve448.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_des3.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_dh.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_dsa.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_dtls.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ecc.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ed25519.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ed448.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_evp.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_hash.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_hmac.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_md2.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_md4.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_md5.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_mldsa.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_mlkem.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ocsp.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ossl_asn1.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ossl_bio.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ossl_bn.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ossl_cipher.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ossl_dgst.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ossl_dh.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ossl_dsa.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ossl_ec.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ossl_ecx.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ossl_mac.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ossl_rsa.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ossl_sk.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_pkcs12.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_pkcs7.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_poly1305.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_random.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_rc2.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_ripemd.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_rsa.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_sha.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_sha256.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_sha3.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_sha512.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_signature.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_sm2.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_sm3.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_sm4.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_tls.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_tls13.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_tls_ext.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_wc_encrypt.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_wolfmath.c \
    ${srcdir}/../../test/ssl/wolfssl/tests/api/test_x509.c \
    ${srcdir}/../../test/ssl/wolfssl/testsuite/utils.c \
    ${srcdir}/../../test/ssl/wolfssl/wolfcrypt/benchmark/benchmark.c \
    ${srcdir}/../../test/ssl/wolfssl/wolfcrypt/test/test.c

endif WITH_SSL

TESTS = test_runner.sh

endif WITH_TESTS
