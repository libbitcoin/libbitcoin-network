/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "test.hpp"

BOOST_AUTO_TEST_SUITE(error_tests)

// error_t
// These test std::error_code equality operator overrides.

BOOST_AUTO_TEST_CASE(error_t__code__success__false_exected_message)
{
    constexpr auto value = error::success;
    const auto ec = code(value);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "success");
}

BOOST_AUTO_TEST_CASE(error_t__code__unknown__true_exected_message)
{
    constexpr auto value = error::unknown;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "unknown error");
}

BOOST_AUTO_TEST_CASE(error_t__code__bypassed__true_exected_message)
{
    constexpr auto value = error::bypassed;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "start bypassed without failure");
}

// addresses

BOOST_AUTO_TEST_CASE(error_t__code__address_not_found__true_exected_message)
{
    constexpr auto value = error::address_not_found;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "address not found");
}

BOOST_AUTO_TEST_CASE(error_t__code__seeding_unsuccessful__true_exected_message)
{
    constexpr auto value = error::seeding_unsuccessful;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "seeding unsuccessful");
}

// file system

BOOST_AUTO_TEST_CASE(error_t__code__file_load__true_exected_message)
{
    constexpr auto value = error::file_load;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "failed to load file");
}

BOOST_AUTO_TEST_CASE(error_t__code__file_save__true_exected_message)
{
    constexpr auto value = error::file_save;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "failed to save file");
}

BOOST_AUTO_TEST_CASE(error_t__code__file_system__true_exected_message)
{
    constexpr auto value = error::file_system;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "file system error");
}

// general I/O failures

BOOST_AUTO_TEST_CASE(error_t__code__bad_stream__true_exected_message)
{
    constexpr auto value = error::bad_stream;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad data stream");
}

BOOST_AUTO_TEST_CASE(error_t__code__insufficient_peer__true_exected_message)
{
    constexpr auto value = error::insufficient_peer;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "insufficient peer configuration");
}

BOOST_AUTO_TEST_CASE(error_t__code__protocol_violation__true_exected_message)
{
    constexpr auto value = error::protocol_violation;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "protocol violation");
}

// incoming connection failures

BOOST_AUTO_TEST_CASE(error_t__code__listen_failed__true_exected_message)
{
    constexpr auto value = error::listen_failed;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "incoming connection failed");
}

BOOST_AUTO_TEST_CASE(error_t__code__accept_failed__true_exected_message)
{
    constexpr auto value = error::accept_failed;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "connection acceptance failed");
}

BOOST_AUTO_TEST_CASE(error_t__code__oversubscribed__true_exected_message)
{
    constexpr auto value = error::oversubscribed;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "service oversubscribed");
}

// outgoing connection failures

BOOST_AUTO_TEST_CASE(error_t__code__address_blocked__true_exected_message)
{
    constexpr auto value = error::address_blocked;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "address blocked by policy");
}

BOOST_AUTO_TEST_CASE(error_t__code__address_in_use__true_exected_message)
{
    constexpr auto value = error::address_in_use;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "address already in use");
}

BOOST_AUTO_TEST_CASE(error_t__code__resolve_failed__true_exected_message)
{
    constexpr auto value = error::resolve_failed;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "resolving hostname failed");
}

BOOST_AUTO_TEST_CASE(error_t__code__connect_failed__true_exected_message)
{
    constexpr auto value = error::connect_failed;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "unable to reach remote host");
}

// heading read failures

BOOST_AUTO_TEST_CASE(error_t__code__invalid_heading__true_exected_message)
{
    constexpr auto value = error::invalid_heading;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "invalid message heading");
}

BOOST_AUTO_TEST_CASE(error_t__code__invalid_magic__true_exected_message)
{
    constexpr auto value = error::invalid_magic;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "invalid message heading magic");
}

// payload read failures

BOOST_AUTO_TEST_CASE(error_t__code__oversized_payload__true_exected_message)
{
    constexpr auto value = error::oversized_payload;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "oversize message payload");
}

BOOST_AUTO_TEST_CASE(error_t__code__invalid_checksum__true_exected_message)
{
    constexpr auto value = error::invalid_checksum;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "invalid message checksum");
}

BOOST_AUTO_TEST_CASE(error_t__code__invalid_message__true_exected_message)
{
    constexpr auto value = error::invalid_message;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "message failed to deserialize");
}

BOOST_AUTO_TEST_CASE(error_t__code__unknown_message__true_exected_message)
{
    constexpr auto value = error::unknown_message;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "unknown message type");
}

// general failures

BOOST_AUTO_TEST_CASE(error_t__code__invalid_configuration__true_exected_message)
{
    constexpr auto value = error::invalid_configuration;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "invalid configuration");
}

BOOST_AUTO_TEST_CASE(error_t__code__operation_timeout__true_exected_message)
{
    constexpr auto value = error::operation_timeout;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "operation timed out");
}

BOOST_AUTO_TEST_CASE(error_t__code__operation_canceled__true_exected_message)
{
    constexpr auto value = error::operation_canceled;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "operation canceled");
}

BOOST_AUTO_TEST_CASE(error_t__code__operation_failed__true_exected_message)
{
    constexpr auto value = error::operation_failed;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "operation failed");
}

// termination

BOOST_AUTO_TEST_CASE(error_t__code__channel_timeout__true_exected_message)
{
    constexpr auto value = error::channel_timeout;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "connection timed out");
}

BOOST_AUTO_TEST_CASE(error_t__code__channel_conflict__true_exected_message)
{
    constexpr auto value = error::channel_conflict;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "channel conflict");
}

BOOST_AUTO_TEST_CASE(error_t__code__channel_dropped__true_exected_message)
{
    constexpr auto value = error::channel_dropped;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "channel dropped");
}

BOOST_AUTO_TEST_CASE(error_t__code__channel_stopped__true_exected_message)
{
    constexpr auto value = error::channel_stopped;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "channel stopped");
}

BOOST_AUTO_TEST_CASE(error_t__code__service_stopped__true_exected_message)
{
    constexpr auto value = error::service_stopped;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "service stopped");
}

BOOST_AUTO_TEST_CASE(error_t__code__subscriber_stopped__true_exected_message)
{
    constexpr auto value = error::subscriber_stopped;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "subscriber stopped");
}

BOOST_AUTO_TEST_SUITE_END()
