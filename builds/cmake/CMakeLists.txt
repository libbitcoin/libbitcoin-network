###############################################################################
#  Copyright (c) 2014-2025 libbitcoin-network developers (see COPYING).
#
#         GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
#
###############################################################################

# libbitcoin-network project configuration.
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(libbitcoin-network LANGUAGES C CXX)

enable_testing()

list( APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules" )
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckLinkerFlag)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (MSVC)
  set( CANONICAL_LIB_NAME libbitcoin-network )
else ()
  set( CANONICAL_LIB_NAME bitcoin-network )
  find_package( PkgConfig REQUIRED )

  set( prefix "${CMAKE_PREFIX_PATH}" )
  set( exec_prefix "\${prefix}" )
  set( libdir "\${exec_prefix}/lib" )
  set( includedir "\${exec_prefix}/include" )

  set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib" )

  set( PACKAGE_VERSION "4.0.0" )
  set( VERSION "${PACKAGE_VERSION}" )
endif ()

set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

# Add compiler options
#------------------------------------------------------------------------------
# Warn on all stuff.
check_cxx_compiler_flag( "-Wall" HAS_FLAG_WALL )
if ( HAS_FLAG_WALL )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wall> )
else()
    message( FATAL_ERROR "Compiler does not support -Wall" )
endif()

# Warn on extra stuff.
check_cxx_compiler_flag( "-Wextra" HAS_FLAG_WEXTRA )
if ( HAS_FLAG_WEXTRA )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wextra> )
else()
    message( FATAL_ERROR "Compiler does not support -Wextra" )
endif()

# Disallow warning on style order of declarations.
check_cxx_compiler_flag( "-Wno-reorder" HAS_FLAG_WNO-REORDER )
if ( HAS_FLAG_WNO-REORDER )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder> )
else()
    message( FATAL_ERROR "Compiler does not support -Wno-reorder" )
endif()

# Suppress warning for incomplete field initialization.
check_cxx_compiler_flag( "-Wno-missing-field-initializers" HAS_FLAG_WNO-MISSING-FIELD-INITIALIZERS )
if ( HAS_FLAG_WNO-MISSING-FIELD-INITIALIZERS )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-field-initializers> )
else()
    message( FATAL_ERROR "Compiler does not support -Wno-missing-field-initializers" )
endif()

# Conform to style.
check_cxx_compiler_flag( "-Wno-missing-braces" HAS_FLAG_WNO-MISSING-BRACES )
if ( HAS_FLAG_WNO-MISSING-BRACES )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-braces> )
else()
    message( FATAL_ERROR "Compiler does not support -Wno-missing-braces" )
endif()

# Ignore comments within comments or commenting of backslash extended lines.
check_cxx_compiler_flag( "-Wno-comment" HAS_FLAG_WNO-COMMENT )
if ( HAS_FLAG_WNO-COMMENT )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wno-comment> )
else()
    message( FATAL_ERROR "Compiler does not support -Wno-comment" )
endif()

# Suppress warning for copy of implicitly generated copy constructor.
check_cxx_compiler_flag( "-Wno-deprecated-copy" HAS_FLAG_WNO-DEPRECATED-COPY )
if ( HAS_FLAG_WNO-DEPRECATED-COPY )
    add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-copy> )
else()
    message( FATAL_ERROR "Compiler does not support -Wno-deprecated-copy" )
endif()

# Conflict in stdlib under clang.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    check_cxx_compiler_flag( "-Wno-mismatched-tags" HAS_FLAG_WNO-MISMATCHED-TAGS )
    if ( HAS_FLAG_WNO-MISMATCHED-TAGS )
        add_compile_options( $<$<COMPILE_LANGUAGE:CXX>:-Wno-mismatched-tags> )
    else()
        message( FATAL_ERROR "Compiler does not support -Wno-mismatched-tags" )
    endif()
endif()

# Implement -Dpkgconfigdir and output ${pkgconfigdir}.
#------------------------------------------------------------------------------
set( pkgconfigdir "${libdir}/pkgconfig" CACHE PATH "Path to pkgconfig directory." )

# Implement -Dwith-tests and declare with-tests.
#------------------------------------------------------------------------------
set( with-tests "yes" CACHE BOOL "Compile with unit tests." )

# Implement -Dwith-ssl and define WOLFSSL_USER_SETTINGS and output ${ssl}.
#------------------------------------------------------------------------------
set( with-ssl "yes" CACHE BOOL "Compile with internal ssl." )

if (with-ssl)
    add_definitions( -DWOLFSSL_USER_SETTINGS )
endif()

if (with-ssl)
    set( ssl "-DWOLFSSL_USER_SETTINGS" )
endif()

# Implement -Denable-ndebug and define NDEBUG.
#------------------------------------------------------------------------------
set( enable-ndebug "yes" CACHE BOOL "Compile without debug assertions." )

if (enable-ndebug)
    add_definitions( -DNDEBUG )
endif()

# Inherit -Denable-shared and define BOOST_ALL_DYN_LINK.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    add_definitions( -DBOOST_ALL_DYN_LINK )
endif()

if (BUILD_SHARED_LIBS)
    set( Boost_USE_STATIC_LIBS "off" )
else()
    set( Boost_USE_STATIC_LIBS "on" )
endif()

# Find boost
#------------------------------------------------------------------------------
find_package( Boost 1.86.0 REQUIRED COMPONENTS
    unit_test_framework )

set( boost_unit_test_framework_LIBS "-lboost_unit_test_framework" )

set( Boost_LIBRARY_DIR $<IF:$<BOOL:${enable-ndebug}>,"${Boost_LIBRARY_DIR_RELEASE}","${Boost_LIBRARY_DIR_DEBUG}">)

set( boost_CPPFLAGS "-I${Boost_INCLUDE_DIR}" )
set( boost_LDFLAGS "-L${Boost_LIBRARY_DIR}" )

# Find bitcoin-system
#------------------------------------------------------------------------------
find_package( Bitcoin-System 4.0.0 REQUIRED )

# Define project common includes for build.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    set( bitcoin_system_FOR_BUILD_INCLUDE_DIRS ${bitcoin_system_INCLUDE_DIRS} CACHE STRING "Placeholder" )
else()
    set( bitcoin_system_FOR_BUILD_INCLUDE_DIRS ${bitcoin_system_STATIC_INCLUDE_DIRS} CACHE STRING "Placeholder" )
endif()

# Define project common includes directories
#------------------------------------------------------------------------------
include_directories( SYSTEM
    ${bitcoin_system_FOR_BUILD_INCLUDE_DIRS} )

# Define project common library directories for build.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    set( bitcoin_system_FOR_BUILD_LIBRARY_DIRS ${bitcoin_system_LIBRARY_DIRS} CACHE STRING "Placeholder" )
else()
    set( bitcoin_system_FOR_BUILD_LIBRARY_DIRS ${bitcoin_system_STATIC_LIBRARY_DIRS} CACHE STRING "Placeholder" )
endif()

# Define project common library directories
#------------------------------------------------------------------------------
link_directories(
    ${bitcoin_system_FOR_BUILD_LIBRARY_DIRS} )

# Define project common linker flags.
#------------------------------------------------------------------------------
check_linker_flag(CXX "-no_fixup_chains" "no_fixup_chains_FOUND")

if (no_fixup_chains_FOUND)
    add_link_options("-no_fixup_chains")
endif()

check_linker_flag(CXX "-fstack-protector" "fstack_protector_FOUND")

if (fstack_protector_FOUND)
    add_link_options("-fstack-protector")
endif()

check_linker_flag(CXX "-fstack-protector-all" "fstack_protector_all_FOUND")

if (fstack_protector_all_FOUND)
    add_link_options("-fstack-protector-all")
endif()


# Define common library usage for build.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    set( bitcoin_system_FOR_BUILD_LIBRARIES ${bitcoin_system_LIBRARIES} CACHE STRING "Placeholder" )
else()
    set( bitcoin_system_FOR_BUILD_LIBRARIES ${bitcoin_system_STATIC_LIBRARIES} CACHE STRING "Placeholder" )
endif()

# Define project common libraries.
#------------------------------------------------------------------------------
link_libraries(
    ${bitcoin_system_FOR_BUILD_LIBRARIES} )

# Define ${CANONICAL_LIB_NAME} project.
#------------------------------------------------------------------------------
add_library( ${CANONICAL_LIB_NAME}
    "../../src/error.cpp"
    "../../src/memory.cpp"
    "../../src/net.cpp"
    "../../src/settings.cpp"
    "../../src/async/thread.cpp"
    "../../src/async/threadpool.cpp"
    "../../src/async/time.cpp"
    "../../src/channels/channel.cpp"
    "../../src/channels/channel_http.cpp"
    "../../src/channels/channel_peer.cpp"
    "../../src/channels/channel_ws.cpp"
    "../../src/config/address.cpp"
    "../../src/config/authority.cpp"
    "../../src/config/endpoint.cpp"
    "../../src/config/utilities.cpp"
    "../../src/log/capture.cpp"
    "../../src/log/logger.cpp"
    "../../src/log/reporter.cpp"
    "../../src/messages/http_body.cpp"
    "../../src/messages/http/fields.cpp"
    "../../src/messages/http/enums/media_type.cpp"
    "../../src/messages/http/enums/target.cpp"
    "../../src/messages/peer/message.cpp"
    "../../src/messages/peer/detail/address.cpp"
    "../../src/messages/peer/detail/address_item.cpp"
    "../../src/messages/peer/detail/alert.cpp"
    "../../src/messages/peer/detail/alert_item.cpp"
    "../../src/messages/peer/detail/block.cpp"
    "../../src/messages/peer/detail/bloom_filter_add.cpp"
    "../../src/messages/peer/detail/bloom_filter_clear.cpp"
    "../../src/messages/peer/detail/bloom_filter_load.cpp"
    "../../src/messages/peer/detail/client_filter.cpp"
    "../../src/messages/peer/detail/client_filter_checkpoint.cpp"
    "../../src/messages/peer/detail/client_filter_headers.cpp"
    "../../src/messages/peer/detail/compact_block.cpp"
    "../../src/messages/peer/detail/compact_block_item.cpp"
    "../../src/messages/peer/detail/compact_transactions.cpp"
    "../../src/messages/peer/detail/fee_filter.cpp"
    "../../src/messages/peer/detail/get_address.cpp"
    "../../src/messages/peer/detail/get_blocks.cpp"
    "../../src/messages/peer/detail/get_client_filter_checkpoint.cpp"
    "../../src/messages/peer/detail/get_client_filter_headers.cpp"
    "../../src/messages/peer/detail/get_client_filters.cpp"
    "../../src/messages/peer/detail/get_compact_transactions.cpp"
    "../../src/messages/peer/detail/get_data.cpp"
    "../../src/messages/peer/detail/get_headers.cpp"
    "../../src/messages/peer/detail/headers.cpp"
    "../../src/messages/peer/detail/heading.cpp"
    "../../src/messages/peer/detail/inventory.cpp"
    "../../src/messages/peer/detail/inventory_item.cpp"
    "../../src/messages/peer/detail/memory_pool.cpp"
    "../../src/messages/peer/detail/merkle_block.cpp"
    "../../src/messages/peer/detail/not_found.cpp"
    "../../src/messages/peer/detail/ping.cpp"
    "../../src/messages/peer/detail/pong.cpp"
    "../../src/messages/peer/detail/reject.cpp"
    "../../src/messages/peer/detail/send_address_v2.cpp"
    "../../src/messages/peer/detail/send_compact.cpp"
    "../../src/messages/peer/detail/send_headers.cpp"
    "../../src/messages/peer/detail/transaction.cpp"
    "../../src/messages/peer/detail/version.cpp"
    "../../src/messages/peer/detail/version_acknowledge.cpp"
    "../../src/messages/peer/detail/witness_tx_id_relay.cpp"
    "../../src/messages/rpc/body.cpp"
    "../../src/messages/rpc/model.cpp"
    "../../src/net/acceptor.cpp"
    "../../src/net/connector.cpp"
    "../../src/net/connector_socks.cpp"
    "../../src/net/deadline.cpp"
    "../../src/net/hosts.cpp"
    "../../src/net/proxy.cpp"
    "../../src/net/socket.cpp"
    "../../src/protocols/protocol.cpp"
    "../../src/protocols/protocol_address_in_209.cpp"
    "../../src/protocols/protocol_address_out_209.cpp"
    "../../src/protocols/protocol_alert_311.cpp"
    "../../src/protocols/protocol_http.cpp"
    "../../src/protocols/protocol_peer.cpp"
    "../../src/protocols/protocol_ping_106.cpp"
    "../../src/protocols/protocol_ping_60001.cpp"
    "../../src/protocols/protocol_reject_70002.cpp"
    "../../src/protocols/protocol_seed_209.cpp"
    "../../src/protocols/protocol_version_106.cpp"
    "../../src/protocols/protocol_version_70001.cpp"
    "../../src/protocols/protocol_version_70002.cpp"
    "../../src/protocols/protocol_version_70016.cpp"
    "../../src/sessions/session.cpp"
    "../../src/sessions/session_inbound.cpp"
    "../../src/sessions/session_manual.cpp"
    "../../src/sessions/session_outbound.cpp"
    "../../src/sessions/session_peer.cpp"
    "../../src/sessions/session_seed.cpp"
    "../../src/sessions/session_server.cpp"
    "../../src/ssl/wolfssl/internal.c"
    "../../src/ssl/wolfssl/keys.c"
    "../../src/ssl/wolfssl/ssl.c"
    "../../src/ssl/wolfssl/tls.c"
    "../../src/ssl/wolfssl/tls13.c"
    "../../src/ssl/wolfssl/wolfio.c"
    "../../src/ssl/wolfssl/src/bio.c"
    "../../src/ssl/wolfssl/src/conf.c"
    "../../src/ssl/wolfssl/src/evp.c"
    "../../src/ssl/wolfssl/src/pk.c"
    "../../src/ssl/wolfssl/src/ssl_asn1.c"
    "../../src/ssl/wolfssl/src/ssl_bn.c"
    "../../src/ssl/wolfssl/src/ssl_certman.c"
    "../../src/ssl/wolfssl/src/ssl_crypto.c"
    "../../src/ssl/wolfssl/src/ssl_load.c"
    "../../src/ssl/wolfssl/src/ssl_misc.c"
    "../../src/ssl/wolfssl/src/ssl_p7p12.c"
    "../../src/ssl/wolfssl/src/ssl_sess.c"
    "../../src/ssl/wolfssl/src/ssl_sk.c"
    "../../src/ssl/wolfssl/src/x509.c"
    "../../src/ssl/wolfssl/src/x509_str.c"
    "../../src/ssl/wolfssl/wolfcrypt/asn.c"
    "../../src/ssl/wolfssl/wolfcrypt/chacha.c"
    "../../src/ssl/wolfssl/wolfcrypt/chacha20_poly1305.c"
    "../../src/ssl/wolfssl/wolfcrypt/coding.c"
    "../../src/ssl/wolfssl/wolfcrypt/cryptocb.c"
    "../../src/ssl/wolfssl/wolfcrypt/curve25519.c"
    "../../src/ssl/wolfssl/wolfcrypt/ecc.c"
    "../../src/ssl/wolfssl/wolfcrypt/error.c"
    "../../src/ssl/wolfssl/wolfcrypt/fe_operations.c"
    "../../src/ssl/wolfssl/wolfcrypt/ge_operations.c"
    "../../src/ssl/wolfssl/wolfcrypt/hash.c"
    "../../src/ssl/wolfssl/wolfcrypt/hmac.c"
    "../../src/ssl/wolfssl/wolfcrypt/kdf.c"
    "../../src/ssl/wolfssl/wolfcrypt/logging.c"
    "../../src/ssl/wolfssl/wolfcrypt/memory.c"
    "../../src/ssl/wolfssl/wolfcrypt/poly1305.c"
    "../../src/ssl/wolfssl/wolfcrypt/random.c"
    "../../src/ssl/wolfssl/wolfcrypt/sha256.c"
    "../../src/ssl/wolfssl/wolfcrypt/signature.c"
    "../../src/ssl/wolfssl/wolfcrypt/sp_int.c"
    "../../src/ssl/wolfssl/wolfcrypt/wc_encrypt.c"
    "../../src/ssl/wolfssl/wolfcrypt/wc_port.c"
    "../../src/ssl/wolfssl/wolfcrypt/wolfmath.c" )

# ${CANONICAL_LIB_NAME} project specific include directory normalization for build.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    set( bitcoin_system_FOR_BUILD_INCLUDE_DIRS ${bitcoin_system_INCLUDE_DIRS} CACHE STRING "Placeholder" )
else()
    set( bitcoin_system_FOR_BUILD_INCLUDE_DIRS ${bitcoin_system_STATIC_INCLUDE_DIRS} CACHE STRING "Placeholder" )
endif()

# ${CANONICAL_LIB_NAME} project specific include directories.
#------------------------------------------------------------------------------
target_include_directories( ${CANONICAL_LIB_NAME} PRIVATE
    "../../include"
    "../../include/bitcoin/ssl"
    ${bitcoin_system_FOR_BUILD_INCLUDE_DIRS} )

target_include_directories( ${CANONICAL_LIB_NAME} PUBLIC
"../../include" )

# ${CANONICAL_LIB_NAME} project specific libraries noramalization for build.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    set( bitcoin_system_FOR_BUILD_LIBRARIES ${bitcoin_system_LIBRARIES} CACHE STRING "Placeholder" )
else()
    set( bitcoin_system_FOR_BUILD_LIBRARIES ${bitcoin_system_STATIC_LIBRARIES} CACHE STRING "Placeholder" )
endif()

# ${CANONICAL_LIB_NAME} project specific libraries/linker flags.
#------------------------------------------------------------------------------
target_link_libraries( ${CANONICAL_LIB_NAME}
    ${bitcoin_system_FOR_BUILD_LIBRARIES} )

# Define libbitcoin-network-test project.
#------------------------------------------------------------------------------
if (with-tests)
    add_executable( libbitcoin-network-test
        "../../test/error.cpp"
        "../../test/main.cpp"
        "../../test/memory.cpp"
        "../../test/net.cpp"
        "../../test/settings.cpp"
        "../../test/test.cpp"
        "../../test/test.hpp"
        "../../test/async/desubscriber.cpp"
        "../../test/async/enable_shared_from_base.cpp"
        "../../test/async/subscriber.cpp"
        "../../test/async/thread.cpp"
        "../../test/async/threadpool.cpp"
        "../../test/async/time.cpp"
        "../../test/async/unsubscriber.cpp"
        "../../test/async/races/race_all.cpp"
        "../../test/async/races/race_quality.cpp"
        "../../test/async/races/race_speed.cpp"
        "../../test/async/races/race_unity.cpp"
        "../../test/async/races/race_volume.cpp"
        "../../test/channels/channel.cpp"
        "../../test/channels/channel_http.cpp"
        "../../test/channels/channel_peer.cpp"
        "../../test/channels/channel_rpc.cpp"
        "../../test/channels/channel_ws.cpp"
        "../../test/config/address.cpp"
        "../../test/config/authority.cpp"
        "../../test/config/endpoint.cpp"
        "../../test/config/utilities.cpp"
        "../../test/log/timer.cpp"
        "../../test/log/tracker.cpp"
        "../../test/messages/http_body_reader.cpp"
        "../../test/messages/http_body_writer.cpp"
        "../../test/messages/json_body_reader.cpp"
        "../../test/messages/json_body_writer.cpp"
        "../../test/messages/http/fields.cpp"
        "../../test/messages/http/method.cpp"
        "../../test/messages/http/enums/magic_numbers.cpp"
        "../../test/messages/http/enums/media_type.cpp"
        "../../test/messages/http/enums/target.cpp"
        "../../test/messages/peer/heading.cpp"
        "../../test/messages/peer/message.cpp"
        "../../test/messages/peer/detail/address.cpp"
        "../../test/messages/peer/detail/address_item.cpp"
        "../../test/messages/peer/detail/alert.cpp"
        "../../test/messages/peer/detail/alert_item.cpp"
        "../../test/messages/peer/detail/block.cpp"
        "../../test/messages/peer/detail/bloom_filter_add.cpp"
        "../../test/messages/peer/detail/bloom_filter_clear.cpp"
        "../../test/messages/peer/detail/bloom_filter_load.cpp"
        "../../test/messages/peer/detail/client_filter.cpp"
        "../../test/messages/peer/detail/client_filter_checkpoint.cpp"
        "../../test/messages/peer/detail/client_filter_headers.cpp"
        "../../test/messages/peer/detail/compact_block.cpp"
        "../../test/messages/peer/detail/compact_block_item.cpp"
        "../../test/messages/peer/detail/compact_transactions.cpp"
        "../../test/messages/peer/detail/fee_filter.cpp"
        "../../test/messages/peer/detail/get_address.cpp"
        "../../test/messages/peer/detail/get_blocks.cpp"
        "../../test/messages/peer/detail/get_client_filter_checkpoint.cpp"
        "../../test/messages/peer/detail/get_client_filter_headers.cpp"
        "../../test/messages/peer/detail/get_client_filters.cpp"
        "../../test/messages/peer/detail/get_compact_transactions.cpp"
        "../../test/messages/peer/detail/get_data.cpp"
        "../../test/messages/peer/detail/get_headers.cpp"
        "../../test/messages/peer/detail/headers.cpp"
        "../../test/messages/peer/detail/inventory.cpp"
        "../../test/messages/peer/detail/inventory_item.cpp"
        "../../test/messages/peer/detail/memory_pool.cpp"
        "../../test/messages/peer/detail/merkle_block.cpp"
        "../../test/messages/peer/detail/not_found.cpp"
        "../../test/messages/peer/detail/ping.cpp"
        "../../test/messages/peer/detail/pong.cpp"
        "../../test/messages/peer/detail/reject.cpp"
        "../../test/messages/peer/detail/send_address_v2.cpp"
        "../../test/messages/peer/detail/send_compact.cpp"
        "../../test/messages/peer/detail/send_headers.cpp"
        "../../test/messages/peer/detail/transaction.cpp"
        "../../test/messages/peer/detail/version.cpp"
        "../../test/messages/peer/detail/version_acknowledge.cpp"
        "../../test/messages/peer/detail/witness_tx_id_relay.cpp"
        "../../test/messages/peer/enums/identifier.cpp"
        "../../test/messages/peer/enums/level.cpp"
        "../../test/messages/peer/enums/magic_numbers.cpp"
        "../../test/messages/peer/enums/service.cpp"
        "../../test/messages/rpc/any.cpp"
        "../../test/messages/rpc/body_reader.cpp"
        "../../test/messages/rpc/body_writer.cpp"
        "../../test/messages/rpc/broadcaster.cpp"
        "../../test/messages/rpc/dispatcher.cpp"
        "../../test/messages/rpc/method.cpp"
        "../../test/messages/rpc/types.cpp"
        "../../test/net/acceptor.cpp"
        "../../test/net/connector.cpp"
        "../../test/net/connector_socks.cpp"
        "../../test/net/deadline.cpp"
        "../../test/net/hosts.cpp"
        "../../test/net/proxy.cpp"
        "../../test/net/socket.cpp"
        "../../test/protocols/protocol.cpp"
        "../../test/protocols/protocol_address_in_209.cpp"
        "../../test/protocols/protocol_address_out_209.cpp"
        "../../test/protocols/protocol_alert_311.cpp"
        "../../test/protocols/protocol_http.cpp"
        "../../test/protocols/protocol_peer.cpp"
        "../../test/protocols/protocol_ping_106.cpp"
        "../../test/protocols/protocol_ping_60001.cpp"
        "../../test/protocols/protocol_reject_70002.cpp"
        "../../test/protocols/protocol_seed_209.cpp"
        "../../test/protocols/protocol_version_106.cpp"
        "../../test/protocols/protocol_version_70001.cpp"
        "../../test/protocols/protocol_version_70002.cpp"
        "../../test/protocols/protocol_version_70016.cpp"
        "../../test/sessions/session.cpp"
        "../../test/sessions/session_html.cpp"
        "../../test/sessions/session_inbound.cpp"
        "../../test/sessions/session_manual.cpp"
        "../../test/sessions/session_outbound.cpp"
        "../../test/sessions/session_seed.cpp"
        "../../test/sessions/session_server.cpp"
        "../../test/ssl/wolfssl.cpp"
        "../../test/ssl/vectors/certs/ca-ecc-cert.pem"
        "../../test/ssl/vectors/certs/ca-ecc384-cert.pem"
        "../../test/ssl/vectors/certs/ca-ecc384-key.der"
        "../../test/ssl/vectors/certs/ca-ecc384-key.pem"
        "../../test/ssl/vectors/certs/client-ecc384-cert.pem"
        "../../test/ssl/vectors/certs/client-ecc384-key.pem"
        "../../test/ssl/vectors/certs/ecc-key.der"
        "../../test/ssl/vectors/certs/ecc-key.pem"
        "../../test/ssl/vectors/certs/ecc-privOnlyCert.pem"
        "../../test/ssl/vectors/certs/ecc-privOnlyKey.pem"
        "../../test/ssl/vectors/certs/entity-no-ca-bool-cert.pem"
        "../../test/ssl/vectors/certs/entity-no-ca-bool-key.pem"
        "../../test/ssl/vectors/certs/server-cert.pem"
        "../../test/ssl/vectors/certs/server-ecc-rsa.pem"
        "../../test/ssl/vectors/certs/server-ecc.pem"
        "../../test/ssl/vectors/certs/server-ecc384-cert.pem"
        "../../test/ssl/vectors/certs/server-ecc384-key.pem"
        "../../test/ssl/vectors/certs/server-key.pem"
        "../../test/ssl/vectors/certs/test/server-cert-ecc-badsig.pem"
        "../../test/ssl/vectors/certs/test/server-cert-rsa-badsig.pem"
        "../../test/ssl/vectors/certs/test/server-goodalt.pem"
        "../../test/ssl/vectors/certs/test/server-goodaltwild.pem"
        "../../test/ssl/vectors/certs/test/server-goodcn.pem"
        "../../test/ssl/vectors/certs/test/server-goodcnwild.pem"
        "../../test/ssl/vectors/certs/test/server-localhost.pem"
        "../../test/ssl/vectors/tests/test-chains.conf"
        "../../test/ssl/vectors/tests/test-dhprime.conf"
        "../../test/ssl/vectors/tests/test-fails.conf"
        "../../test/ssl/vectors/tests/test-tls13-down.conf"
        "../../test/ssl/vectors/tests/test-tls13-ecc.conf"
        "../../test/ssl/vectors/tests/test-tls13.conf"
        "../../test/ssl/vectors/tests/test.conf"
        "../../test/ssl/wolfssl/examples/client/client.c"
        "../../test/ssl/wolfssl/examples/client/client.h"
        "../../test/ssl/wolfssl/examples/server/server.c"
        "../../test/ssl/wolfssl/examples/server/server.h"
        "../../test/ssl/wolfssl/tests/api.c"
        "../../test/ssl/wolfssl/tests/quic.c"
        "../../test/ssl/wolfssl/tests/srp.c"
        "../../test/ssl/wolfssl/tests/suites.c"
        "../../test/ssl/wolfssl/tests/unit.c"
        "../../test/ssl/wolfssl/tests/unit.h"
        "../../test/ssl/wolfssl/tests/utils.c"
        "../../test/ssl/wolfssl/tests/utils.h"
        "../../test/ssl/wolfssl/tests/w64wrapper.c"
        "../../test/ssl/wolfssl/tests/api/api.h"
        "../../test/ssl/wolfssl/tests/api/api_decl.h"
        "../../test/ssl/wolfssl/tests/api/test_aes.c"
        "../../test/ssl/wolfssl/tests/api/test_aes.h"
        "../../test/ssl/wolfssl/tests/api/test_arc4.c"
        "../../test/ssl/wolfssl/tests/api/test_arc4.h"
        "../../test/ssl/wolfssl/tests/api/test_ascon.c"
        "../../test/ssl/wolfssl/tests/api/test_ascon.h"
        "../../test/ssl/wolfssl/tests/api/test_ascon_kats.h"
        "../../test/ssl/wolfssl/tests/api/test_asn.c"
        "../../test/ssl/wolfssl/tests/api/test_asn.h"
        "../../test/ssl/wolfssl/tests/api/test_blake2.c"
        "../../test/ssl/wolfssl/tests/api/test_blake2.h"
        "../../test/ssl/wolfssl/tests/api/test_camellia.c"
        "../../test/ssl/wolfssl/tests/api/test_camellia.h"
        "../../test/ssl/wolfssl/tests/api/test_chacha.c"
        "../../test/ssl/wolfssl/tests/api/test_chacha.h"
        "../../test/ssl/wolfssl/tests/api/test_chacha20_poly1305.c"
        "../../test/ssl/wolfssl/tests/api/test_chacha20_poly1305.h"
        "../../test/ssl/wolfssl/tests/api/test_cmac.c"
        "../../test/ssl/wolfssl/tests/api/test_cmac.h"
        "../../test/ssl/wolfssl/tests/api/test_curve25519.c"
        "../../test/ssl/wolfssl/tests/api/test_curve25519.h"
        "../../test/ssl/wolfssl/tests/api/test_curve448.c"
        "../../test/ssl/wolfssl/tests/api/test_curve448.h"
        "../../test/ssl/wolfssl/tests/api/test_des3.c"
        "../../test/ssl/wolfssl/tests/api/test_des3.h"
        "../../test/ssl/wolfssl/tests/api/test_dh.c"
        "../../test/ssl/wolfssl/tests/api/test_dh.h"
        "../../test/ssl/wolfssl/tests/api/test_digest.h"
        "../../test/ssl/wolfssl/tests/api/test_dsa.c"
        "../../test/ssl/wolfssl/tests/api/test_dsa.h"
        "../../test/ssl/wolfssl/tests/api/test_dtls.c"
        "../../test/ssl/wolfssl/tests/api/test_dtls.h"
        "../../test/ssl/wolfssl/tests/api/test_ecc.c"
        "../../test/ssl/wolfssl/tests/api/test_ecc.h"
        "../../test/ssl/wolfssl/tests/api/test_ed25519.c"
        "../../test/ssl/wolfssl/tests/api/test_ed25519.h"
        "../../test/ssl/wolfssl/tests/api/test_ed448.c"
        "../../test/ssl/wolfssl/tests/api/test_ed448.h"
        "../../test/ssl/wolfssl/tests/api/test_evp.c"
        "../../test/ssl/wolfssl/tests/api/test_evp.h"
        "../../test/ssl/wolfssl/tests/api/test_hash.c"
        "../../test/ssl/wolfssl/tests/api/test_hash.h"
        "../../test/ssl/wolfssl/tests/api/test_hmac.c"
        "../../test/ssl/wolfssl/tests/api/test_hmac.h"
        "../../test/ssl/wolfssl/tests/api/test_md2.c"
        "../../test/ssl/wolfssl/tests/api/test_md2.h"
        "../../test/ssl/wolfssl/tests/api/test_md4.c"
        "../../test/ssl/wolfssl/tests/api/test_md4.h"
        "../../test/ssl/wolfssl/tests/api/test_md5.c"
        "../../test/ssl/wolfssl/tests/api/test_md5.h"
        "../../test/ssl/wolfssl/tests/api/test_mldsa.c"
        "../../test/ssl/wolfssl/tests/api/test_mldsa.h"
        "../../test/ssl/wolfssl/tests/api/test_mlkem.c"
        "../../test/ssl/wolfssl/tests/api/test_mlkem.h"
        "../../test/ssl/wolfssl/tests/api/test_ocsp.c"
        "../../test/ssl/wolfssl/tests/api/test_ocsp.h"
        "../../test/ssl/wolfssl/tests/api/test_ocsp_test_blobs.h"
        "../../test/ssl/wolfssl/tests/api/test_ossl_asn1.c"
        "../../test/ssl/wolfssl/tests/api/test_ossl_asn1.h"
        "../../test/ssl/wolfssl/tests/api/test_ossl_bio.c"
        "../../test/ssl/wolfssl/tests/api/test_ossl_bio.h"
        "../../test/ssl/wolfssl/tests/api/test_ossl_bn.c"
        "../../test/ssl/wolfssl/tests/api/test_ossl_bn.h"
        "../../test/ssl/wolfssl/tests/api/test_ossl_cipher.c"
        "../../test/ssl/wolfssl/tests/api/test_ossl_cipher.h"
        "../../test/ssl/wolfssl/tests/api/test_ossl_dgst.c"
        "../../test/ssl/wolfssl/tests/api/test_ossl_dgst.h"
        "../../test/ssl/wolfssl/tests/api/test_ossl_dh.c"
        "../../test/ssl/wolfssl/tests/api/test_ossl_dh.h"
        "../../test/ssl/wolfssl/tests/api/test_ossl_dsa.c"
        "../../test/ssl/wolfssl/tests/api/test_ossl_dsa.h"
        "../../test/ssl/wolfssl/tests/api/test_ossl_ec.c"
        "../../test/ssl/wolfssl/tests/api/test_ossl_ec.h"
        "../../test/ssl/wolfssl/tests/api/test_ossl_ecx.c"
        "../../test/ssl/wolfssl/tests/api/test_ossl_ecx.h"
        "../../test/ssl/wolfssl/tests/api/test_ossl_mac.c"
        "../../test/ssl/wolfssl/tests/api/test_ossl_mac.h"
        "../../test/ssl/wolfssl/tests/api/test_ossl_rsa.c"
        "../../test/ssl/wolfssl/tests/api/test_ossl_rsa.h"
        "../../test/ssl/wolfssl/tests/api/test_ossl_sk.c"
        "../../test/ssl/wolfssl/tests/api/test_ossl_sk.h"
        "../../test/ssl/wolfssl/tests/api/test_pkcs12.c"
        "../../test/ssl/wolfssl/tests/api/test_pkcs12.h"
        "../../test/ssl/wolfssl/tests/api/test_pkcs7.c"
        "../../test/ssl/wolfssl/tests/api/test_pkcs7.h"
        "../../test/ssl/wolfssl/tests/api/test_poly1305.c"
        "../../test/ssl/wolfssl/tests/api/test_poly1305.h"
        "../../test/ssl/wolfssl/tests/api/test_random.c"
        "../../test/ssl/wolfssl/tests/api/test_random.h"
        "../../test/ssl/wolfssl/tests/api/test_rc2.c"
        "../../test/ssl/wolfssl/tests/api/test_rc2.h"
        "../../test/ssl/wolfssl/tests/api/test_ripemd.c"
        "../../test/ssl/wolfssl/tests/api/test_ripemd.h"
        "../../test/ssl/wolfssl/tests/api/test_rsa.c"
        "../../test/ssl/wolfssl/tests/api/test_rsa.h"
        "../../test/ssl/wolfssl/tests/api/test_sha.c"
        "../../test/ssl/wolfssl/tests/api/test_sha.h"
        "../../test/ssl/wolfssl/tests/api/test_sha256.c"
        "../../test/ssl/wolfssl/tests/api/test_sha256.h"
        "../../test/ssl/wolfssl/tests/api/test_sha3.c"
        "../../test/ssl/wolfssl/tests/api/test_sha3.h"
        "../../test/ssl/wolfssl/tests/api/test_sha512.c"
        "../../test/ssl/wolfssl/tests/api/test_sha512.h"
        "../../test/ssl/wolfssl/tests/api/test_signature.c"
        "../../test/ssl/wolfssl/tests/api/test_signature.h"
        "../../test/ssl/wolfssl/tests/api/test_sm2.c"
        "../../test/ssl/wolfssl/tests/api/test_sm2.h"
        "../../test/ssl/wolfssl/tests/api/test_sm3.c"
        "../../test/ssl/wolfssl/tests/api/test_sm3.h"
        "../../test/ssl/wolfssl/tests/api/test_sm4.c"
        "../../test/ssl/wolfssl/tests/api/test_sm4.h"
        "../../test/ssl/wolfssl/tests/api/test_tls.c"
        "../../test/ssl/wolfssl/tests/api/test_tls.h"
        "../../test/ssl/wolfssl/tests/api/test_tls13.c"
        "../../test/ssl/wolfssl/tests/api/test_tls13.h"
        "../../test/ssl/wolfssl/tests/api/test_tls_ext.c"
        "../../test/ssl/wolfssl/tests/api/test_tls_ext.h"
        "../../test/ssl/wolfssl/tests/api/test_wc_encrypt.c"
        "../../test/ssl/wolfssl/tests/api/test_wc_encrypt.h"
        "../../test/ssl/wolfssl/tests/api/test_wolfmath.c"
        "../../test/ssl/wolfssl/tests/api/test_wolfmath.h"
        "../../test/ssl/wolfssl/tests/api/test_x509.c"
        "../../test/ssl/wolfssl/tests/api/test_x509.h"
        "../../test/ssl/wolfssl/testsuite/utils.h"
        "../../test/ssl/wolfssl/testsuite/utils1.c"
        "../../test/ssl/wolfssl/wolfcrypt/benchmark/README.md"
        "../../test/ssl/wolfssl/wolfcrypt/benchmark/benchmark.c"
        "../../test/ssl/wolfssl/wolfcrypt/benchmark/benchmark.h"
        "../../test/ssl/wolfssl/wolfcrypt/test/test.c"
        "../../test/ssl/wolfssl/wolfcrypt/test/test.h" )

    add_test( NAME libbitcoin-network-test COMMAND libbitcoin-network-test
            --run_test=*
            --log_level=warning
            --show_progress=no
            --detect_memory_leak=0
            --report_level=no
            --build_info=yes )

#     libbitcoin-network-test project specific include directories.
#------------------------------------------------------------------------------
    target_include_directories( libbitcoin-network-test PRIVATE
        "../../include"
        "../../include/bitcoin/ssl"
        "../../test/ssl/wolfssl" )

#     libbitcoin-network-test project specific libraries/linker flags.
#------------------------------------------------------------------------------
    target_link_libraries( libbitcoin-network-test
        ${CANONICAL_LIB_NAME}
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} )

endif()

# Manage pkgconfig installation.
#------------------------------------------------------------------------------
configure_file(
  "../../libbitcoin-network.pc.in"
  "libbitcoin-network.pc" @ONLY )

install( FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libbitcoin-network.pc"
    DESTINATION "${pkgconfigdir}" )

# Manage installation of docs.
#------------------------------------------------------------------------------
install( FILES
    "../../AUTHORS"
    "../../COPYING"
    "../../ChangeLog"
    "../../INSTALL"
    "../../NEWS"
    "../../README"
    DESTINATION share/doc/libbitcoin-network )

# Manage ${CANONICAL_LIB_NAME} installation.
#------------------------------------------------------------------------------
install( TARGETS ${CANONICAL_LIB_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include )

# Manage include installation.
#------------------------------------------------------------------------------
install( DIRECTORY "../../include/bitcoin"
    DESTINATION include )

