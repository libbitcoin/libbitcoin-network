###############################################################################
#  Copyright (c) 2014-2020 libbitcoin-network developers (see COPYING).
#
#         GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
#
###############################################################################

# libbitcoin-network project configuration.
#------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(libbitcoin-network LANGUAGES C CXX)

enable_testing()

list( APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules" )
include(CheckIncludeFiles)
include(CheckSymbolExists)

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

set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

# Add compiler options
#------------------------------------------------------------------------------
# Warn on all stuff.
add_compile_options( "-Wall" )

# Warn on extra stuff.
add_compile_options( "-Wextra" )

# Be really annoying.
add_compile_options( "-Wpedantic" )

# Disallow warning on style order of declarations.
add_compile_options( "-Wno-reorder" )

# Suppress warning for incomplete field initialization.
add_compile_options( "-Wno-missing-field-initializers" )

# Conform to style.
add_compile_options( "-Wno-missing-braces" )

# Ignore comments within comments or commenting of backslash extended lines.
add_compile_options( "-Wno-comment" )

# Conflict in stdlib under clang.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options( "-Wno-mismatched-tags" )
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

# Find boost
#------------------------------------------------------------------------------
find_package( Boost 1.72.0 REQUIRED COMPONENTS
    unit_test_framework )

set( boost_unit_test_framework_LIBS "-lboost_unit_test_framework" )

if (enable-ndebug)
    set( Boost_LIBRARY_DIR "${Boost_LIBRARY_DIR_DEBUG}" )
else ()
    set( Boost_LIBRARY_DIR "${Boost_LIBRARY_DIR_RELEASE}" )
endif()

set( boost_CPPFLAGS "-I${Boost_INCLUDE_DIR}" )
set( boost_LDFLAGS "-L${Boost_LIBRARY_DIR}" )

# Find bitcoin-system
#------------------------------------------------------------------------------
find_package( Bitcoin-System 4.0.0 REQUIRED )

# Define project common includes directories
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    include_directories( SYSTEM
        ${bitcoin_system_INCLUDE_DIRS} )
else()
    include_directories( SYSTEM
        ${bitcoin_system_STATIC_INCLUDE_DIRS} )
endif()

# Define project common library directories
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    link_directories(
        ${bitcoin_system_LIBRARY_DIRS} )
else()
    link_directories(
        ${bitcoin_system_STATIC_LIBRARY_DIRS} )
endif()

# Define project common libraries/linker flags.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    link_libraries(
        "-fstack-protector"
        "-fstack-protector-all"
        ${bitcoin_system_LIBRARIES} )
else()
    link_libraries(
        "-fstack-protector"
        "-fstack-protector-all"
        ${bitcoin_system_STATIC_LIBRARIES} )
endif()

# Define ${CANONICAL_LIB_NAME} project.
#------------------------------------------------------------------------------
add_library( ${CANONICAL_LIB_NAME}
    "../../src/acceptor.cpp"
    "../../src/channel.cpp"
    "../../src/connector.cpp"
    "../../src/hosts.cpp"
    "../../src/message_subscriber.cpp"
    "../../src/p2p.cpp"
    "../../src/proxy.cpp"
    "../../src/settings.cpp"
    "../../src/concurrent/conditional_lock.cpp"
    "../../src/concurrent/deadline.cpp"
    "../../src/concurrent/dispatcher.cpp"
    "../../src/concurrent/file_lock.cpp"
    "../../src/concurrent/flush_lock.cpp"
    "../../src/concurrent/interprocess_lock.cpp"
    "../../src/concurrent/monitor.cpp"
    "../../src/concurrent/prioritized_mutex.cpp"
    "../../src/concurrent/scope_lock.cpp"
    "../../src/concurrent/sequencer.cpp"
    "../../src/concurrent/sequential_lock.cpp"
    "../../src/concurrent/socket.cpp"
    "../../src/concurrent/thread.cpp"
    "../../src/concurrent/threadpool.cpp"
    "../../src/concurrent/work.cpp"
    "../../src/log/file_collector.cpp"
    "../../src/log/file_collector_repository.cpp"
    "../../src/log/file_counter_formatter.cpp"
    "../../src/log/sink.cpp"
    "../../src/log/statsd_sink.cpp"
    "../../src/log/udp_client_sink.cpp"
    "../../src/protocols/protocol.cpp"
    "../../src/protocols/protocol_address_31402.cpp"
    "../../src/protocols/protocol_events.cpp"
    "../../src/protocols/protocol_ping_31402.cpp"
    "../../src/protocols/protocol_ping_60001.cpp"
    "../../src/protocols/protocol_reject_70002.cpp"
    "../../src/protocols/protocol_seed_31402.cpp"
    "../../src/protocols/protocol_timer.cpp"
    "../../src/protocols/protocol_version_31402.cpp"
    "../../src/protocols/protocol_version_70002.cpp"
    "../../src/sessions/session.cpp"
    "../../src/sessions/session_batch.cpp"
    "../../src/sessions/session_inbound.cpp"
    "../../src/sessions/session_manual.cpp"
    "../../src/sessions/session_outbound.cpp"
    "../../src/sessions/session_seed.cpp" )

# ${CANONICAL_LIB_NAME} project specific include directories.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    target_include_directories( ${CANONICAL_LIB_NAME} PRIVATE
        "../../include"
        ${bitcoin_system_INCLUDE_DIRS} )
else()
    target_include_directories( ${CANONICAL_LIB_NAME} PRIVATE
        "../../include"
        ${bitcoin_system_STATIC_INCLUDE_DIRS} )
endif()

target_include_directories( ${CANONICAL_LIB_NAME} PUBLIC
"../../include" )

# ${CANONICAL_LIB_NAME} project specific libraries/linker flags.
#------------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
    target_link_libraries( ${CANONICAL_LIB_NAME}
        ${bitcoin_system_LIBRARIES} )
else()
    target_link_libraries( ${CANONICAL_LIB_NAME}
        ${bitcoin_system_STATIC_LIBRARIES} )
endif()

# Define libbitcoin-network-test project.
#------------------------------------------------------------------------------
if (with-tests)
    add_executable( libbitcoin-network-test
        "../../test/main.cpp"
        "../../test/p2p.cpp"
        "../../test/test.cpp"
        "../../test/test.hpp"
        "../../test/concurrent/file_lock.cpp"
        "../../test/concurrent/flush_lock.cpp"
        "../../test/concurrent/interprocess_lock.cpp"
        "../../test/concurrent/thread.cpp" )

    add_test( NAME libbitcoin-network-test COMMAND libbitcoin-network-test
            --run_test=empty_tests
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

