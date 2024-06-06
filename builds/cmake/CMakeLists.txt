###############################################################################
#  Copyright (c) 2014-2023 libbitcoin-network developers (see COPYING).
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
find_package( Boost 1.76.0 REQUIRED COMPONENTS
    regex
    unit_test_framework )

set( boost_regex_LIBS "-lboost_regex" )
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
    ${Boost_REGEX_LIBRARY}
    ${bitcoin_system_FOR_BUILD_LIBRARIES} )

# Define ${CANONICAL_LIB_NAME} project.
#------------------------------------------------------------------------------
add_library( ${CANONICAL_LIB_NAME}
    "../../src/error.cpp"
    "../../src/p2p.cpp"
    "../../src/settings.cpp"
    "../../src/async/thread.cpp"
    "../../src/async/threadpool.cpp"
    "../../src/async/time.cpp"
    "../../src/config/address.cpp"
    "../../src/config/authority.cpp"
    "../../src/config/endpoint.cpp"
    "../../src/config/utilities.cpp"
    "../../src/log/capture.cpp"
    "../../src/log/logger.cpp"
    "../../src/log/reporter.cpp"
    "../../src/messages/address.cpp"
    "../../src/messages/address_item.cpp"
    "../../src/messages/alert.cpp"
    "../../src/messages/alert_item.cpp"
    "../../src/messages/block.cpp"
    "../../src/messages/bloom_filter_add.cpp"
    "../../src/messages/bloom_filter_clear.cpp"
    "../../src/messages/bloom_filter_load.cpp"
    "../../src/messages/client_filter.cpp"
    "../../src/messages/client_filter_checkpoint.cpp"
    "../../src/messages/client_filter_headers.cpp"
    "../../src/messages/compact_block.cpp"
    "../../src/messages/compact_block_item.cpp"
    "../../src/messages/compact_transactions.cpp"
    "../../src/messages/fee_filter.cpp"
    "../../src/messages/get_address.cpp"
    "../../src/messages/get_blocks.cpp"
    "../../src/messages/get_client_filter_checkpoint.cpp"
    "../../src/messages/get_client_filter_headers.cpp"
    "../../src/messages/get_client_filters.cpp"
    "../../src/messages/get_compact_transactions.cpp"
    "../../src/messages/get_data.cpp"
    "../../src/messages/get_headers.cpp"
    "../../src/messages/headers.cpp"
    "../../src/messages/heading.cpp"
    "../../src/messages/inventory.cpp"
    "../../src/messages/inventory_item.cpp"
    "../../src/messages/memory_pool.cpp"
    "../../src/messages/merkle_block.cpp"
    "../../src/messages/message.cpp"
    "../../src/messages/not_found.cpp"
    "../../src/messages/ping.cpp"
    "../../src/messages/pong.cpp"
    "../../src/messages/reject.cpp"
    "../../src/messages/send_compact.cpp"
    "../../src/messages/send_headers.cpp"
    "../../src/messages/transaction.cpp"
    "../../src/messages/version.cpp"
    "../../src/messages/version_acknowledge.cpp"
    "../../src/net/acceptor.cpp"
    "../../src/net/broadcaster.cpp"
    "../../src/net/channel.cpp"
    "../../src/net/connector.cpp"
    "../../src/net/deadline.cpp"
    "../../src/net/distributor.cpp"
    "../../src/net/hosts.cpp"
    "../../src/net/proxy.cpp"
    "../../src/net/socket.cpp"
    "../../src/protocols/protocol.cpp"
    "../../src/protocols/protocol_address_in_31402.cpp"
    "../../src/protocols/protocol_address_out_31402.cpp"
    "../../src/protocols/protocol_alert_31402.cpp"
    "../../src/protocols/protocol_ping_31402.cpp"
    "../../src/protocols/protocol_ping_60001.cpp"
    "../../src/protocols/protocol_reject_70002.cpp"
    "../../src/protocols/protocol_seed_31402.cpp"
    "../../src/protocols/protocol_version_31402.cpp"
    "../../src/protocols/protocol_version_70001.cpp"
    "../../src/protocols/protocol_version_70002.cpp"
    "../../src/sessions/session.cpp"
    "../../src/sessions/session_inbound.cpp"
    "../../src/sessions/session_manual.cpp"
    "../../src/sessions/session_outbound.cpp"
    "../../src/sessions/session_seed.cpp" )

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
    ${Boost_REGEX_LIBRARY}
    ${bitcoin_system_FOR_BUILD_LIBRARIES} )

# Define libbitcoin-network-test project.
#------------------------------------------------------------------------------
if (with-tests)
    add_executable( libbitcoin-network-test
        "../../test/error.cpp"
        "../../test/main.cpp"
        "../../test/p2p.cpp"
        "../../test/settings.cpp"
        "../../test/test.cpp"
        "../../test/test.hpp"
        "../../test/async/desubscriber.cpp"
        "../../test/async/enable_shared_from_base.cpp"
        "../../test/async/subscriber.cpp"
        "../../test/async/thread.cpp"
        "../../test/async/threadpool.cpp"
        "../../test/async/unsubscriber.cpp"
        "../../test/async/races/race_all.cpp"
        "../../test/async/races/race_quality.cpp"
        "../../test/async/races/race_speed.cpp"
        "../../test/async/races/race_unity.cpp"
        "../../test/async/races/race_volume.cpp"
        "../../test/config/address.cpp"
        "../../test/config/authority.cpp"
        "../../test/config/endpoint.cpp"
        "../../test/config/utilities.cpp"
        "../../test/log/timer.cpp"
        "../../test/log/tracker.cpp"
        "../../test/messages/address.cpp"
        "../../test/messages/address_item.cpp"
        "../../test/messages/alert.cpp"
        "../../test/messages/alert_item.cpp"
        "../../test/messages/block.cpp"
        "../../test/messages/bloom_filter_add.cpp"
        "../../test/messages/bloom_filter_clear.cpp"
        "../../test/messages/bloom_filter_load.cpp"
        "../../test/messages/client_filter.cpp"
        "../../test/messages/client_filter_checkpoint.cpp"
        "../../test/messages/client_filter_headers.cpp"
        "../../test/messages/compact_block.cpp"
        "../../test/messages/compact_block_item.cpp"
        "../../test/messages/compact_transactions.cpp"
        "../../test/messages/fee_filter.cpp"
        "../../test/messages/get_address.cpp"
        "../../test/messages/get_blocks.cpp"
        "../../test/messages/get_client_filter_checkpoint.cpp"
        "../../test/messages/get_client_filter_headers.cpp"
        "../../test/messages/get_client_filters.cpp"
        "../../test/messages/get_compact_transactions.cpp"
        "../../test/messages/get_data.cpp"
        "../../test/messages/get_headers.cpp"
        "../../test/messages/headers.cpp"
        "../../test/messages/heading.cpp"
        "../../test/messages/inventory.cpp"
        "../../test/messages/inventory_item.cpp"
        "../../test/messages/memory_pool.cpp"
        "../../test/messages/merkle_block.cpp"
        "../../test/messages/message.cpp"
        "../../test/messages/not_found.cpp"
        "../../test/messages/ping.cpp"
        "../../test/messages/pong.cpp"
        "../../test/messages/reject.cpp"
        "../../test/messages/send_compact.cpp"
        "../../test/messages/send_headers.cpp"
        "../../test/messages/transaction.cpp"
        "../../test/messages/version.cpp"
        "../../test/messages/version_acknowledge.cpp"
        "../../test/net/acceptor.cpp"
        "../../test/net/broadcaster.cpp"
        "../../test/net/channel.cpp"
        "../../test/net/connector.cpp"
        "../../test/net/deadline.cpp"
        "../../test/net/distributor.cpp"
        "../../test/net/hosts.cpp"
        "../../test/net/proxy.cpp"
        "../../test/net/socket.cpp"
        "../../test/protocols/protocol.cpp"
        "../../test/protocols/protocol_address_in_31402.cpp"
        "../../test/protocols/protocol_address_out_31402.cpp"
        "../../test/protocols/protocol_alert_31402.cpp"
        "../../test/protocols/protocol_ping_31402.cpp"
        "../../test/protocols/protocol_ping_60001.cpp"
        "../../test/protocols/protocol_reject_70002.cpp"
        "../../test/protocols/protocol_seed_31402.cpp"
        "../../test/protocols/protocol_version_31402.cpp"
        "../../test/protocols/protocol_version_70001.cpp"
        "../../test/protocols/protocol_version_70002.cpp"
        "../../test/sessions/session.cpp"
        "../../test/sessions/session_inbound.cpp"
        "../../test/sessions/session_manual.cpp"
        "../../test/sessions/session_outbound.cpp"
        "../../test/sessions/session_seed.cpp" )

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

