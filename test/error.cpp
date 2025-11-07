/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_CASE(error_t__code__upgraded__true_exected_message)
{
    constexpr auto value = error::upgraded;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "upgraded");
}

// addresses

BOOST_AUTO_TEST_CASE(error_t__code__address_invalid__true_exected_message)
{
    constexpr auto value = error::address_invalid;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "address invalid");
}

BOOST_AUTO_TEST_CASE(error_t__code__address_not_found__true_exected_message)
{
    constexpr auto value = error::address_not_found;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "address not found");
}

BOOST_AUTO_TEST_CASE(error_t__code__address_disabled__true_exected_message)
{
    constexpr auto value = error::address_disabled;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "address protocol disabled");
}

BOOST_AUTO_TEST_CASE(error_t__code__address_unsupported__true_exected_message)
{
    constexpr auto value = error::address_unsupported;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "advertised services unsupported");
}

BOOST_AUTO_TEST_CASE(error_t__code__address_insufficient__true_exected_message)
{
    constexpr auto value = error::address_insufficient;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "advertised services insufficient");
}

BOOST_AUTO_TEST_CASE(error_t__code__seeding_unsuccessful__true_exected_message)
{
    constexpr auto value = error::seeding_unsuccessful;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "seeding unsuccessful");
}

BOOST_AUTO_TEST_CASE(error_t__code__seeding_complete__true_exected_message)
{
    constexpr auto value = error::seeding_complete;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "seeding complete");
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

BOOST_AUTO_TEST_CASE(error_t__code__file_exception__true_exected_message)
{
    constexpr auto value = error::file_exception;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "file exception");
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

BOOST_AUTO_TEST_CASE(error_t__code__not_allowed__true_exected_message)
{
    constexpr auto value = error::not_allowed;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "not allowed");
}

BOOST_AUTO_TEST_CASE(error_t__code__peer_disconnect__true_exected_message)
{
    constexpr auto value = error::peer_disconnect;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "peer disconnect");
}

BOOST_AUTO_TEST_CASE(error_t__code__peer_unsupported__true_exected_message)
{
    constexpr auto value = error::peer_unsupported;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "peer unsupported");
}

BOOST_AUTO_TEST_CASE(error_t__code__peer_insufficient__true_exected_message)
{
    constexpr auto value = error::peer_insufficient;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "peer insufficient");
}

BOOST_AUTO_TEST_CASE(error_t__code__peer_timestamp__true_exected_message)
{
    constexpr auto value = error::peer_timestamp;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "peer timestamp");
}

BOOST_AUTO_TEST_CASE(error_t__code__protocol_violation__true_exected_message)
{
    constexpr auto value = error::protocol_violation;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "protocol violation");
}

BOOST_AUTO_TEST_CASE(error_t__code__channel_overflow__true_exected_message)
{
    constexpr auto value = error::channel_overflow;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "channel overflow");
}

BOOST_AUTO_TEST_CASE(error_t__code__channel_underflow__true_exected_message)
{
    constexpr auto value = error::channel_underflow;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "channel underflow");
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
    BOOST_REQUIRE_EQUAL(ec.message(), "connection to self aborted");
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
    BOOST_REQUIRE_EQUAL(ec.message(), "channel timed out");
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

BOOST_AUTO_TEST_CASE(error_t__code__channel_expired__true_exected_message)
{
    constexpr auto value = error::channel_expired;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "channel expired");
}

BOOST_AUTO_TEST_CASE(error_t__code__channel_inactive__true_exected_message)
{
    constexpr auto value = error::channel_inactive;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "channel inactive");
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

BOOST_AUTO_TEST_CASE(error_t__code__service_suspended__true_exected_message)
{
    constexpr auto value = error::service_suspended;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "service suspended");
}

BOOST_AUTO_TEST_CASE(error_t__code__subscriber_exists__true_exected_message)
{
    constexpr auto value = error::subscriber_exists;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "subscriber exists");
}

BOOST_AUTO_TEST_CASE(error_t__code__subscriber_stopped__true_exected_message)
{
    constexpr auto value = error::subscriber_stopped;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "subscriber stopped");
}

BOOST_AUTO_TEST_CASE(error_t__code__desubscribed__true_exected_message)
{
    constexpr auto value = error::desubscribed;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "subscriber desubscribed");
}

// http 4xx client error

BOOST_AUTO_TEST_CASE(error_t__code__bad_request__true_exected_message)
{
    constexpr auto value = error::bad_request;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad request");
}

////BOOST_AUTO_TEST_CASE(error_t__code__unauthorized__true_exected_message)
////{
////    constexpr auto value = error::unauthorized;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "unauthorized");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__payment_required__true_exected_message)
////{
////    constexpr auto value = error::payment_required;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "payment required");
////}

BOOST_AUTO_TEST_CASE(error_t__code__forbidden__true_exected_message)
{
    constexpr auto value = error::forbidden;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "forbidden");
}

BOOST_AUTO_TEST_CASE(error_t__code__not_found__true_exected_message)
{
    constexpr auto value = error::not_found;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "not found");
}

BOOST_AUTO_TEST_CASE(error_t__code__method_not_allowed__true_exected_message)
{
    constexpr auto value = error::method_not_allowed;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "method not allowed");
}

////BOOST_AUTO_TEST_CASE(error_t__code__not_acceptable__true_exected_message)
////{
////    constexpr auto value = error::not_acceptable;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "not acceptable");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__proxy_authentication_required__true_exected_message)
////{
////    constexpr auto value = error::proxy_authentication_required;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "proxy authentication required");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__request_timeout__true_exected_message)
////{
////    constexpr auto value = error::request_timeout;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "request timeout");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__conflict__true_exected_message)
////{
////    constexpr auto value = error::conflict;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "conflict");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__gone__true_exected_message)
////{
////    constexpr auto value = error::gone;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "gone");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__length_required__true_exected_message)
////{
////    constexpr auto value = error::length_required;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "length required");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__precondition_failed__true_exected_message)
////{
////    constexpr auto value = error::precondition_failed;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "precondition failed");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__payload_too_large__true_exected_message)
////{
////    constexpr auto value = error::payload_too_large;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "payload too large");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__uri_too_long__true_exected_message)
////{
////    constexpr auto value = error::uri_too_long;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "uri too long");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__unsupported_media_type__true_exected_message)
////{
////    constexpr auto value = error::unsupported_media_type;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "unsupported media type");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__range_not_satisfiable__true_exected_message)
////{
////    constexpr auto value = error::range_not_satisfiable;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "range not satisfiable");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__expectation_failed__true_exected_message)
////{
////    constexpr auto value = error::expectation_failed;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "expectation failed");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__im_a_teapot__true_exected_message)
////{
////    constexpr auto value = error::im_a_teapot;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "im a teapot");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__misdirected_request__true_exected_message)
////{
////    constexpr auto value = error::misdirected_request;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "misdirected request");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__unprocessable_entity__true_exected_message)
////{
////    constexpr auto value = error::unprocessable_entity;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "unprocessable entity");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__locked__true_exected_message)
////{
////    constexpr auto value = error::locked;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "locked");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__failed_dependency__true_exected_message)
////{
////    constexpr auto value = error::failed_dependency;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "failed dependency");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__too_early__true_exected_message)
////{
////    constexpr auto value = error::too_early;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "too early");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__upgrade_required__true_exected_message)
////{
////    constexpr auto value = error::upgrade_required;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "upgrade required");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__precondition_required__true_exected_message)
////{
////    constexpr auto value = error::precondition_required;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "precondition required");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__too_many_requests__true_exected_message)
////{
////    constexpr auto value = error::too_many_requests;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "too many requests");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__request_header_fields_too_large__true_exected_message)
////{
////    constexpr auto value = error::request_header_fields_too_large;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "request header fields too large");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__unavailable_for_legal_reasons__true_exected_message)
////{
////    constexpr auto value = error::unavailable_for_legal_reasons;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "unavailable for legal reasons");
////}

// http 5xx server error

////BOOST_AUTO_TEST_CASE(error_t__code__internal_server_error__true_exected_message)
////{
////    constexpr auto value = error::internal_server_error;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "internal server error");
////}

BOOST_AUTO_TEST_CASE(error_t__code__not_implemented__true_exected_message)
{
    constexpr auto value = error::not_implemented;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "not implemented");
}

////BOOST_AUTO_TEST_CASE(error_t__code__bad_gateway__true_exected_message)
////{
////    constexpr auto value = error::bad_gateway;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "bad gateway");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__service_unavailable__true_exected_message)
////{
////    constexpr auto value = error::service_unavailable;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "service unavailable");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__gateway_timeout__true_exected_message)
////{
////    constexpr auto value = error::gateway_timeout;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "gateway timeout");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__http_version_not_supported__true_exected_message)
////{
////    constexpr auto value = error::http_version_not_supported;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "http version not supported");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__variant_also_negotiates__true_exected_message)
////{
////    constexpr auto value = error::variant_also_negotiates;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "variant also negotiates");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__insufficient_storage__true_exected_message)
////{
////    constexpr auto value = error::insufficient_storage;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "insufficient storage");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__loop_detected__true_exected_message)
////{
////    constexpr auto value = error::loop_detected;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "loop detected");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__not_extended__true_exected_message)
////{
////    constexpr auto value = error::not_extended;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "not extended");
////}
////
////BOOST_AUTO_TEST_CASE(error_t__code__network_authentication_required__true_exected_message)
////{
////    constexpr auto value = error::network_authentication_required;
////    const auto ec = code(value);
////    BOOST_REQUIRE(ec);
////    BOOST_REQUIRE(ec == value);
////    BOOST_REQUIRE_EQUAL(ec.message(), "network authentication required");
////}

// boost beast error

BOOST_AUTO_TEST_CASE(error_t__code__end_of_stream__true_exected_message)
{
    constexpr auto value = error::end_of_stream;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "end of stream");
}

BOOST_AUTO_TEST_CASE(error_t__code__partial_message__true_exected_message)
{
    constexpr auto value = error::partial_message;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "partial message");
}

BOOST_AUTO_TEST_CASE(error_t__code__need_more__true_exected_message)
{
    constexpr auto value = error::need_more;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "need more");
}

BOOST_AUTO_TEST_CASE(error_t__code__unexpected_body__true_exected_message)
{
    constexpr auto value = error::unexpected_body;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "unexpected body");
}

BOOST_AUTO_TEST_CASE(error_t__code__need_buffer__true_exected_message)
{
    constexpr auto value = error::need_buffer;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "need buffer");
}

BOOST_AUTO_TEST_CASE(error_t__code__end_of_chunk__true_exected_message)
{
    constexpr auto value = error::end_of_chunk;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "end of chunk");
}

BOOST_AUTO_TEST_CASE(error_t__code__buffer_overflow__true_exected_message)
{
    constexpr auto value = error::buffer_overflow;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "buffer overflow");
}

BOOST_AUTO_TEST_CASE(error_t__code__header_limit__true_exected_message)
{
    constexpr auto value = error::header_limit;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "header limit");
}

BOOST_AUTO_TEST_CASE(error_t__code__body_limit__true_exected_message)
{
    constexpr auto value = error::body_limit;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "body limit");
}

BOOST_AUTO_TEST_CASE(error_t__code__bad_alloc__true_exected_message)
{
    constexpr auto value = error::bad_alloc;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad alloc");
}

BOOST_AUTO_TEST_CASE(error_t__code__bad_line_ending__true_exected_message)
{
    constexpr auto value = error::bad_line_ending;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad line ending");
}

BOOST_AUTO_TEST_CASE(error_t__code__bad_method__true_exected_message)
{
    constexpr auto value = error::bad_method;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad method");
}

BOOST_AUTO_TEST_CASE(error_t__code__bad_target__true_exected_message)
{
    constexpr auto value = error::bad_target;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad target");
}

BOOST_AUTO_TEST_CASE(error_t__code__bad_version__true_exected_message)
{
    constexpr auto value = error::bad_version;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad version");
}

BOOST_AUTO_TEST_CASE(error_t__code__bad_status__true_exected_message)
{
    constexpr auto value = error::bad_status;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad status");
}

BOOST_AUTO_TEST_CASE(error_t__code__bad_reason__true_exected_message)
{
    constexpr auto value = error::bad_reason;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad reason");
}

BOOST_AUTO_TEST_CASE(error_t__code__bad_field__true_exected_message)
{
    constexpr auto value = error::bad_field;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad field");
}

BOOST_AUTO_TEST_CASE(error_t__code__bad_value__true_exected_message)
{
    constexpr auto value = error::bad_value;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad value");
}

BOOST_AUTO_TEST_CASE(error_t__code__bad_content_length__true_exected_message)
{
    constexpr auto value = error::bad_content_length;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad content length");
}

BOOST_AUTO_TEST_CASE(error_t__code__bad_transfer_encoding__true_exected_message)
{
    constexpr auto value = error::bad_transfer_encoding;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad transfer encoding");
}

BOOST_AUTO_TEST_CASE(error_t__code__bad_chunk__true_exected_message)
{
    constexpr auto value = error::bad_chunk;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad chunk");
}

BOOST_AUTO_TEST_CASE(error_t__code__bad_chunk_extension__true_exected_message)
{
    constexpr auto value = error::bad_chunk_extension;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad chunk extension");
}

BOOST_AUTO_TEST_CASE(error_t__code__bad_obs_fold__true_exected_message)
{
    constexpr auto value = error::bad_obs_fold;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "bad obs fold");
}

BOOST_AUTO_TEST_CASE(error_t__code__multiple_content_length__true_exected_message)
{
    constexpr auto value = error::multiple_content_length;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "multiple content length");
}

BOOST_AUTO_TEST_CASE(error_t__code__stale_parser__true_exected_message)
{
    constexpr auto value = error::stale_parser;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "stale parser");
}

BOOST_AUTO_TEST_CASE(error_t__code__short_read__true_exected_message)
{
    constexpr auto value = error::short_read;
    const auto ec = code(value);
    BOOST_REQUIRE(ec);
    BOOST_REQUIRE(ec == value);
    BOOST_REQUIRE_EQUAL(ec.message(), "short read");
}

BOOST_AUTO_TEST_SUITE_END()
