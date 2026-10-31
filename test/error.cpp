/**
 * Copyright (c) 2011-2026 libbitcoin developers
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

BC_AUTO_TEST_SUITE(error_tests)

// error_t
// These test std::error_code equality operator overrides.

BC_AUTO_TEST_CASE(error_t__code__success__false_expected_message)
{
    constexpr auto value = error::success;
    const auto ec = code(value);
    BC_REQUIRE(!ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "success");
}

BC_AUTO_TEST_CASE(error_t__code__unknown__true_expected_message)
{
    constexpr auto value = error::unknown;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "unknown error");
}

BC_AUTO_TEST_CASE(error_t__code__upgrade__true_expected_message)
{
    constexpr auto value = error::upgrade;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "upgrade requested");
}

BC_AUTO_TEST_CASE(error_t__code__upgraded__true_expected_message)
{
    constexpr auto value = error::upgraded;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "upgraded");
}

// addresses

BC_AUTO_TEST_CASE(error_t__code__address_invalid__true_expected_message)
{
    constexpr auto value = error::address_invalid;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "address invalid");
}

BC_AUTO_TEST_CASE(error_t__code__address_not_found__true_expected_message)
{
    constexpr auto value = error::address_not_found;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "address not found");
}

BC_AUTO_TEST_CASE(error_t__code__address_disabled__true_expected_message)
{
    constexpr auto value = error::address_disabled;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "address protocol disabled");
}

BC_AUTO_TEST_CASE(error_t__code__address_unsupported__true_expected_message)
{
    constexpr auto value = error::address_unsupported;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "advertised services unsupported");
}

BC_AUTO_TEST_CASE(error_t__code__address_insufficient__true_expected_message)
{
    constexpr auto value = error::address_insufficient;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "advertised services insufficient");
}

BC_AUTO_TEST_CASE(error_t__code__seeding_unsuccessful__true_expected_message)
{
    constexpr auto value = error::seeding_unsuccessful;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "seeding unsuccessful");
}

BC_AUTO_TEST_CASE(error_t__code__seeding_complete__true_expected_message)
{
    constexpr auto value = error::seeding_complete;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "seeding complete");
}

// file system

BC_AUTO_TEST_CASE(error_t__code__file_load__true_expected_message)
{
    constexpr auto value = error::file_load;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "failed to load file");
}

BC_AUTO_TEST_CASE(error_t__code__file_save__true_expected_message)
{
    constexpr auto value = error::file_save;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "failed to save file");
}

BC_AUTO_TEST_CASE(error_t__code__file_system__true_expected_message)
{
    constexpr auto value = error::file_system;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "file system error");
}

BC_AUTO_TEST_CASE(error_t__code__file_exception__true_expected_message)
{
    constexpr auto value = error::file_exception;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "file exception");
}

// general I/O failures

BC_AUTO_TEST_CASE(error_t__code__bad_stream__true_expected_message)
{
    constexpr auto value = error::bad_stream;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad data stream");
}

BC_AUTO_TEST_CASE(error_t__code__not_allowed__true_expected_message)
{
    constexpr auto value = error::not_allowed;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not allowed");
}

BC_AUTO_TEST_CASE(error_t__code__peer_disconnect__true_expected_message)
{
    constexpr auto value = error::peer_disconnect;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "peer disconnect");
}

BC_AUTO_TEST_CASE(error_t__code__peer_unsupported__true_expected_message)
{
    constexpr auto value = error::peer_unsupported;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "peer unsupported");
}

BC_AUTO_TEST_CASE(error_t__code__peer_insufficient__true_expected_message)
{
    constexpr auto value = error::peer_insufficient;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "peer insufficient");
}

BC_AUTO_TEST_CASE(error_t__code__peer_timestamp__true_expected_message)
{
    constexpr auto value = error::peer_timestamp;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "peer timestamp");
}

BC_AUTO_TEST_CASE(error_t__code__protocol_violation__true_expected_message)
{
    constexpr auto value = error::protocol_violation;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "protocol violation");
}

BC_AUTO_TEST_CASE(error_t__code__channel_overflow__true_expected_message)
{
    constexpr auto value = error::channel_overflow;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "channel overflow");
}

BC_AUTO_TEST_CASE(error_t__code__channel_underflow__true_expected_message)
{
    constexpr auto value = error::channel_underflow;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "channel underflow");
}

BC_AUTO_TEST_CASE(error_t__code__channel_backlog__true_expected_message)
{
    constexpr auto value = error::channel_backlog;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "channel write backlog");
}

BC_AUTO_TEST_CASE(error_t__code__message_dropped__true_expected_message)
{
    constexpr auto value = error::message_dropped;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "message dropped");
}

// incoming connection failures

BC_AUTO_TEST_CASE(error_t__code__listen_failed__true_expected_message)
{
    constexpr auto value = error::listen_failed;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "incoming connection failed");
}

BC_AUTO_TEST_CASE(error_t__code__accept_failed__true_expected_message)
{
    constexpr auto value = error::accept_failed;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "connection to self aborted");
}

BC_AUTO_TEST_CASE(error_t__code__oversubscribed__true_expected_message)
{
    constexpr auto value = error::oversubscribed;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "service oversubscribed");
}

BC_AUTO_TEST_CASE(error_t__code__address_blocked__true_expected_message)
{
    constexpr auto value = error::address_blocked;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "address blocked by policy");
}

// outgoing connection failures

BC_AUTO_TEST_CASE(error_t__code__address_in_use__true_expected_message)
{
    constexpr auto value = error::address_in_use;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "address already in use");
}

BC_AUTO_TEST_CASE(error_t__code__resolve_failed__true_expected_message)
{
    constexpr auto value = error::resolve_failed;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "resolving hostname failed");
}

BC_AUTO_TEST_CASE(error_t__code__connect_failed__true_expected_message)
{
    constexpr auto value = error::connect_failed;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "unable to reach remote host");
}

BC_AUTO_TEST_CASE(error_t__code__net_unreachable__true_expected_message)
{
    constexpr auto value = error::net_unreachable;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "network unreachable");
}

BC_AUTO_TEST_CASE(error_t__code__host_unreachable__true_expected_message)
{
    constexpr auto value = error::host_unreachable;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "host unreachable");
}

BC_AUTO_TEST_CASE(error_t__code__connection_refused__true_expected_message)
{
    constexpr auto value = error::connection_refused;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "connection refused");
}

// heading read failures

BC_AUTO_TEST_CASE(error_t__code__invalid_heading__true_expected_message)
{
    constexpr auto value = error::invalid_heading;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "invalid message heading");
}

BC_AUTO_TEST_CASE(error_t__code__invalid_magic__true_expected_message)
{
    constexpr auto value = error::invalid_magic;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "invalid message heading magic");
}

// payload read failures

BC_AUTO_TEST_CASE(error_t__code__oversized_payload__true_expected_message)
{
    constexpr auto value = error::oversized_payload;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "oversize message payload");
}

BC_AUTO_TEST_CASE(error_t__code__invalid_checksum__true_expected_message)
{
    constexpr auto value = error::invalid_checksum;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "invalid message checksum");
}

BC_AUTO_TEST_CASE(error_t__code__invalid_message__true_expected_message)
{
    constexpr auto value = error::invalid_message;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "message failed to deserialize");
}

BC_AUTO_TEST_CASE(error_t__code__unknown_message__true_expected_message)
{
    constexpr auto value = error::unknown_message;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "unknown message type");
}

// general failures

BC_AUTO_TEST_CASE(error_t__code__invalid_configuration__true_expected_message)
{
    constexpr auto value = error::invalid_configuration;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "invalid configuration");
}

BC_AUTO_TEST_CASE(error_t__code__insufficient_buffer__true_expected_message)
{
    constexpr auto value = error::insufficient_buffer;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "insufficient buffer space");
}

BC_AUTO_TEST_CASE(error_t__code__operation_timeout__true_expected_message)
{
    constexpr auto value = error::operation_timeout;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "operation timed out");
}

BC_AUTO_TEST_CASE(error_t__code__operation_canceled__true_expected_message)
{
    constexpr auto value = error::operation_canceled;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "operation canceled");
}

BC_AUTO_TEST_CASE(error_t__code__operation_failed__true_expected_message)
{
    constexpr auto value = error::operation_failed;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "operation failed");
}

// termination

BC_AUTO_TEST_CASE(error_t__code__channel_timeout__true_expected_message)
{
    constexpr auto value = error::channel_timeout;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "channel timed out");
}

BC_AUTO_TEST_CASE(error_t__code__channel_conflict__true_expected_message)
{
    constexpr auto value = error::channel_conflict;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "channel conflict");
}

BC_AUTO_TEST_CASE(error_t__code__channel_dropped__true_expected_message)
{
    constexpr auto value = error::channel_dropped;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "channel dropped");
}

BC_AUTO_TEST_CASE(error_t__code__channel_expired__true_expected_message)
{
    constexpr auto value = error::channel_expired;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "channel expired");
}

BC_AUTO_TEST_CASE(error_t__code__channel_inactive__true_expected_message)
{
    constexpr auto value = error::channel_inactive;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "channel inactive");
}

BC_AUTO_TEST_CASE(error_t__code__channel_stopped__true_expected_message)
{
    constexpr auto value = error::channel_stopped;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "channel stopped");
}

BC_AUTO_TEST_CASE(error_t__code__service_stopped__true_expected_message)
{
    constexpr auto value = error::service_stopped;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "service stopped");
}

BC_AUTO_TEST_CASE(error_t__code__service_suspended__true_expected_message)
{
    constexpr auto value = error::service_suspended;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "service suspended");
}

BC_AUTO_TEST_CASE(error_t__code__subscriber_exists__true_expected_message)
{
    constexpr auto value = error::subscriber_exists;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "subscriber exists");
}

BC_AUTO_TEST_CASE(error_t__code__subscriber_stopped__true_expected_message)
{
    constexpr auto value = error::subscriber_stopped;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "subscriber stopped");
}

BC_AUTO_TEST_CASE(error_t__code__desubscribed__true_expected_message)
{
    constexpr auto value = error::desubscribed;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "subscriber desubscribed");
}

// socks5

BC_AUTO_TEST_CASE(error_t__code__socks_method__true_expected_message)
{
    constexpr auto value = error::socks_method;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "socks method not supported");
}

BC_AUTO_TEST_CASE(error_t__code__socks_username__true_expected_message)
{
    constexpr auto value = error::socks_username;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "socks username too long");
}

BC_AUTO_TEST_CASE(error_t__code__socks_password__true_expected_message)
{
    constexpr auto value = error::socks_password;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "socks password too long");
}

BC_AUTO_TEST_CASE(error_t__code__socks_host_name__true_expected_message)
{
    constexpr auto value = error::socks_host_name;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "socks host name too long");
}

BC_AUTO_TEST_CASE(error_t__code__socks_authentication__true_expected_message)
{
    constexpr auto value = error::socks_authentication;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "socks authentication failed");
}

BC_AUTO_TEST_CASE(error_t__code__socks_failure__true_expected_message)
{
    constexpr auto value = error::socks_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "socks failure");
}

BC_AUTO_TEST_CASE(error_t__code__socks_disallowed__true_expected_message)
{
    constexpr auto value = error::socks_disallowed;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "socks connection disallowed");
}

BC_AUTO_TEST_CASE(error_t__code__socks_net_unreachable__true_expected_message)
{
    constexpr auto value = error::socks_net_unreachable;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "socks network unreachable");
}

BC_AUTO_TEST_CASE(error_t__code__socks_host_unreachable__true_expected_message)
{
    constexpr auto value = error::socks_host_unreachable;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "socks host unreachable");
}

BC_AUTO_TEST_CASE(error_t__code__socks_connection_refused__true_expected_message)
{
    constexpr auto value = error::socks_connection_refused;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "socks connection refused");
}

BC_AUTO_TEST_CASE(error_t__code__socks_connection_expired__true_expected_message)
{
    constexpr auto value = error::socks_connection_expired;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "socks connection expired");
}

BC_AUTO_TEST_CASE(error_t__code__socks_unsupported_command__true_expected_message)
{
    constexpr auto value = error::socks_unsupported_command;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "socks unsupported command");
}

BC_AUTO_TEST_CASE(error_t__code__socks_unsupported_address__true_expected_message)
{
    constexpr auto value = error::socks_unsupported_address;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "socks unsupported address");
}

BC_AUTO_TEST_CASE(error_t__code__socks_unassigned_failure__true_expected_message)
{
    constexpr auto value = error::socks_unassigned_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "socks unassigned failure");
}

BC_AUTO_TEST_CASE(error_t__code__socks_response_invalid__true_expected_message)
{
    constexpr auto value = error::socks_response_invalid;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "socks response invalid");
}

// sam

BC_AUTO_TEST_CASE(error_t__code__sam_response_invalid__true_expected_message)
{
    constexpr auto value = error::sam_response_invalid;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "sam response invalid");
}

BC_AUTO_TEST_CASE(error_t__code__sam_no_version__true_expected_message)
{
    constexpr auto value = error::sam_no_version;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "sam version not supported");
}

BC_AUTO_TEST_CASE(error_t__code__sam_cant_reach_peer__true_expected_message)
{
    constexpr auto value = error::sam_cant_reach_peer;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "sam cannot reach peer");
}

BC_AUTO_TEST_CASE(error_t__code__sam_duplicated_dest__true_expected_message)
{
    constexpr auto value = error::sam_duplicated_dest;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "sam duplicated destination");
}

BC_AUTO_TEST_CASE(error_t__code__sam_duplicated_id__true_expected_message)
{
    constexpr auto value = error::sam_duplicated_id;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "sam duplicated session");
}

BC_AUTO_TEST_CASE(error_t__code__sam_i2p_error__true_expected_message)
{
    constexpr auto value = error::sam_i2p_error;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "sam i2p error");
}

BC_AUTO_TEST_CASE(error_t__code__sam_invalid_id__true_expected_message)
{
    constexpr auto value = error::sam_invalid_id;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "sam invalid session");
}

BC_AUTO_TEST_CASE(error_t__code__sam_invalid_key__true_expected_message)
{
    constexpr auto value = error::sam_invalid_key;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "sam invalid key");
}

BC_AUTO_TEST_CASE(error_t__code__sam_key_not_found__true_expected_message)
{
    constexpr auto value = error::sam_key_not_found;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "sam key not found");
}

BC_AUTO_TEST_CASE(error_t__code__sam_peer_not_found__true_expected_message)
{
    constexpr auto value = error::sam_peer_not_found;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "sam peer not found");
}

BC_AUTO_TEST_CASE(error_t__code__sam_timeout__true_expected_message)
{
    constexpr auto value = error::sam_timeout;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "sam timeout");
}

BC_AUTO_TEST_CASE(error_t__code__sam_unassigned_failure__true_expected_message)
{
    constexpr auto value = error::sam_unassigned_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "sam unassigned failure");
}

// tls

BC_AUTO_TEST_CASE(error_t__code__tls_use_certificate__true_expected_message)
{
    constexpr auto value = error::tls_use_certificate;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "failed to set tls certificate");
}

BC_AUTO_TEST_CASE(error_t__code__tls_use_private_key__true_expected_message)
{
    constexpr auto value = error::tls_use_private_key;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "failed to set tls private key");
}

BC_AUTO_TEST_CASE(error_t__code__tls_set_add_verify__true_expected_message)
{
    constexpr auto value = error::tls_set_add_verify;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "failed to set tls certificate authority");
}

BC_AUTO_TEST_CASE(error_t__code__tls_stream_truncated__true_expected_message)
{
    constexpr auto value = error::tls_stream_truncated;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls stream truncated");
}

// tls alerts

BC_AUTO_TEST_CASE(error_t__code__tls_alert_unexpected_message__true_expected_message)
{
    constexpr auto value = error::tls_alert_unexpected_message;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert unexpected message");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_bad_record_mac__true_expected_message)
{
    constexpr auto value = error::tls_alert_bad_record_mac;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert bad record mac");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_record_overflow__true_expected_message)
{
    constexpr auto value = error::tls_alert_record_overflow;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert record overflow");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_handshake_failure__true_expected_message)
{
    constexpr auto value = error::tls_alert_handshake_failure;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert handshake failure");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_bad_certificate__true_expected_message)
{
    constexpr auto value = error::tls_alert_bad_certificate;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert bad certificate");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_unsupported_certificate__true_expected_message)
{
    constexpr auto value = error::tls_alert_unsupported_certificate;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert unsupported certificate");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_certificate_expired__true_expected_message)
{
    constexpr auto value = error::tls_alert_certificate_expired;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert certificate expired");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_certificate_unknown__true_expected_message)
{
    constexpr auto value = error::tls_alert_certificate_unknown;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert certificate unknown");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_illegal_parameter__true_expected_message)
{
    constexpr auto value = error::tls_alert_illegal_parameter;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert illegal parameter");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_unknown_ca__true_expected_message)
{
    constexpr auto value = error::tls_alert_unknown_ca;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert unknown ca");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_decode_error__true_expected_message)
{
    constexpr auto value = error::tls_alert_decode_error;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert decode error");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_decrypt_error__true_expected_message)
{
    constexpr auto value = error::tls_alert_decrypt_error;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert decrypt error");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_protocol_version__true_expected_message)
{
    constexpr auto value = error::tls_alert_protocol_version;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert protocol version");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_internal_error__true_expected_message)
{
    constexpr auto value = error::tls_alert_internal_error;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert internal error");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_missing_extension__true_expected_message)
{
    constexpr auto value = error::tls_alert_missing_extension;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert missing extension");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_certificate_required__true_expected_message)
{
    constexpr auto value = error::tls_alert_certificate_required;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert certificate required");
}

BC_AUTO_TEST_CASE(error_t__code__tls_alert_unknown__true_expected_message)
{
    constexpr auto value = error::tls_alert_unknown;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "tls alert");
}

// http 4xx client error

BC_AUTO_TEST_CASE(error_t__code__bad_request__true_expected_message)
{
    constexpr auto value = error::bad_request;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad request");
}

BC_AUTO_TEST_CASE(error_t__code__unauthorized__true_expected_message)
{
    constexpr auto value = error::unauthorized;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "unauthorized");
}

////BC_AUTO_TEST_CASE(error_t__code__payment_required__true_expected_message)
////{
////    constexpr auto value = error::payment_required;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "payment required");
////}

BC_AUTO_TEST_CASE(error_t__code__forbidden__true_expected_message)
{
    constexpr auto value = error::forbidden;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "forbidden");
}

BC_AUTO_TEST_CASE(error_t__code__not_found__true_expected_message)
{
    constexpr auto value = error::not_found;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not found");
}

BC_AUTO_TEST_CASE(error_t__code__method_not_allowed__true_expected_message)
{
    constexpr auto value = error::method_not_allowed;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "method not allowed");
}

BC_AUTO_TEST_CASE(error_t__code__not_acceptable__true_expected_message)
{
    constexpr auto value = error::not_acceptable;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not acceptable");
}

////BC_AUTO_TEST_CASE(error_t__code__proxy_authentication_required__true_expected_message)
////{
////    constexpr auto value = error::proxy_authentication_required;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "proxy authentication required");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__request_timeout__true_expected_message)
////{
////    constexpr auto value = error::request_timeout;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "request timeout");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__conflict__true_expected_message)
////{
////    constexpr auto value = error::conflict;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "conflict");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__gone__true_expected_message)
////{
////    constexpr auto value = error::gone;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "gone");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__length_required__true_expected_message)
////{
////    constexpr auto value = error::length_required;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "length required");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__precondition_failed__true_expected_message)
////{
////    constexpr auto value = error::precondition_failed;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "precondition failed");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__payload_too_large__true_expected_message)
////{
////    constexpr auto value = error::payload_too_large;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "payload too large");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__uri_too_long__true_expected_message)
////{
////    constexpr auto value = error::uri_too_long;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "uri too long");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__unsupported_media_type__true_expected_message)
////{
////    constexpr auto value = error::unsupported_media_type;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "unsupported media type");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__range_not_satisfiable__true_expected_message)
////{
////    constexpr auto value = error::range_not_satisfiable;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "range not satisfiable");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__expectation_failed__true_expected_message)
////{
////    constexpr auto value = error::expectation_failed;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "expectation failed");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__im_a_teapot__true_expected_message)
////{
////    constexpr auto value = error::im_a_teapot;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "im a teapot");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__misdirected_request__true_expected_message)
////{
////    constexpr auto value = error::misdirected_request;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "misdirected request");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__unprocessable_entity__true_expected_message)
////{
////    constexpr auto value = error::unprocessable_entity;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "unprocessable entity");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__locked__true_expected_message)
////{
////    constexpr auto value = error::locked;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "locked");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__failed_dependency__true_expected_message)
////{
////    constexpr auto value = error::failed_dependency;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "failed dependency");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__too_early__true_expected_message)
////{
////    constexpr auto value = error::too_early;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "too early");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__upgrade_required__true_expected_message)
////{
////    constexpr auto value = error::upgrade_required;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "upgrade required");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__precondition_required__true_expected_message)
////{
////    constexpr auto value = error::precondition_required;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "precondition required");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__too_many_requests__true_expected_message)
////{
////    constexpr auto value = error::too_many_requests;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "too many requests");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__request_header_fields_too_large__true_expected_message)
////{
////    constexpr auto value = error::request_header_fields_too_large;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "request header fields too large");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__unavailable_for_legal_reasons__true_expected_message)
////{
////    constexpr auto value = error::unavailable_for_legal_reasons;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "unavailable for legal reasons");
////}

// http 5xx server error

BC_AUTO_TEST_CASE(error_t__code__internal_server_error__true_expected_message)
{
    constexpr auto value = error::internal_server_error;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "internal server error");
}

BC_AUTO_TEST_CASE(error_t__code__not_implemented__true_expected_message)
{
    constexpr auto value = error::not_implemented;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not implemented");
}

////BC_AUTO_TEST_CASE(error_t__code__bad_gateway__true_expected_message)
////{
////    constexpr auto value = error::bad_gateway;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "bad gateway");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__service_unavailable__true_expected_message)
////{
////    constexpr auto value = error::service_unavailable;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "service unavailable");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__gateway_timeout__true_expected_message)
////{
////    constexpr auto value = error::gateway_timeout;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "gateway timeout");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__http_version_not_supported__true_expected_message)
////{
////    constexpr auto value = error::http_version_not_supported;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "http version not supported");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__variant_also_negotiates__true_expected_message)
////{
////    constexpr auto value = error::variant_also_negotiates;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "variant also negotiates");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__insufficient_storage__true_expected_message)
////{
////    constexpr auto value = error::insufficient_storage;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "insufficient storage");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__loop_detected__true_expected_message)
////{
////    constexpr auto value = error::loop_detected;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "loop detected");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__not_extended__true_expected_message)
////{
////    constexpr auto value = error::not_extended;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "not extended");
////}
////
////BC_AUTO_TEST_CASE(error_t__code__network_authentication_required__true_expected_message)
////{
////    constexpr auto value = error::network_authentication_required;
////    const auto ec = code(value);
////    BC_REQUIRE(ec);
////    BC_REQUIRE(ec == value);
////    BC_REQUIRE_EQUAL(ec.message(), "network authentication required");
////}

// http error

BC_AUTO_TEST_CASE(error_t__code__end_of_stream__true_expected_message)
{
    constexpr auto value = error::end_of_stream;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "end of stream");
}

BC_AUTO_TEST_CASE(error_t__code__partial_message__true_expected_message)
{
    constexpr auto value = error::partial_message;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "partial message");
}

BC_AUTO_TEST_CASE(error_t__code__need_more__true_expected_message)
{
    constexpr auto value = error::need_more;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "need more");
}

BC_AUTO_TEST_CASE(error_t__code__unexpected_body__true_expected_message)
{
    constexpr auto value = error::unexpected_body;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "unexpected body");
}

BC_AUTO_TEST_CASE(error_t__code__need_buffer__true_expected_message)
{
    constexpr auto value = error::need_buffer;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "need buffer");
}

BC_AUTO_TEST_CASE(error_t__code__end_of_chunk__true_expected_message)
{
    constexpr auto value = error::end_of_chunk;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "end of chunk");
}

BC_AUTO_TEST_CASE(error_t__code__buffer_overflow__true_expected_message)
{
    constexpr auto value = error::buffer_overflow;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "buffer overflow");
}

BC_AUTO_TEST_CASE(error_t__code__header_limit__true_expected_message)
{
    constexpr auto value = error::header_limit;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "header limit");
}

BC_AUTO_TEST_CASE(error_t__code__body_limit__true_expected_message)
{
    constexpr auto value = error::body_limit;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "body limit");
}

BC_AUTO_TEST_CASE(error_t__code__bad_alloc__true_expected_message)
{
    constexpr auto value = error::bad_alloc;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad alloc");
}

BC_AUTO_TEST_CASE(error_t__code__bad_line_ending__true_expected_message)
{
    constexpr auto value = error::bad_line_ending;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad line ending");
}

BC_AUTO_TEST_CASE(error_t__code__bad_method__true_expected_message)
{
    constexpr auto value = error::bad_method;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad method");
}

BC_AUTO_TEST_CASE(error_t__code__bad_target__true_expected_message)
{
    constexpr auto value = error::bad_target;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad target");
}

BC_AUTO_TEST_CASE(error_t__code__bad_version__true_expected_message)
{
    constexpr auto value = error::bad_version;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad version");
}

BC_AUTO_TEST_CASE(error_t__code__bad_status__true_expected_message)
{
    constexpr auto value = error::bad_status;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad status");
}

BC_AUTO_TEST_CASE(error_t__code__bad_reason__true_expected_message)
{
    constexpr auto value = error::bad_reason;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad reason");
}

BC_AUTO_TEST_CASE(error_t__code__bad_field__true_expected_message)
{
    constexpr auto value = error::bad_field;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad field");
}

BC_AUTO_TEST_CASE(error_t__code__bad_value__true_expected_message)
{
    constexpr auto value = error::bad_value;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad value");
}

BC_AUTO_TEST_CASE(error_t__code__bad_content_length__true_expected_message)
{
    constexpr auto value = error::bad_content_length;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad content length");
}

BC_AUTO_TEST_CASE(error_t__code__bad_transfer_encoding__true_expected_message)
{
    constexpr auto value = error::bad_transfer_encoding;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad transfer encoding");
}

BC_AUTO_TEST_CASE(error_t__code__bad_chunk__true_expected_message)
{
    constexpr auto value = error::bad_chunk;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad chunk");
}

BC_AUTO_TEST_CASE(error_t__code__bad_chunk_extension__true_expected_message)
{
    constexpr auto value = error::bad_chunk_extension;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad chunk extension");
}

BC_AUTO_TEST_CASE(error_t__code__bad_obs_fold__true_expected_message)
{
    constexpr auto value = error::bad_obs_fold;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad obs fold");
}

BC_AUTO_TEST_CASE(error_t__code__multiple_content_length__true_expected_message)
{
    constexpr auto value = error::multiple_content_length;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "multiple content length");
}

BC_AUTO_TEST_CASE(error_t__code__stale_parser__true_expected_message)
{
    constexpr auto value = error::stale_parser;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "stale parser");
}

BC_AUTO_TEST_CASE(error_t__code__short_read__true_expected_message)
{
    constexpr auto value = error::short_read;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "short read");
}

BC_AUTO_TEST_CASE(error_t__code__http_unknown__true_expected_message)
{
    constexpr auto value = error::http_unknown;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "http error");
}

// websocket error

BC_AUTO_TEST_CASE(error_t__code__websocket_closed__true_expected_message)
{
    constexpr auto value = error::websocket_closed;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "websocket closed");
}

BC_AUTO_TEST_CASE(error_t__code__websocket_buffer_overflow__true_expected_message)
{
    constexpr auto value = error::websocket_buffer_overflow;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "websocket buffer overflow");
}

BC_AUTO_TEST_CASE(error_t__code__partial_deflate_block__true_expected_message)
{
    constexpr auto value = error::partial_deflate_block;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "partial deflate block");
}

BC_AUTO_TEST_CASE(error_t__code__message_too_big__true_expected_message)
{
    constexpr auto value = error::message_too_big;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "message too big");
}

BC_AUTO_TEST_CASE(error_t__code__bad_http_version__true_expected_message)
{
    constexpr auto value = error::bad_http_version;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad http version");
}

BC_AUTO_TEST_CASE(error_t__code__websocket_bad_method__true_expected_message)
{
    constexpr auto value = error::websocket_bad_method;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "websocket bad method");
}

BC_AUTO_TEST_CASE(error_t__code__no_host__true_expected_message)
{
    constexpr auto value = error::no_host;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "no host");
}

BC_AUTO_TEST_CASE(error_t__code__no_connection__true_expected_message)
{
    constexpr auto value = error::no_connection;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "no connection");
}

BC_AUTO_TEST_CASE(error_t__code__no_connection_upgrade__true_expected_message)
{
    constexpr auto value = error::no_connection_upgrade;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "no connection upgrade");
}

BC_AUTO_TEST_CASE(error_t__code__no_upgrade__true_expected_message)
{
    constexpr auto value = error::no_upgrade;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "no upgrade");
}

BC_AUTO_TEST_CASE(error_t__code__no_upgrade_websocket__true_expected_message)
{
    constexpr auto value = error::no_upgrade_websocket;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "no upgrade websocket");
}

BC_AUTO_TEST_CASE(error_t__code__no_sec_key__true_expected_message)
{
    constexpr auto value = error::no_sec_key;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "no sec key");
}

BC_AUTO_TEST_CASE(error_t__code__bad_sec_key__true_expected_message)
{
    constexpr auto value = error::bad_sec_key;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad sec key");
}

BC_AUTO_TEST_CASE(error_t__code__no_sec_version__true_expected_message)
{
    constexpr auto value = error::no_sec_version;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "no sec version");
}

BC_AUTO_TEST_CASE(error_t__code__bad_sec_version__true_expected_message)
{
    constexpr auto value = error::bad_sec_version;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad sec version");
}

BC_AUTO_TEST_CASE(error_t__code__no_sec_accept__true_expected_message)
{
    constexpr auto value = error::no_sec_accept;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "no sec accept");
}

BC_AUTO_TEST_CASE(error_t__code__bad_sec_accept__true_expected_message)
{
    constexpr auto value = error::bad_sec_accept;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad sec accept");
}

BC_AUTO_TEST_CASE(error_t__code__upgrade_declined__true_expected_message)
{
    constexpr auto value = error::upgrade_declined;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "upgrade declined");
}

BC_AUTO_TEST_CASE(error_t__code__bad_opcode__true_expected_message)
{
    constexpr auto value = error::bad_opcode;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad opcode");
}

BC_AUTO_TEST_CASE(error_t__code__bad_data_frame__true_expected_message)
{
    constexpr auto value = error::bad_data_frame;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad data frame");
}

BC_AUTO_TEST_CASE(error_t__code__bad_continuation__true_expected_message)
{
    constexpr auto value = error::bad_continuation;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad continuation");
}

BC_AUTO_TEST_CASE(error_t__code__bad_reserved_bits__true_expected_message)
{
    constexpr auto value = error::bad_reserved_bits;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad reserved bits");
}

BC_AUTO_TEST_CASE(error_t__code__bad_control_fragment__true_expected_message)
{
    constexpr auto value = error::bad_control_fragment;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad control fragment");
}

BC_AUTO_TEST_CASE(error_t__code__bad_control_size__true_expected_message)
{
    constexpr auto value = error::bad_control_size;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad control size");
}

BC_AUTO_TEST_CASE(error_t__code__bad_unmasked_frame__true_expected_message)
{
    constexpr auto value = error::bad_unmasked_frame;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad unmasked frame");
}

BC_AUTO_TEST_CASE(error_t__code__bad_masked_frame__true_expected_message)
{
    constexpr auto value = error::bad_masked_frame;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad masked frame");
}

BC_AUTO_TEST_CASE(error_t__code__bad_size__true_expected_message)
{
    constexpr auto value = error::bad_size;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad size");
}

BC_AUTO_TEST_CASE(error_t__code__bad_frame_payload__true_expected_message)
{
    constexpr auto value = error::bad_frame_payload;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad frame payload");
}

BC_AUTO_TEST_CASE(error_t__code__bad_close_code__true_expected_message)
{
    constexpr auto value = error::bad_close_code;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad close code");
}

BC_AUTO_TEST_CASE(error_t__code__bad_close_size__true_expected_message)
{
    constexpr auto value = error::bad_close_size;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad close size");
}

BC_AUTO_TEST_CASE(error_t__code__bad_close_payload__true_expected_message)
{
    constexpr auto value = error::bad_close_payload;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "bad close payload");
}

// boost json error

BC_AUTO_TEST_CASE(error_t__code__syntax__true_expected_message)
{
    constexpr auto value = error::syntax;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "syntax error");
}

BC_AUTO_TEST_CASE(error_t__code__extra_data__true_expected_message)
{
    constexpr auto value = error::extra_data;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "extra data");
}

BC_AUTO_TEST_CASE(error_t__code__incomplete__true_expected_message)
{
    constexpr auto value = error::incomplete;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "incomplete json");
}

BC_AUTO_TEST_CASE(error_t__code__exponent_overflow__true_expected_message)
{
    constexpr auto value = error::exponent_overflow;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "exponent too large");
}

BC_AUTO_TEST_CASE(error_t__code__too_deep__true_expected_message)
{
    constexpr auto value = error::too_deep;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "too deep");
}

BC_AUTO_TEST_CASE(error_t__code__illegal_leading_surrogate__true_expected_message)
{
    constexpr auto value = error::illegal_leading_surrogate;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "illegal leading surrogate");
}

BC_AUTO_TEST_CASE(error_t__code__illegal_trailing_surrogate__true_expected_message)
{
    constexpr auto value = error::illegal_trailing_surrogate;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "illegal trailing surrogate");
}

BC_AUTO_TEST_CASE(error_t__code__invalid_utf8__true_expected_message)
{
    constexpr auto value = error::invalid_utf8;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "invalid utf8");
}

BC_AUTO_TEST_CASE(error_t__code__expected_hex_digit__true_expected_message)
{
    constexpr auto value = error::expected_hex_digit;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "expected hex digit");
}

BC_AUTO_TEST_CASE(error_t__code__expected_utf16_escape__true_expected_message)
{
    constexpr auto value = error::expected_utf16_escape;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "expected utf16 escape");
}

BC_AUTO_TEST_CASE(error_t__code__object_too_large__true_expected_message)
{
    constexpr auto value = error::object_too_large;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "object too large");
}

BC_AUTO_TEST_CASE(error_t__code__array_too_large__true_expected_message)
{
    constexpr auto value = error::array_too_large;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "array too large");
}

BC_AUTO_TEST_CASE(error_t__code__key_too_large__true_expected_message)
{
    constexpr auto value = error::key_too_large;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "key too large");
}

BC_AUTO_TEST_CASE(error_t__code__string_too_large__true_expected_message)
{
    constexpr auto value = error::string_too_large;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "string too large");
}

BC_AUTO_TEST_CASE(error_t__code__number_too_large__true_expected_message)
{
    constexpr auto value = error::number_too_large;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "number too large");
}

BC_AUTO_TEST_CASE(error_t__code__out_of_range__true_expected_message)
{
    constexpr auto value = error::out_of_range;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "out of range");
}

BC_AUTO_TEST_CASE(error_t__code__invalid_escape__true_expected_message)
{
    constexpr auto value = error::invalid_escape;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "invalid escape");
}

BC_AUTO_TEST_CASE(error_t__code__not_number__true_expected_message)
{
    constexpr auto value = error::not_number;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not number");
}

BC_AUTO_TEST_CASE(error_t__code__not_exact__true_expected_message)
{
    constexpr auto value = error::not_exact;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not exact");
}

BC_AUTO_TEST_CASE(error_t__code__not_null__true_expected_message)
{
    constexpr auto value = error::not_null;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not null");
}

BC_AUTO_TEST_CASE(error_t__code__not_bool__true_expected_message)
{
    constexpr auto value = error::not_bool;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not bool");
}

BC_AUTO_TEST_CASE(error_t__code__not_array__true_expected_message)
{
    constexpr auto value = error::not_array;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not array");
}

BC_AUTO_TEST_CASE(error_t__code__not_object__true_expected_message)
{
    constexpr auto value = error::not_object;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not object");
}

BC_AUTO_TEST_CASE(error_t__code__not_string__true_expected_message)
{
    constexpr auto value = error::not_string;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not string");
}

BC_AUTO_TEST_CASE(error_t__code__not_int64__true_expected_message)
{
    constexpr auto value = error::not_int64;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not int64");
}

BC_AUTO_TEST_CASE(error_t__code__not_uint64__true_expected_message)
{
    constexpr auto value = error::not_uint64;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not uint64");
}

BC_AUTO_TEST_CASE(error_t__code__not_double__true_expected_message)
{
    constexpr auto value = error::not_double;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "not double");
}

BC_AUTO_TEST_CASE(error_t__code__size_mismatch__true_expected_message)
{
    constexpr auto value = error::size_mismatch;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "size mismatch");
}

BC_AUTO_TEST_CASE(error_t__code__exhausted_variants__true_expected_message)
{
    constexpr auto value = error::exhausted_variants;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "exhausted variants");
}

BC_AUTO_TEST_CASE(error_t__code__json_unknown__true_expected_message)
{
    constexpr auto value = error::json_unknown;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "json error");
}

// query string parse error

BC_AUTO_TEST_CASE(error_t__code__message_overflow__true_expected_message)
{
    constexpr auto value = error::message_overflow;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "message overflow");
}

BC_AUTO_TEST_CASE(error_t__code__undefined_type__true_expected_message)
{
    constexpr auto value = error::undefined_type;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "undefined type");
}

BC_AUTO_TEST_CASE(error_t__code__unexpected_method__true_expected_message)
{
    constexpr auto value = error::unexpected_method;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "unexpected method");
}

BC_AUTO_TEST_CASE(error_t__code__unexpected_type__true_expected_message)
{
    constexpr auto value = error::unexpected_type;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "unexpected type");
}

BC_AUTO_TEST_CASE(error_t__code__extra_positional__true_expected_message)
{
    constexpr auto value = error::extra_positional;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "extra positional");
}

BC_AUTO_TEST_CASE(error_t__code__extra_named__true_expected_message)
{
    constexpr auto value = error::extra_named;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "extra named");
}

BC_AUTO_TEST_CASE(error_t__code__missing_array__true_expected_message)
{
    constexpr auto value = error::missing_array;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "missing array");
}

BC_AUTO_TEST_CASE(error_t__code__missing_object__true_expected_message)
{
    constexpr auto value = error::missing_object;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "missing object");
}

BC_AUTO_TEST_CASE(error_t__code__missing_parameter__true_expected_message)
{
    constexpr auto value = error::missing_parameter;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "missing parameter");
}

// json-rpc error

BC_AUTO_TEST_CASE(error_t__code__jsonrpc_requires_method__true_expected_message)
{
    constexpr auto value = error::jsonrpc_requires_method;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "jsonrpc requires method");
}

BC_AUTO_TEST_CASE(error_t__code__jsonrpc_v1_requires_params__true_expected_message)
{
    constexpr auto value = error::jsonrpc_v1_requires_params;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "jsonrpc v1 requires params");
}

BC_AUTO_TEST_CASE(error_t__code__jsonrpc_v1_requires_array_params__true_expected_message)
{
    constexpr auto value = error::jsonrpc_v1_requires_array_params;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "jsonrpc v1 requires array params");
}

BC_AUTO_TEST_CASE(error_t__code__jsonrpc_v1_requires_id__true_expected_message)
{
    constexpr auto value = error::jsonrpc_v1_requires_id;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "jsonrpc v1 requires id");
}

BC_AUTO_TEST_CASE(error_t__code__jsonrpc_params_not_collection__true_expected_message)
{
    constexpr auto value = error::jsonrpc_params_not_collection;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "jsonrpc params not collection");
}

BC_AUTO_TEST_CASE(error_t__code__jsonrpc_reader_stall__true_expected_message)
{
    constexpr auto value = error::jsonrpc_reader_stall;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "jsonrpc reader stall");
}

BC_AUTO_TEST_CASE(error_t__code__jsonrpc_reader_exception__true_expected_message)
{
    constexpr auto value = error::jsonrpc_reader_exception;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "jsonrpc reader exception");
}

BC_AUTO_TEST_CASE(error_t__code__jsonrpc_writer_exception__true_expected_message)
{
    constexpr auto value = error::jsonrpc_writer_exception;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "jsonrpc writer exception");
}

BC_AUTO_TEST_CASE(error_t__code__would_block__true_expected_message)
{
    constexpr auto value = error::would_block;
    const auto ec = code(value);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec == value);
    BC_REQUIRE_EQUAL(ec.message(), "would block");
}

// mapping
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(to_error_code__default_code__success)
{
    BC_REQUIRE_EQUAL(error::http_to_error_code(code{}), error::success);
    BC_REQUIRE_EQUAL(error::json_to_error_code(code{}), error::success);
    BC_REQUIRE_EQUAL(error::rpc_to_error_code(code{}), error::success);
}

BC_AUTO_TEST_CASE(http_to_error_code__beast_codes__expected)
{
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::end_of_stream)), error::end_of_stream);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::partial_message)), error::partial_message);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::need_more)), error::need_more);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::unexpected_body)), error::unexpected_body);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::need_buffer)), error::need_buffer);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::end_of_chunk)), error::end_of_chunk);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::buffer_overflow)), error::buffer_overflow);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::header_limit)), error::header_limit);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::body_limit)), error::body_limit);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::bad_alloc)), error::bad_alloc);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::bad_line_ending)), error::bad_line_ending);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::bad_method)), error::bad_method);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::bad_target)), error::bad_target);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::bad_version)), error::bad_version);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::bad_status)), error::bad_status);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::bad_reason)), error::bad_reason);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::bad_field)), error::bad_field);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::bad_value)), error::bad_value);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::bad_content_length)), error::bad_content_length);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::bad_transfer_encoding)), error::bad_transfer_encoding);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::bad_chunk)), error::bad_chunk);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::bad_chunk_extension)), error::bad_chunk_extension);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::bad_obs_fold)), error::bad_obs_fold);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::multiple_content_length)), error::multiple_content_length);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::stale_parser)), error::stale_parser);
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(error::http_error_t::short_read)), error::short_read);
}

BC_AUTO_TEST_CASE(http_to_error_code__unmapped_http_code__http_unknown)
{
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_http_code(static_cast<error::http_error_t>(0xff))), error::http_unknown);
}

BC_AUTO_TEST_CASE(http_to_error_code__json_code__json_mapping)
{
    BC_REQUIRE_EQUAL(error::http_to_error_code(error::to_json_code(error::json_error_t::json_syntax)), error::syntax);
}

BC_AUTO_TEST_CASE(tls_to_error_code__alerts__expected)
{
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::unexpected_message), error::tls_alert_unexpected_message);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::bad_record_mac), error::tls_alert_bad_record_mac);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::record_overflow), error::tls_alert_record_overflow);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::handshake_failure), error::tls_alert_handshake_failure);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::bad_certificate), error::tls_alert_bad_certificate);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::unsupported_certificate), error::tls_alert_unsupported_certificate);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::certificate_expired), error::tls_alert_certificate_expired);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::certificate_unknown), error::tls_alert_certificate_unknown);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::illegal_parameter), error::tls_alert_illegal_parameter);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::unknown_ca), error::tls_alert_unknown_ca);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::decode_error), error::tls_alert_decode_error);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::decrypt_error), error::tls_alert_decrypt_error);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::protocol_version), error::tls_alert_protocol_version);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::internal_error), error::tls_alert_internal_error);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::missing_extension), error::tls_alert_missing_extension);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::certificate_required), error::tls_alert_certificate_required);
}

BC_AUTO_TEST_CASE(tls_to_error_code__unmapped_alert__tls_alert_unknown)
{
    BC_REQUIRE_EQUAL(error::tls_to_error_code(0xff), error::tls_alert_unknown);
    BC_REQUIRE_EQUAL(error::tls_to_error_code(network::tls::alert::close_notify), error::tls_alert_unknown);
}

BC_AUTO_TEST_CASE(json_to_error_code__json_codes__expected)
{
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_syntax)), error::syntax);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_extra_data)), error::extra_data);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_incomplete)), error::incomplete);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_too_deep)), error::too_deep);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_invalid_escape)), error::invalid_escape);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_expected_hex_digit)), error::expected_hex_digit);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_expected_utf16_escape)), error::expected_utf16_escape);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_illegal_leading_surrogate)), error::illegal_leading_surrogate);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_illegal_trailing_surrogate)), error::illegal_trailing_surrogate);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_invalid_utf8)), error::invalid_utf8);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_exponent_overflow)), error::exponent_overflow);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_number_too_large)), error::number_too_large);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_string_too_large)), error::string_too_large);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_key_too_large)), error::key_too_large);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_array_too_large)), error::array_too_large);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_object_too_large)), error::object_too_large);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_not_null)), error::not_null);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_not_bool)), error::not_bool);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_not_number)), error::not_number);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_not_int64)), error::not_int64);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_not_uint64)), error::not_uint64);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_not_double)), error::not_double);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_not_string)), error::not_string);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_not_array)), error::not_array);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_not_object)), error::not_object);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_not_exact)), error::not_exact);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_out_of_range)), error::out_of_range);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_size_mismatch)), error::size_mismatch);
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(error::json_error_t::json_exhausted_variants)), error::exhausted_variants);
}

BC_AUTO_TEST_CASE(json_to_error_code__unmapped_json_code__json_unknown)
{
    BC_REQUIRE_EQUAL(error::json_to_error_code(error::to_json_code(static_cast<error::json_error_t>(0xff))), error::json_unknown);
}

BC_AUTO_TEST_CASE(json_to_error_code__network_code__same_code)
{
    BC_REQUIRE_EQUAL(error::json_to_error_code(code(error::channel_timeout)), error::channel_timeout);
}

BC_AUTO_TEST_CASE(json_to_error_code__other_category__unknown)
{
    BC_REQUIRE_EQUAL(error::json_to_error_code(std::make_error_code(std::errc::timed_out)), error::unknown);
}

BC_AUTO_TEST_CASE(rpc_to_error_code__network_code__same_code)
{
    BC_REQUIRE_EQUAL(error::rpc_to_error_code(code(error::channel_timeout)), error::channel_timeout);
}

BC_AUTO_TEST_CASE(rpc_to_error_code__http_code__http_mapping)
{
    BC_REQUIRE_EQUAL(error::rpc_to_error_code(error::to_http_code(error::http_error_t::bad_method)), error::bad_method);
}

BC_AUTO_TEST_SUITE_END()
