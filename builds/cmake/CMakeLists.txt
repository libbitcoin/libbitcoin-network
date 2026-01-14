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
    "../../src/wolfssl/src/bio.c"
    "../../src/wolfssl/src/conf.c"
    "../../src/wolfssl/src/crl.c"
    "../../src/wolfssl/src/dtls.c"
    "../../src/wolfssl/src/dtls13.c"
    "../../src/wolfssl/src/internal.c"
    "../../src/wolfssl/src/keys.c"
    "../../src/wolfssl/src/ocsp.c"
    "../../src/wolfssl/src/pk.c"
    "../../src/wolfssl/src/quic.c"
    "../../src/wolfssl/src/sniffer.c"
    "../../src/wolfssl/src/ssl.c"
    "../../src/wolfssl/src/ssl_asn1.c"
    "../../src/wolfssl/src/ssl_bn.c"
    "../../src/wolfssl/src/ssl_certman.c"
    "../../src/wolfssl/src/ssl_crypto.c"
    "../../src/wolfssl/src/ssl_load.c"
    "../../src/wolfssl/src/ssl_misc.c"
    "../../src/wolfssl/src/ssl_p7p12.c"
    "../../src/wolfssl/src/ssl_sess.c"
    "../../src/wolfssl/src/ssl_sk.c"
    "../../src/wolfssl/src/tls.c"
    "../../src/wolfssl/src/tls13.c"
    "../../src/wolfssl/src/wolfio.c"
    "../../src/wolfssl/src/x509.c"
    "../../src/wolfssl/src/x509_str.c"
    "../../src/wolfssl/wolfcrypt/src/aes.c"
    "../../src/wolfssl/wolfcrypt/src/arc4.c"
    "../../src/wolfssl/wolfcrypt/src/ascon.c"
    "../../src/wolfssl/wolfcrypt/src/asm.c"
    "../../src/wolfssl/wolfcrypt/src/asn.c"
    "../../src/wolfssl/wolfcrypt/src/blake2b.c"
    "../../src/wolfssl/wolfcrypt/src/blake2s.c"
    "../../src/wolfssl/wolfcrypt/src/camellia.c"
    "../../src/wolfssl/wolfcrypt/src/chacha.c"
    "../../src/wolfssl/wolfcrypt/src/chacha20_poly1305.c"
    "../../src/wolfssl/wolfcrypt/src/cmac.c"
    "../../src/wolfssl/wolfcrypt/src/coding.c"
    "../../src/wolfssl/wolfcrypt/src/compress.c"
    "../../src/wolfssl/wolfcrypt/src/cpuid.c"
    "../../src/wolfssl/wolfcrypt/src/cryptocb.c"
    "../../src/wolfssl/wolfcrypt/src/curve25519.c"
    "../../src/wolfssl/wolfcrypt/src/curve448.c"
    "../../src/wolfssl/wolfcrypt/src/des3.c"
    "../../src/wolfssl/wolfcrypt/src/dh.c"
    "../../src/wolfssl/wolfcrypt/src/dilithium.c"
    "../../src/wolfssl/wolfcrypt/src/dsa.c"
    "../../src/wolfssl/wolfcrypt/src/ecc.c"
    "../../src/wolfssl/wolfcrypt/src/eccsi.c"
    "../../src/wolfssl/wolfcrypt/src/ed25519.c"
    "../../src/wolfssl/wolfcrypt/src/ed448.c"
    "../../src/wolfssl/wolfcrypt/src/error1.c"
    "../../src/wolfssl/wolfcrypt/src/evp.c"
    "../../src/wolfssl/wolfcrypt/src/ext_lms.c"
    "../../src/wolfssl/wolfcrypt/src/ext_mlkem.c"
    "../../src/wolfssl/wolfcrypt/src/ext_xmss.c"
    "../../src/wolfssl/wolfcrypt/src/falcon.c"
    "../../src/wolfssl/wolfcrypt/src/fe_448.c"
    "../../src/wolfssl/wolfcrypt/src/fe_low_mem.c"
    "../../src/wolfssl/wolfcrypt/src/fe_operations.c"
    "../../src/wolfssl/wolfcrypt/src/fe_x25519_128.h"
    "../../src/wolfssl/wolfcrypt/src/ge_448.c"
    "../../src/wolfssl/wolfcrypt/src/ge_low_mem.c"
    "../../src/wolfssl/wolfcrypt/src/ge_operations.c"
    "../../src/wolfssl/wolfcrypt/src/hash.c"
    "../../src/wolfssl/wolfcrypt/src/hmac.c"
    "../../src/wolfssl/wolfcrypt/src/hpke.c"
    "../../src/wolfssl/wolfcrypt/src/integer.c"
    "../../src/wolfssl/wolfcrypt/src/kdf.c"
    "../../src/wolfssl/wolfcrypt/src/logging.c"
    "../../src/wolfssl/wolfcrypt/src/md2.c"
    "../../src/wolfssl/wolfcrypt/src/md4.c"
    "../../src/wolfssl/wolfcrypt/src/md5.c"
    "../../src/wolfssl/wolfcrypt/src/memory1.c"
    "../../src/wolfssl/wolfcrypt/src/misc.c"
    "../../src/wolfssl/wolfcrypt/src/pkcs12.c"
    "../../src/wolfssl/wolfcrypt/src/pkcs7.c"
    "../../src/wolfssl/wolfcrypt/src/poly1305.c"
    "../../src/wolfssl/wolfcrypt/src/pwdbased.c"
    "../../src/wolfssl/wolfcrypt/src/random.c"
    "../../src/wolfssl/wolfcrypt/src/rc2.c"
    "../../src/wolfssl/wolfcrypt/src/ripemd.c"
    "../../src/wolfssl/wolfcrypt/src/rsa.c"
    "../../src/wolfssl/wolfcrypt/src/sakke.c"
    "../../src/wolfssl/wolfcrypt/src/sha.c"
    "../../src/wolfssl/wolfcrypt/src/sha256.c"
    "../../src/wolfssl/wolfcrypt/src/sha3.c"
    "../../src/wolfssl/wolfcrypt/src/sha512.c"
    "../../src/wolfssl/wolfcrypt/src/signature.c"
    "../../src/wolfssl/wolfcrypt/src/siphash.c"
    "../../src/wolfssl/wolfcrypt/src/sp_arm32.c"
    "../../src/wolfssl/wolfcrypt/src/sp_arm64.c"
    "../../src/wolfssl/wolfcrypt/src/sp_armthumb.c"
    "../../src/wolfssl/wolfcrypt/src/sp_c32.c"
    "../../src/wolfssl/wolfcrypt/src/sp_c64.c"
    "../../src/wolfssl/wolfcrypt/src/sp_cortexm.c"
    "../../src/wolfssl/wolfcrypt/src/sp_dsp32.c"
    "../../src/wolfssl/wolfcrypt/src/sp_int.c"
    "../../src/wolfssl/wolfcrypt/src/sp_x86_64.c"
    "../../src/wolfssl/wolfcrypt/src/sphincs.c"
    "../../src/wolfssl/wolfcrypt/src/srp.c"
    "../../src/wolfssl/wolfcrypt/src/tfm.c"
    "../../src/wolfssl/wolfcrypt/src/wc_dsp.c"
    "../../src/wolfssl/wolfcrypt/src/wc_encrypt.c"
    "../../src/wolfssl/wolfcrypt/src/wc_lms.c"
    "../../src/wolfssl/wolfcrypt/src/wc_lms_impl.c"
    "../../src/wolfssl/wolfcrypt/src/wc_mlkem.c"
    "../../src/wolfssl/wolfcrypt/src/wc_mlkem_poly.c"
    "../../src/wolfssl/wolfcrypt/src/wc_pkcs11.c"
    "../../src/wolfssl/wolfcrypt/src/wc_port.c"
    "../../src/wolfssl/wolfcrypt/src/wc_xmss.c"
    "../../src/wolfssl/wolfcrypt/src/wc_xmss_impl.c"
    "../../src/wolfssl/wolfcrypt/src/wolfevent.c"
    "../../src/wolfssl/wolfcrypt/src/wolfmath.c"
    "../../src/wolfssl/wolfssl/callbacks.h"
    "../../src/wolfssl/wolfssl/certs_test.h"
    "../../src/wolfssl/wolfssl/certs_test_sm.h"
    "../../src/wolfssl/wolfssl/crl.h"
    "../../src/wolfssl/wolfssl/error-ssl.h"
    "../../src/wolfssl/wolfssl/internal.h"
    "../../src/wolfssl/wolfssl/ocsp.h"
    "../../src/wolfssl/wolfssl/quic.h"
    "../../src/wolfssl/wolfssl/ssl.h"
    "../../src/wolfssl/wolfssl/test.h"
    "../../src/wolfssl/wolfssl/version.h"
    "../../src/wolfssl/wolfssl/wolfio.h"
    "../../src/wolfssl/wolfssl/openssl/aes.h"
    "../../src/wolfssl/wolfssl/openssl/asn1.h"
    "../../src/wolfssl/wolfssl/openssl/asn1t.h"
    "../../src/wolfssl/wolfssl/openssl/bio.h"
    "../../src/wolfssl/wolfssl/openssl/bn.h"
    "../../src/wolfssl/wolfssl/openssl/buffer.h"
    "../../src/wolfssl/wolfssl/openssl/camellia.h"
    "../../src/wolfssl/wolfssl/openssl/cmac.h"
    "../../src/wolfssl/wolfssl/openssl/cms.h"
    "../../src/wolfssl/wolfssl/openssl/compat_types.h"
    "../../src/wolfssl/wolfssl/openssl/conf.h"
    "../../src/wolfssl/wolfssl/openssl/crypto.h"
    "../../src/wolfssl/wolfssl/openssl/des.h"
    "../../src/wolfssl/wolfssl/openssl/dh.h"
    "../../src/wolfssl/wolfssl/openssl/dsa.h"
    "../../src/wolfssl/wolfssl/openssl/ec.h"
    "../../src/wolfssl/wolfssl/openssl/ec25519.h"
    "../../src/wolfssl/wolfssl/openssl/ec448.h"
    "../../src/wolfssl/wolfssl/openssl/ecdh.h"
    "../../src/wolfssl/wolfssl/openssl/ecdsa.h"
    "../../src/wolfssl/wolfssl/openssl/ed25519.h"
    "../../src/wolfssl/wolfssl/openssl/ed448.h"
    "../../src/wolfssl/wolfssl/openssl/engine.h"
    "../../src/wolfssl/wolfssl/openssl/err.h"
    "../../src/wolfssl/wolfssl/openssl/evp.h"
    "../../src/wolfssl/wolfssl/openssl/fips_rand.h"
    "../../src/wolfssl/wolfssl/openssl/hmac.h"
    "../../src/wolfssl/wolfssl/openssl/kdf.h"
    "../../src/wolfssl/wolfssl/openssl/lhash.h"
    "../../src/wolfssl/wolfssl/openssl/md4.h"
    "../../src/wolfssl/wolfssl/openssl/md5.h"
    "../../src/wolfssl/wolfssl/openssl/modes.h"
    "../../src/wolfssl/wolfssl/openssl/obj_mac.h"
    "../../src/wolfssl/wolfssl/openssl/objects.h"
    "../../src/wolfssl/wolfssl/openssl/ocsp.h"
    "../../src/wolfssl/wolfssl/openssl/opensslconf.h"
    "../../src/wolfssl/wolfssl/openssl/opensslv.h"
    "../../src/wolfssl/wolfssl/openssl/ossl_typ.h"
    "../../src/wolfssl/wolfssl/openssl/pem.h"
    "../../src/wolfssl/wolfssl/openssl/pkcs12.h"
    "../../src/wolfssl/wolfssl/openssl/pkcs7.h"
    "../../src/wolfssl/wolfssl/openssl/rand.h"
    "../../src/wolfssl/wolfssl/openssl/rc4.h"
    "../../src/wolfssl/wolfssl/openssl/ripemd.h"
    "../../src/wolfssl/wolfssl/openssl/rsa.h"
    "../../src/wolfssl/wolfssl/openssl/safestack.h"
    "../../src/wolfssl/wolfssl/openssl/sha.h"
    "../../src/wolfssl/wolfssl/openssl/sha3.h"
    "../../src/wolfssl/wolfssl/openssl/srp.h"
    "../../src/wolfssl/wolfssl/openssl/ssl.h"
    "../../src/wolfssl/wolfssl/openssl/ssl23.h"
    "../../src/wolfssl/wolfssl/openssl/stack.h"
    "../../src/wolfssl/wolfssl/openssl/tls1.h"
    "../../src/wolfssl/wolfssl/openssl/txt_db.h"
    "../../src/wolfssl/wolfssl/openssl/ui.h"
    "../../src/wolfssl/wolfssl/openssl/x509.h"
    "../../src/wolfssl/wolfssl/openssl/x509_vfy.h"
    "../../src/wolfssl/wolfssl/openssl/x509v3.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/aes.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/arc4.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/ascon.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/asn.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/asn_public.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/blake2-impl.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/blake2-int.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/blake2.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/camellia.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/chacha.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/chacha20_poly1305.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/cmac.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/coding.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/compress.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/cpuid.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/cryptocb.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/curve25519.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/curve448.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/des3.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/dh.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/dilithium.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/dsa.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/ecc.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/eccsi.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/ed25519.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/ed448.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/error-crypt.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/ext_lms.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/ext_mlkem.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/ext_xmss.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/falcon.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/fe_448.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/fe_operations.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/fips_test.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/ge_448.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/ge_operations.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/hash.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/hmac.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/hpke.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/integer.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/kdf.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/libwolfssl_sources.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/libwolfssl_sources_asm.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/lms.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/logging.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/md2.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/md4.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/md5.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/mem_track.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/memory.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/misc.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/mlkem.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/mpi_class.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/mpi_superclass.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/oid_sum.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/pkcs11.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/pkcs12.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/pkcs7.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/poly1305.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/pwdbased.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/random.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/rc2.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/ripemd.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/rsa.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/sakke.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/selftest.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/settings.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/sha.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/sha256.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/sha3.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/sha512.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/signature.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/siphash.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/sp.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/sp_int.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/sphincs.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/srp.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/tfm.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/types.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/user_settings.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/visibility.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/wc_encrypt.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/wc_lms.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/wc_mlkem.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/wc_pkcs11.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/wc_port.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/wc_xmss.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/wolfevent.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/wolfmath.h"
    "../../src/wolfssl/wolfssl/wolfcrypt/xmss.h" )

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
    "../../src/wolfssl"
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
        "../../test/sessions/session_server.cpp" )

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
        "../../include" )

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

