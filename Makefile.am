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
src_libbitcoin_network_la_CPPFLAGS = -I${srcdir}/include -I${srcdir}/include/bitcoin/ssl ${bitcoin_system_BUILD_CPPFLAGS}
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
    src/channels/channel_ws.cpp \
    src/config/address.cpp \
    src/config/authority.cpp \
    src/config/endpoint.cpp \
    src/config/utilities.cpp \
    src/log/capture.cpp \
    src/log/logger.cpp \
    src/log/reporter.cpp \
    src/messages/http_body.cpp \
    src/messages/http/fields.cpp \
    src/messages/http/enums/media_type.cpp \
    src/messages/http/enums/target.cpp \
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
    src/messages/rpc/body.cpp \
    src/messages/rpc/model.cpp \
    src/net/acceptor.cpp \
    src/net/connector.cpp \
    src/net/connector_socks.cpp \
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
    src/sessions/session.cpp \
    src/sessions/session_inbound.cpp \
    src/sessions/session_manual.cpp \
    src/sessions/session_outbound.cpp \
    src/sessions/session_peer.cpp \
    src/sessions/session_seed.cpp \
    src/sessions/session_server.cpp \
    src/ssl/wolfssl/internal.c \
    src/ssl/wolfssl/keys.c \
    src/ssl/wolfssl/ssl.c \
    src/ssl/wolfssl/tls.c \
    src/ssl/wolfssl/tls13.c \
    src/ssl/wolfssl/wolfio.c \
    src/ssl/wolfssl/src/bio.c \
    src/ssl/wolfssl/src/conf.c \
    src/ssl/wolfssl/src/evp.c \
    src/ssl/wolfssl/src/pk.c \
    src/ssl/wolfssl/src/ssl_asn1.c \
    src/ssl/wolfssl/src/ssl_bn.c \
    src/ssl/wolfssl/src/ssl_certman.c \
    src/ssl/wolfssl/src/ssl_crypto.c \
    src/ssl/wolfssl/src/ssl_load.c \
    src/ssl/wolfssl/src/ssl_misc.c \
    src/ssl/wolfssl/src/ssl_p7p12.c \
    src/ssl/wolfssl/src/ssl_sess.c \
    src/ssl/wolfssl/src/ssl_sk.c \
    src/ssl/wolfssl/src/x509.c \
    src/ssl/wolfssl/src/x509_str.c \
    src/ssl/wolfssl/wolfcrypt/asn.c \
    src/ssl/wolfssl/wolfcrypt/chacha.c \
    src/ssl/wolfssl/wolfcrypt/chacha20_poly1305.c \
    src/ssl/wolfssl/wolfcrypt/coding.c \
    src/ssl/wolfssl/wolfcrypt/cryptocb.c \
    src/ssl/wolfssl/wolfcrypt/curve25519.c \
    src/ssl/wolfssl/wolfcrypt/ecc.c \
    src/ssl/wolfssl/wolfcrypt/error.c \
    src/ssl/wolfssl/wolfcrypt/fe_operations.c \
    src/ssl/wolfssl/wolfcrypt/ge_operations.c \
    src/ssl/wolfssl/wolfcrypt/hash.c \
    src/ssl/wolfssl/wolfcrypt/hmac.c \
    src/ssl/wolfssl/wolfcrypt/kdf.c \
    src/ssl/wolfssl/wolfcrypt/logging.c \
    src/ssl/wolfssl/wolfcrypt/memory.c \
    src/ssl/wolfssl/wolfcrypt/poly1305.c \
    src/ssl/wolfssl/wolfcrypt/random.c \
    src/ssl/wolfssl/wolfcrypt/sha256.c \
    src/ssl/wolfssl/wolfcrypt/signature.c \
    src/ssl/wolfssl/wolfcrypt/sp_int.c \
    src/ssl/wolfssl/wolfcrypt/wc_encrypt.c \
    src/ssl/wolfssl/wolfcrypt/wc_port.c \
    src/ssl/wolfssl/wolfcrypt/wolfmath.c

# local: test/libbitcoin-network-test
#------------------------------------------------------------------------------
if WITH_TESTS

TESTS = libbitcoin-network-test_runner.sh

check_PROGRAMS = test/libbitcoin-network-test
test_libbitcoin_network_test_CPPFLAGS = -I${srcdir}/include -I${srcdir}/include/bitcoin/ssl -I${srcdir}/test/ssl/wolfssl ${bitcoin_system_BUILD_CPPFLAGS}
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
    test/channels/channel_rpc.cpp \
    test/channels/channel_ws.cpp \
    test/config/address.cpp \
    test/config/authority.cpp \
    test/config/endpoint.cpp \
    test/config/utilities.cpp \
    test/log/timer.cpp \
    test/log/tracker.cpp \
    test/messages/http_body_reader.cpp \
    test/messages/http_body_writer.cpp \
    test/messages/json_body_reader.cpp \
    test/messages/json_body_writer.cpp \
    test/messages/http/fields.cpp \
    test/messages/http/method.cpp \
    test/messages/http/enums/magic_numbers.cpp \
    test/messages/http/enums/media_type.cpp \
    test/messages/http/enums/target.cpp \
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
    test/messages/rpc/any.cpp \
    test/messages/rpc/body_reader.cpp \
    test/messages/rpc/body_writer.cpp \
    test/messages/rpc/broadcaster.cpp \
    test/messages/rpc/dispatcher.cpp \
    test/messages/rpc/method.cpp \
    test/messages/rpc/types.cpp \
    test/net/acceptor.cpp \
    test/net/connector.cpp \
    test/net/connector_socks.cpp \
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
    test/protocols/protocol_version_106.cpp \
    test/protocols/protocol_version_70001.cpp \
    test/protocols/protocol_version_70002.cpp \
    test/protocols/protocol_version_70016.cpp \
    test/sessions/session.cpp \
    test/sessions/session_html.cpp \
    test/sessions/session_inbound.cpp \
    test/sessions/session_manual.cpp \
    test/sessions/session_outbound.cpp \
    test/sessions/session_seed.cpp \
    test/sessions/session_server.cpp \
    test/ssl/wolfssl.cpp \
    test/ssl/wolfssl/examples/client/client.c \
    test/ssl/wolfssl/examples/client/client.h \
    test/ssl/wolfssl/examples/server/server.c \
    test/ssl/wolfssl/examples/server/server.h \
    test/ssl/wolfssl/tests/api.c \
    test/ssl/wolfssl/tests/quic.c \
    test/ssl/wolfssl/tests/srp.c \
    test/ssl/wolfssl/tests/suites.c \
    test/ssl/wolfssl/tests/unit.c \
    test/ssl/wolfssl/tests/unit.h \
    test/ssl/wolfssl/tests/utils.c \
    test/ssl/wolfssl/tests/utils.h \
    test/ssl/wolfssl/tests/w64wrapper.c \
    test/ssl/wolfssl/tests/api/api.h \
    test/ssl/wolfssl/tests/api/api_decl.h \
    test/ssl/wolfssl/tests/api/test_aes.c \
    test/ssl/wolfssl/tests/api/test_aes.h \
    test/ssl/wolfssl/tests/api/test_arc4.c \
    test/ssl/wolfssl/tests/api/test_arc4.h \
    test/ssl/wolfssl/tests/api/test_ascon.c \
    test/ssl/wolfssl/tests/api/test_ascon.h \
    test/ssl/wolfssl/tests/api/test_ascon_kats.h \
    test/ssl/wolfssl/tests/api/test_asn.c \
    test/ssl/wolfssl/tests/api/test_asn.h \
    test/ssl/wolfssl/tests/api/test_blake2.c \
    test/ssl/wolfssl/tests/api/test_blake2.h \
    test/ssl/wolfssl/tests/api/test_camellia.c \
    test/ssl/wolfssl/tests/api/test_camellia.h \
    test/ssl/wolfssl/tests/api/test_chacha.c \
    test/ssl/wolfssl/tests/api/test_chacha.h \
    test/ssl/wolfssl/tests/api/test_chacha20_poly1305.c \
    test/ssl/wolfssl/tests/api/test_chacha20_poly1305.h \
    test/ssl/wolfssl/tests/api/test_cmac.c \
    test/ssl/wolfssl/tests/api/test_cmac.h \
    test/ssl/wolfssl/tests/api/test_curve25519.c \
    test/ssl/wolfssl/tests/api/test_curve25519.h \
    test/ssl/wolfssl/tests/api/test_curve448.c \
    test/ssl/wolfssl/tests/api/test_curve448.h \
    test/ssl/wolfssl/tests/api/test_des3.c \
    test/ssl/wolfssl/tests/api/test_des3.h \
    test/ssl/wolfssl/tests/api/test_dh.c \
    test/ssl/wolfssl/tests/api/test_dh.h \
    test/ssl/wolfssl/tests/api/test_digest.h \
    test/ssl/wolfssl/tests/api/test_dsa.c \
    test/ssl/wolfssl/tests/api/test_dsa.h \
    test/ssl/wolfssl/tests/api/test_dtls.c \
    test/ssl/wolfssl/tests/api/test_dtls.h \
    test/ssl/wolfssl/tests/api/test_ecc.c \
    test/ssl/wolfssl/tests/api/test_ecc.h \
    test/ssl/wolfssl/tests/api/test_ed25519.c \
    test/ssl/wolfssl/tests/api/test_ed25519.h \
    test/ssl/wolfssl/tests/api/test_ed448.c \
    test/ssl/wolfssl/tests/api/test_ed448.h \
    test/ssl/wolfssl/tests/api/test_evp.c \
    test/ssl/wolfssl/tests/api/test_evp.h \
    test/ssl/wolfssl/tests/api/test_hash.c \
    test/ssl/wolfssl/tests/api/test_hash.h \
    test/ssl/wolfssl/tests/api/test_hmac.c \
    test/ssl/wolfssl/tests/api/test_hmac.h \
    test/ssl/wolfssl/tests/api/test_md2.c \
    test/ssl/wolfssl/tests/api/test_md2.h \
    test/ssl/wolfssl/tests/api/test_md4.c \
    test/ssl/wolfssl/tests/api/test_md4.h \
    test/ssl/wolfssl/tests/api/test_md5.c \
    test/ssl/wolfssl/tests/api/test_md5.h \
    test/ssl/wolfssl/tests/api/test_mldsa.c \
    test/ssl/wolfssl/tests/api/test_mldsa.h \
    test/ssl/wolfssl/tests/api/test_mlkem.c \
    test/ssl/wolfssl/tests/api/test_mlkem.h \
    test/ssl/wolfssl/tests/api/test_ocsp.c \
    test/ssl/wolfssl/tests/api/test_ocsp.h \
    test/ssl/wolfssl/tests/api/test_ocsp_test_blobs.h \
    test/ssl/wolfssl/tests/api/test_ossl_asn1.c \
    test/ssl/wolfssl/tests/api/test_ossl_asn1.h \
    test/ssl/wolfssl/tests/api/test_ossl_bio.c \
    test/ssl/wolfssl/tests/api/test_ossl_bio.h \
    test/ssl/wolfssl/tests/api/test_ossl_bn.c \
    test/ssl/wolfssl/tests/api/test_ossl_bn.h \
    test/ssl/wolfssl/tests/api/test_ossl_cipher.c \
    test/ssl/wolfssl/tests/api/test_ossl_cipher.h \
    test/ssl/wolfssl/tests/api/test_ossl_dgst.c \
    test/ssl/wolfssl/tests/api/test_ossl_dgst.h \
    test/ssl/wolfssl/tests/api/test_ossl_dh.c \
    test/ssl/wolfssl/tests/api/test_ossl_dh.h \
    test/ssl/wolfssl/tests/api/test_ossl_dsa.c \
    test/ssl/wolfssl/tests/api/test_ossl_dsa.h \
    test/ssl/wolfssl/tests/api/test_ossl_ec.c \
    test/ssl/wolfssl/tests/api/test_ossl_ec.h \
    test/ssl/wolfssl/tests/api/test_ossl_ecx.c \
    test/ssl/wolfssl/tests/api/test_ossl_ecx.h \
    test/ssl/wolfssl/tests/api/test_ossl_mac.c \
    test/ssl/wolfssl/tests/api/test_ossl_mac.h \
    test/ssl/wolfssl/tests/api/test_ossl_rsa.c \
    test/ssl/wolfssl/tests/api/test_ossl_rsa.h \
    test/ssl/wolfssl/tests/api/test_ossl_sk.c \
    test/ssl/wolfssl/tests/api/test_ossl_sk.h \
    test/ssl/wolfssl/tests/api/test_pkcs12.c \
    test/ssl/wolfssl/tests/api/test_pkcs12.h \
    test/ssl/wolfssl/tests/api/test_pkcs7.c \
    test/ssl/wolfssl/tests/api/test_pkcs7.h \
    test/ssl/wolfssl/tests/api/test_poly1305.c \
    test/ssl/wolfssl/tests/api/test_poly1305.h \
    test/ssl/wolfssl/tests/api/test_random.c \
    test/ssl/wolfssl/tests/api/test_random.h \
    test/ssl/wolfssl/tests/api/test_rc2.c \
    test/ssl/wolfssl/tests/api/test_rc2.h \
    test/ssl/wolfssl/tests/api/test_ripemd.c \
    test/ssl/wolfssl/tests/api/test_ripemd.h \
    test/ssl/wolfssl/tests/api/test_rsa.c \
    test/ssl/wolfssl/tests/api/test_rsa.h \
    test/ssl/wolfssl/tests/api/test_sha.c \
    test/ssl/wolfssl/tests/api/test_sha.h \
    test/ssl/wolfssl/tests/api/test_sha256.c \
    test/ssl/wolfssl/tests/api/test_sha256.h \
    test/ssl/wolfssl/tests/api/test_sha3.c \
    test/ssl/wolfssl/tests/api/test_sha3.h \
    test/ssl/wolfssl/tests/api/test_sha512.c \
    test/ssl/wolfssl/tests/api/test_sha512.h \
    test/ssl/wolfssl/tests/api/test_signature.c \
    test/ssl/wolfssl/tests/api/test_signature.h \
    test/ssl/wolfssl/tests/api/test_sm2.c \
    test/ssl/wolfssl/tests/api/test_sm2.h \
    test/ssl/wolfssl/tests/api/test_sm3.c \
    test/ssl/wolfssl/tests/api/test_sm3.h \
    test/ssl/wolfssl/tests/api/test_sm4.c \
    test/ssl/wolfssl/tests/api/test_sm4.h \
    test/ssl/wolfssl/tests/api/test_tls.c \
    test/ssl/wolfssl/tests/api/test_tls.h \
    test/ssl/wolfssl/tests/api/test_tls13.c \
    test/ssl/wolfssl/tests/api/test_tls13.h \
    test/ssl/wolfssl/tests/api/test_tls_ext.c \
    test/ssl/wolfssl/tests/api/test_tls_ext.h \
    test/ssl/wolfssl/tests/api/test_wc_encrypt.c \
    test/ssl/wolfssl/tests/api/test_wc_encrypt.h \
    test/ssl/wolfssl/tests/api/test_wolfmath.c \
    test/ssl/wolfssl/tests/api/test_wolfmath.h \
    test/ssl/wolfssl/tests/api/test_x509.c \
    test/ssl/wolfssl/tests/api/test_x509.h \
    test/ssl/wolfssl/testsuite/utils.c \
    test/ssl/wolfssl/testsuite/utils.h \
    test/ssl/wolfssl/wolfcrypt/benchmark/benchmark.c \
    test/ssl/wolfssl/wolfcrypt/benchmark/benchmark.h \
    test/ssl/wolfssl/wolfcrypt/test/test.c \
    test/ssl/wolfssl/wolfcrypt/test/test.h

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
    include/bitcoin/network/have.hpp \
    include/bitcoin/network/memory.hpp \
    include/bitcoin/network/net.hpp \
    include/bitcoin/network/preprocessor.hpp \
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
    include/bitcoin/network/channels/channel_rpc.hpp \
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

include_bitcoin_network_impl_channelsdir = ${includedir}/bitcoin/network/impl/channels
include_bitcoin_network_impl_channels_HEADERS = \
    include/bitcoin/network/impl/channels/channel_rpc.ipp

include_bitcoin_network_impl_messagesdir = ${includedir}/bitcoin/network/impl/messages
include_bitcoin_network_impl_messages_HEADERS = \
    include/bitcoin/network/impl/messages/json_body.ipp

include_bitcoin_network_impl_messages_rpcdir = ${includedir}/bitcoin/network/impl/messages/rpc
include_bitcoin_network_impl_messages_rpc_HEADERS = \
    include/bitcoin/network/impl/messages/rpc/broadcaster.ipp \
    include/bitcoin/network/impl/messages/rpc/dispatcher.ipp

include_bitcoin_network_impl_protocolsdir = ${includedir}/bitcoin/network/impl/protocols
include_bitcoin_network_impl_protocols_HEADERS = \
    include/bitcoin/network/impl/protocols/protocol_rpc.ipp

include_bitcoin_network_interfacesdir = ${includedir}/bitcoin/network/interfaces
include_bitcoin_network_interfaces_HEADERS = \
    include/bitcoin/network/interfaces/http.hpp \
    include/bitcoin/network/interfaces/interfaces.hpp \
    include/bitcoin/network/interfaces/peer_broadcast.hpp \
    include/bitcoin/network/interfaces/peer_dispatch.hpp

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
    include/bitcoin/network/messages/http_body.hpp \
    include/bitcoin/network/messages/http_method.hpp \
    include/bitcoin/network/messages/json_body.hpp \
    include/bitcoin/network/messages/messages.hpp

include_bitcoin_network_messages_httpdir = ${includedir}/bitcoin/network/messages/http
include_bitcoin_network_messages_http_HEADERS = \
    include/bitcoin/network/messages/http/fields.hpp \
    include/bitcoin/network/messages/http/http.hpp

include_bitcoin_network_messages_http_enumsdir = ${includedir}/bitcoin/network/messages/http/enums
include_bitcoin_network_messages_http_enums_HEADERS = \
    include/bitcoin/network/messages/http/enums/magic_numbers.hpp \
    include/bitcoin/network/messages/http/enums/media_type.hpp \
    include/bitcoin/network/messages/http/enums/status.hpp \
    include/bitcoin/network/messages/http/enums/target.hpp \
    include/bitcoin/network/messages/http/enums/verb.hpp

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

include_bitcoin_network_messages_rpcdir = ${includedir}/bitcoin/network/messages/rpc
include_bitcoin_network_messages_rpc_HEADERS = \
    include/bitcoin/network/messages/rpc/any.hpp \
    include/bitcoin/network/messages/rpc/body.hpp \
    include/bitcoin/network/messages/rpc/broadcaster.hpp \
    include/bitcoin/network/messages/rpc/dispatcher.hpp \
    include/bitcoin/network/messages/rpc/method.hpp \
    include/bitcoin/network/messages/rpc/model.hpp \
    include/bitcoin/network/messages/rpc/publish.hpp \
    include/bitcoin/network/messages/rpc/rpc.hpp \
    include/bitcoin/network/messages/rpc/types.hpp

include_bitcoin_network_messages_rpc_enumsdir = ${includedir}/bitcoin/network/messages/rpc/enums
include_bitcoin_network_messages_rpc_enums_HEADERS = \
    include/bitcoin/network/messages/rpc/enums/grouping.hpp \
    include/bitcoin/network/messages/rpc/enums/version.hpp

include_bitcoin_network_netdir = ${includedir}/bitcoin/network/net
include_bitcoin_network_net_HEADERS = \
    include/bitcoin/network/net/acceptor.hpp \
    include/bitcoin/network/net/connector.hpp \
    include/bitcoin/network/net/connector_socks.hpp \
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
    include/bitcoin/network/protocols/protocol_rpc.hpp \
    include/bitcoin/network/protocols/protocol_seed_209.hpp \
    include/bitcoin/network/protocols/protocol_version_106.hpp \
    include/bitcoin/network/protocols/protocol_version_70001.hpp \
    include/bitcoin/network/protocols/protocol_version_70002.hpp \
    include/bitcoin/network/protocols/protocol_version_70016.hpp \
    include/bitcoin/network/protocols/protocol_ws.hpp \
    include/bitcoin/network/protocols/protocols.hpp

include_bitcoin_network_sessionsdir = ${includedir}/bitcoin/network/sessions
include_bitcoin_network_sessions_HEADERS = \
    include/bitcoin/network/sessions/session.hpp \
    include/bitcoin/network/sessions/session_inbound.hpp \
    include/bitcoin/network/sessions/session_manual.hpp \
    include/bitcoin/network/sessions/session_outbound.hpp \
    include/bitcoin/network/sessions/session_peer.hpp \
    include/bitcoin/network/sessions/session_seed.hpp \
    include/bitcoin/network/sessions/session_server.hpp \
    include/bitcoin/network/sessions/sessions.hpp

include_bitcoin_ssl_openssldir = ${includedir}/bitcoin/ssl/openssl
include_bitcoin_ssl_openssl_HEADERS = \
    include/bitcoin/ssl/openssl/conf.h \
    include/bitcoin/ssl/openssl/dh.h \
    include/bitcoin/ssl/openssl/engine.h \
    include/bitcoin/ssl/openssl/err.h \
    include/bitcoin/ssl/openssl/openssl.h \
    include/bitcoin/ssl/openssl/rsa.h \
    include/bitcoin/ssl/openssl/ssl.h \
    include/bitcoin/ssl/openssl/x509.h \
    include/bitcoin/ssl/openssl/x509v3.h

include_bitcoin_ssl_wolfssldir = ${includedir}/bitcoin/ssl/wolfssl
include_bitcoin_ssl_wolfssl_HEADERS = \
    include/bitcoin/ssl/wolfssl/callbacks.h \
    include/bitcoin/ssl/wolfssl/certs_test.h \
    include/bitcoin/ssl/wolfssl/error-ssl.h \
    include/bitcoin/ssl/wolfssl/internal.h \
    include/bitcoin/ssl/wolfssl/ocsp.h \
    include/bitcoin/ssl/wolfssl/options.h \
    include/bitcoin/ssl/wolfssl/ssl.h \
    include/bitcoin/ssl/wolfssl/test.h \
    include/bitcoin/ssl/wolfssl/version.h \
    include/bitcoin/ssl/wolfssl/wolfio.h \
    include/bitcoin/ssl/wolfssl/wolfssl.h

include_bitcoin_ssl_wolfssl_openssldir = ${includedir}/bitcoin/ssl/wolfssl/openssl
include_bitcoin_ssl_wolfssl_openssl_HEADERS = \
    include/bitcoin/ssl/wolfssl/openssl/aes.h \
    include/bitcoin/ssl/wolfssl/openssl/asn1.h \
    include/bitcoin/ssl/wolfssl/openssl/bio.h \
    include/bitcoin/ssl/wolfssl/openssl/bn.h \
    include/bitcoin/ssl/wolfssl/openssl/buffer.h \
    include/bitcoin/ssl/wolfssl/openssl/cmac.h \
    include/bitcoin/ssl/wolfssl/openssl/compat_types.h \
    include/bitcoin/ssl/wolfssl/openssl/conf.h \
    include/bitcoin/ssl/wolfssl/openssl/crypto.h \
    include/bitcoin/ssl/wolfssl/openssl/des.h \
    include/bitcoin/ssl/wolfssl/openssl/dh.h \
    include/bitcoin/ssl/wolfssl/openssl/dsa.h \
    include/bitcoin/ssl/wolfssl/openssl/ec.h \
    include/bitcoin/ssl/wolfssl/openssl/ec25519.h \
    include/bitcoin/ssl/wolfssl/openssl/ec448.h \
    include/bitcoin/ssl/wolfssl/openssl/ecdh.h \
    include/bitcoin/ssl/wolfssl/openssl/ecdsa.h \
    include/bitcoin/ssl/wolfssl/openssl/ed25519.h \
    include/bitcoin/ssl/wolfssl/openssl/ed448.h \
    include/bitcoin/ssl/wolfssl/openssl/engine.h \
    include/bitcoin/ssl/wolfssl/openssl/err.h \
    include/bitcoin/ssl/wolfssl/openssl/evp.h \
    include/bitcoin/ssl/wolfssl/openssl/fips_rand.h \
    include/bitcoin/ssl/wolfssl/openssl/hmac.h \
    include/bitcoin/ssl/wolfssl/openssl/kdf.h \
    include/bitcoin/ssl/wolfssl/openssl/lhash.h \
    include/bitcoin/ssl/wolfssl/openssl/md4.h \
    include/bitcoin/ssl/wolfssl/openssl/md5.h \
    include/bitcoin/ssl/wolfssl/openssl/modes.h \
    include/bitcoin/ssl/wolfssl/openssl/obj_mac.h \
    include/bitcoin/ssl/wolfssl/openssl/objects.h \
    include/bitcoin/ssl/wolfssl/openssl/opensslv.h \
    include/bitcoin/ssl/wolfssl/openssl/pem.h \
    include/bitcoin/ssl/wolfssl/openssl/pkcs12.h \
    include/bitcoin/ssl/wolfssl/openssl/pkcs7.h \
    include/bitcoin/ssl/wolfssl/openssl/rand.h \
    include/bitcoin/ssl/wolfssl/openssl/rc4.h \
    include/bitcoin/ssl/wolfssl/openssl/ripemd.h \
    include/bitcoin/ssl/wolfssl/openssl/rsa.h \
    include/bitcoin/ssl/wolfssl/openssl/sha.h \
    include/bitcoin/ssl/wolfssl/openssl/sha3.h \
    include/bitcoin/ssl/wolfssl/openssl/ssl.h \
    include/bitcoin/ssl/wolfssl/openssl/stack.h \
    include/bitcoin/ssl/wolfssl/openssl/tls1.h \
    include/bitcoin/ssl/wolfssl/openssl/x509.h \
    include/bitcoin/ssl/wolfssl/openssl/x509_vfy.h \
    include/bitcoin/ssl/wolfssl/openssl/x509v3.h

include_bitcoin_ssl_wolfssl_wolfcryptdir = ${includedir}/bitcoin/ssl/wolfssl/wolfcrypt
include_bitcoin_ssl_wolfssl_wolfcrypt_HEADERS = \
    include/bitcoin/ssl/wolfssl/wolfcrypt/aes.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/arc4.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/ascon.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/asn.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/asn_public.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/blake2.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/camellia.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/chacha.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/chacha20_poly1305.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/cmac.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/coding.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/cpuid.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/cryptocb.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/curve25519.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/curve448.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/des3.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/dh.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/dsa.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/ecc.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/ed25519.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/ed448.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/error-crypt.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/fe_operations.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/fips_test.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/hash.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/hmac.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/hpke.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/kdf.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/libwolfssl_sources.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/logging.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/md2.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/md4.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/md5.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/mem_track.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/memory.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/misc.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/oid_sum.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/pkcs12.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/pkcs7.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/poly1305.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/pwdbased.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/random.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/rc2.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/ripemd.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/rsa.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/settings.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/sha.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/sha256.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/sha3.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/sha512.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/signature.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/siphash.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/sp_int.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/srp.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/types.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/user_settings.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/visibility.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/wc_encrypt.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/wc_port.h \
    include/bitcoin/ssl/wolfssl/wolfcrypt/wolfmath.h

