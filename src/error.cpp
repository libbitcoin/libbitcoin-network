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
#include <bitcoin/network/error.hpp>

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace error {

DEFINE_ERROR_T_MESSAGE_MAP(error)
{
    { success, "success" },
    { unknown, "unknown error" },
    { upgraded, "upgraded" },

    // addresses
    { address_invalid, "address invalid" },
    { address_not_found, "address not found" },
    { address_disabled, "address protocol disabled" },
    { address_unsupported, "advertised services unsupported" },
    { address_insufficient, "advertised services insufficient" },
    { seeding_unsuccessful, "seeding unsuccessful" },
    { seeding_complete, "seeding complete" },
        
    // file system
    { file_load, "failed to load file" },
    { file_save, "failed to save file" },
    { file_system, "file system error" },
    { file_exception, "file exception" },

    // general I/O failures
    { bad_stream, "bad data stream" },
    { not_allowed, "not allowed" },
    { peer_disconnect, "peer disconnect" },
    { peer_unsupported, "peer unsupported" },
    { peer_insufficient, "peer insufficient" },
    { peer_timestamp, "peer timestamp" },
    { protocol_violation, "protocol violation" },
    { channel_overflow, "channel overflow" },
    { channel_underflow, "channel underflow" },

    // incoming connection failures
    { listen_failed, "incoming connection failed" },
    { accept_failed, "connection to self aborted" },
    { oversubscribed, "service oversubscribed" },

    // incoming/outgoing connection failures
    { address_blocked, "address blocked by policy" },
        
    // outgoing connection failures
    { address_in_use, "address already in use" },
    { resolve_failed, "resolving hostname failed" },
    { connect_failed, "unable to reach remote host" },

    // heading read failures
    { invalid_heading, "invalid message heading" },
    { invalid_magic, "invalid message heading magic" },

    // payload read failures
    { oversized_payload, "oversize message payload" },
    { invalid_checksum, "invalid message checksum" },
    { invalid_message, "message failed to deserialize" },
    { unknown_message, "unknown message type" },

    // general failures
    { invalid_configuration, "invalid configuration" },
    { operation_timeout, "operation timed out" },
    { operation_canceled, "operation canceled" },
    { operation_failed, "operation failed" },

    // termination
    { channel_timeout, "channel timed out" },
    { channel_conflict, "channel conflict" },
    { channel_dropped, "channel dropped" },
    { channel_expired, "channel expired" },
    { channel_inactive, "channel inactive" },
    { channel_stopped, "channel stopped" },
    { service_stopped, "service stopped" },
    { service_suspended, "service suspended" },
    { subscriber_exists, "subscriber exists" },
    { subscriber_stopped, "subscriber stopped" },
    { desubscribed, "subscriber desubscribed" },

    // socks5
    { socks_method, "socks method not supported" },
    { socks_username, "socks username too long" },
    { socks_password, "socks password too long" },
    { socks_host_name, "socks host name too long" },
    { socks_authentication, "socks authentication failed" },
    { socks_failure, "socks failure" },
    { socks_disallowed, "socks connection disallowed" },
    { socks_net_unreachable, "socks network unreachable" },
    { socks_host_unreachable, "socks host unreachable" },
    { socks_connection_refused, "socks connection refused" },
    { socks_connection_expired, "socks connection expired" },
    { socks_unsupported_command, "socks unsupported command" },
    { socks_unsupported_address, "socks unsupported address" },
    { socks_unassigned_failure, "socks unassigned failure" },
    { socks_response_invalid, "socks response invalid" },

    // tls
    { tls_set_options, "failed to set tls options" },
    { tls_use_certificate, "failed to set tls certificate" },
    { tls_use_private_key, "failed to set tls private key" },
    { tls_set_password, "failed to set tls private key password" },
    { tls_set_default_verify, "failed to set tls default certificate authority" },
    { tls_set_add_verify, "failed to set tls certificate authority" },
    { tls_stream_truncated, "tls stream truncated" },
    { tls_unspecified_system_error, "tls unspecified system error" },
    { tls_unexpected_result, "tls unexpected result" },

    ////// http 4xx client error
    { bad_request, "bad request" },
    ////{ unauthorized, "unauthorized" },
    ////{ payment_required, "payment required" },
    { forbidden, "forbidden" },
    { not_found, "not found" },
    { method_not_allowed, "method not allowed" },
    ////{ not_acceptable, "not acceptable" },
    ////{ proxy_authentication_required, "proxy authentication required" },
    ////{ request_timeout, "request timeout" },
    ////{ conflict, "conflict" },
    ////{ gone, "gone" },
    ////{ length_required, "length required" },
    ////{ precondition_failed, "precondition failed" },
    ////{ payload_too_large, "payload too large" },
    ////{ uri_too_long, "uri too long" },
    ////{ unsupported_media_type, "unsupported media type" },
    ////{ range_not_satisfiable, "range not satisfiable" },
    ////{ expectation_failed, "expectation failed" },
    ////{ im_a_teapot, "im a teapot" },
    ////{ misdirected_request, "misdirected request" },
    ////{ unprocessable_entity, "unprocessable entity" },
    ////{ locked, "locked" },
    ////{ failed_dependency, "failed dependency" },
    ////{ too_early, "too early" },
    ////{ upgrade_required, "upgrade required" },
    ////{ precondition_required, "precondition required" },
    ////{ too_many_requests, "too many requests" },
    ////{ request_header_fields_too_large, "request header fields too large" },
    ////{ unavailable_for_legal_reasons, "unavailable for legal reasons" },

    ////// http 5xx server error
    { internal_server_error, "internal server error" },
    { not_implemented, "not implemented" },
    ////{ bad_gateway, "bad gateway" },
    ////{ service_unavailable, "service unavailable" },
    ////{ gateway_timeout, "gateway timeout" },
    ////{ http_version_not_supported, "http version not supported" },
    ////{ variant_also_negotiates, "variant also negotiates" },
    ////{ insufficient_storage, "insufficient storage" },
    ////{ loop_detected, "loop detected" },
    ////{ not_extended, "not extended" },
    ////{ network_authentication_required, "network authentication required" },

    // boost beast error
    { end_of_stream, "end of stream" },
    { partial_message, "partial message" },
    { need_more, "need more" },
    { unexpected_body, "unexpected body" },
    { need_buffer, "need buffer" },
    { end_of_chunk, "end of chunk" },
    { buffer_overflow, "buffer overflow" },
    { header_limit, "header limit" },
    { body_limit, "body limit" },
    { bad_alloc, "bad alloc" },
    { bad_line_ending, "bad line ending" },
    { bad_method, "bad method" },
    { bad_target, "bad target" },
    { bad_version, "bad version" },
    { bad_status, "bad status" },
    { bad_reason, "bad reason" },
    { bad_field, "bad field" },
    { bad_value, "bad value" },
    { bad_content_length, "bad content length" },
    { bad_transfer_encoding, "bad transfer encoding" },
    { bad_chunk, "bad chunk" },
    { bad_chunk_extension, "bad chunk extension" },
    { bad_obs_fold, "bad obs fold" },
    { multiple_content_length, "multiple content length" },
    { stale_parser, "stale parser" },
    { short_read, "short read" },

    // boost beast websocket error
    { websocket_closed, "websocket closed" },
    { websocket_buffer_overflow, "websocket buffer overflow" },
    { partial_deflate_block, "partial deflate block" },
    { message_too_big, "message too big" },
    { bad_http_version, "bad http version" },
    { websocket_bad_method, "websocket bad method" },
    { no_host, "no host" },
    { no_connection, "no connection" },
    { no_connection_upgrade, "no connection upgrade" },
    { no_upgrade, "no upgrade" },
    { no_upgrade_websocket, "no upgrade websocket" },
    { no_sec_key, "no sec key" },
    { bad_sec_key, "bad sec key" },
    { no_sec_version, "no sec version" },
    { bad_sec_version, "bad sec version" },
    { no_sec_accept, "no sec accept" },
    { bad_sec_accept, "bad sec accept" },
    { upgrade_declined, "upgrade declined" },
    { bad_opcode, "bad opcode" },
    { bad_data_frame, "bad data frame" },
    { bad_continuation, "bad continuation" },
    { bad_reserved_bits, "bad reserved bits" },
    { bad_control_fragment, "bad control fragment" },
    { bad_control_size, "bad control size" },
    { bad_unmasked_frame, "bad unmasked frame" },
    { bad_masked_frame, "bad masked frame" },
    { bad_size, "bad size" },
    { bad_frame_payload, "bad frame payload" },
    { bad_close_code, "bad close code" },
    { bad_close_size, "bad close size" },
    { bad_close_payload, "bad close payload" },

    // boost json error
    { syntax, "syntax error" },
    { extra_data, "extra data" },
    { incomplete, "incomplete json" },
    { exponent_overflow, "exponent too large" },
    { too_deep, "too deep" },
    { illegal_leading_surrogate, "illegal leading surrogate" },
    { illegal_trailing_surrogate, "illegal trailing surrogate" },
    { expected_hex_digit, "expected hex digit" },
    { expected_utf16_escape, "expected utf16 escape" },
    { object_too_large, "object too large" },
    { array_too_large, "array too large" },
    { key_too_large, "key too large" },
    { string_too_large, "string too large" },
    { number_too_large, "number too large" },
    { input_error, "input error" },
    { exception, "exception" },
    { out_of_range, "out of range" },
    { test_failure, "test failure" },
    { missing_slash, "missing slash" },
    { invalid_escape, "invalid escape" },
    { token_not_number, "token not number" },
    { value_is_scalar, "value is scalar" },
    { json_not_found, "json not found" },
    { token_overflow, "token overflow" },
    { past_the_end, "past the end" },
    { not_number, "not number" },
    { not_exact, "not exact" },
    { not_null, "not null" },
    { not_bool, "not bool" },
    { not_array, "not array" },
    { not_object, "not object" },
    { not_string, "not string" },
    { not_int64, "not int64" },
    { not_uint64, "not uint64" },
    { not_double, "not double" },
    { not_integer, "not integer" },
    { size_mismatch, "size mismatch" },
    { exhausted_variants, "exhausted variants" },
    { unknown_name, "unknown name" },

    // rpc error
    { message_overflow, "message overflow" },
    { undefined_type, "undefined type" },
    { unexpected_method, "unexpected method" },
    { unexpected_type, "unexpected type" },
    { extra_positional, "extra positional" },
    { extra_named, "extra named" },
    { missing_array, "missing array" },
    { missing_object, "missing object" },
    { missing_parameter, "missing optional" }
};

DEFINE_ERROR_T_CATEGORY(error, "network", "network code")

// boost_code overloads the `==` operator to include category.
bool asio_is_canceled(const boost_code& ec) NOEXCEPT
{
    // self termination
    return ec == boost_error_t::operation_canceled
        || ec == boost::asio::error::operation_aborted;
}

// The success and operation_canceled codes are the only expected in normal
// operation, so these are first, to optimize the case where asio_is_canceled
// is not used. boost_code overloads the `==` operator to include category.
code asio_to_error_code(const boost_code& ec) NOEXCEPT
{
    if (!ec)
        return error::success;

    // self termination
    if (ec == boost_error_t::connection_aborted ||
        ec == boost_error_t::operation_canceled)
        return error::operation_canceled;

    // peer termination
    // stackoverflow.com/a/19891985/1172329
    if (ec == asio_misc_error_t::eof ||
        ec == boost_error_t::connection_reset)
        return error::peer_disconnect;

    // learn.microsoft.com/en-us/troubleshoot/windows-client/networking/
    // connect-tcp-greater-than-5000-error-wsaenobufs-10055
    if (ec == asio_system_error_t::no_buffer_space)
        return error::invalid_configuration;

    // network
    if (ec == boost_error_t::operation_not_permitted ||
        ec == boost_error_t::operation_not_supported ||
        ec == boost_error_t::owner_dead ||
        ec == boost_error_t::permission_denied)
        return error::not_allowed;

    // connect-resolve
    if (ec == boost_error_t::address_family_not_supported ||
        ec == boost_error_t::bad_address ||
        ec == boost_error_t::destination_address_required ||
        ec == asio_netdb_error_t::host_not_found ||
        ec == asio_netdb_error_t::host_not_found_try_again)
        return error::resolve_failed;

    // connect-connect
    if (ec == boost_error_t::address_not_available ||
        ec == boost_error_t::not_connected ||
        ec == boost_error_t::connection_refused ||
        ec == boost_error_t::broken_pipe ||
        ec == boost_error_t::host_unreachable ||
        ec == boost_error_t::network_down ||
        ec == boost_error_t::network_reset ||
        ec == boost_error_t::network_unreachable ||
        ec == boost_error_t::no_link ||
        ec == boost_error_t::no_protocol_option ||
        ec == boost_error_t::no_such_file_or_directory ||
        ec == boost_error_t::not_a_socket ||
        ec == boost_error_t::protocol_not_supported ||
        ec == boost_error_t::wrong_protocol_type)
        return error::connect_failed;

    // connect-address
    if (ec == boost_error_t::address_in_use ||
        ec == boost_error_t::already_connected ||
        ec == boost_error_t::connection_already_in_progress ||
        ec == boost_error_t::operation_in_progress)
        return error::address_in_use;

    // I/O (bad_file_descriptor if socket is not initialized)
    if (ec == boost_error_t::bad_file_descriptor ||
        ec == boost_error_t::bad_message ||
        ec == boost_error_t::illegal_byte_sequence ||
        ec == boost_error_t::io_error ||
        ec == boost_error_t::message_size ||
        ec == boost_error_t::no_message_available ||
        ec == boost_error_t::no_message ||
        ec == boost_error_t::no_stream_resources ||
        ec == boost_error_t::not_a_stream ||
        ec == boost_error_t::protocol_error)
        return error::bad_stream;

    // timeout
    if (ec == boost_error_t::stream_timeout ||
        ec == boost_error_t::timed_out)
        return error::channel_timeout;

    // file system errors (bad_file_descriptor used in I/O)
    if (ec == boost_error_t::cross_device_link ||
        ec == boost_error_t::device_or_resource_busy ||
        ec == boost_error_t::directory_not_empty ||
        ec == boost_error_t::executable_format_error ||
        ec == boost_error_t::file_exists ||
        ec == boost_error_t::file_too_large ||
        ec == boost_error_t::filename_too_long ||
        ec == boost_error_t::invalid_seek ||
        ec == boost_error_t::is_a_directory ||
        ec == boost_error_t::no_space_on_device ||
        ec == boost_error_t::no_such_device ||
        ec == boost_error_t::no_such_device_or_address ||
        ec == boost_error_t::read_only_file_system ||
        ec == boost_error_t::resource_unavailable_try_again ||
        ec == boost_error_t::text_file_busy ||
        ec == boost_error_t::too_many_files_open ||
        ec == boost_error_t::too_many_files_open_in_system ||
        ec == boost_error_t::too_many_links ||
        ec == boost_error_t::too_many_symbolic_link_levels)
        return error::file_system;

    ////// unknown
    ////if (ec == boost_error_t::argument_list_too_long ||
    ////    ec == boost_error_t::argument_out_of_domain ||
    ////    ec == boost_error_t::function_not_supported ||
    ////    ec == boost_error_t::identifier_removed ||
    ////    ec == boost_error_t::inappropriate_io_control_operation ||
    ////    ec == boost_error_t::interrupted ||
    ////    ec == boost_error_t::invalid_argument ||
    ////    ec == boost_error_t::no_buffer_space ||
    ////    ec == boost_error_t::no_child_process ||
    ////    ec == boost_error_t::no_lock_available ||
    ////    ec == boost_error_t::no_such_process ||
    ////    ec == boost_error_t::not_a_directory ||
    ////    ec == boost_error_t::not_enough_memory ||
    ////    ec == boost_error_t::operation_would_block ||
    ////    ec == boost_error_t::resource_deadlock_would_occur ||
    ////    ec == boost_error_t::result_out_of_range ||
    ////    ec == boost_error_t::state_not_recoverable ||
    ////    ec == boost_error_t::value_too_large)

    // TODO: return asio category generic error.
    return error::unknown;
}

// Boost defines this for error numbers produced by openssl. But we get generic
// error codes because WOLFSSL_HAVE_ERROR_QUEUE is not defined. This is because
// otherwise in some cases we fail to get failure results when necessary.
////typedef boost::asio::error::ssl_errors asio_ssl_error_t;
////const auto& category = boost::asio::error::get_ssl_category();

// includes asio codes
code ssl_to_error_code(const boost_code& ec) NOEXCEPT
{
    const auto& category = boost::asio::ssl::error::get_stream_category();

    if (!ec)
        return error::success;

    if (ec.category() != category)
        return asio_to_error_code(ec);

    switch (static_cast<asio_ssl_stream_error_t>(ec.value()))
    {
        case asio_ssl_stream_error_t::stream_truncated: return error::tls_stream_truncated;
        case asio_ssl_stream_error_t::unspecified_system_error: return error::tls_unspecified_system_error;
        case asio_ssl_stream_error_t::unexpected_result: return error::tls_unexpected_result;

        // TODO: return ssl category generic error.
        default: return error::unknown;
    }
}

// includes json codes
code http_to_error_code(const boost_code& ec) NOEXCEPT
{
    // TODO: use boost static initializer.
    static boost::beast::http::detail::http_error_category category{};

    if (!ec)
        return error::success;

    if (ec.category() != category)
        return json_to_error_code(ec);

    switch (static_cast<http_error_t>(ec.value()))
    {
        case http_error_t::end_of_stream: return error::end_of_stream;
        case http_error_t::partial_message: return error::partial_message;
        case http_error_t::need_more: return error::need_more;
        case http_error_t::unexpected_body: return error::unexpected_body;
        case http_error_t::need_buffer: return error::need_buffer;
        case http_error_t::end_of_chunk: return error::end_of_chunk;
        case http_error_t::buffer_overflow: return error::buffer_overflow;
        case http_error_t::header_limit: return error::header_limit;
        case http_error_t::body_limit: return error::body_limit;
        case http_error_t::bad_alloc: return error::bad_alloc;
        case http_error_t::bad_line_ending: return error::bad_line_ending;
        case http_error_t::bad_method: return error::bad_method;
        case http_error_t::bad_target: return error::bad_target;
        case http_error_t::bad_version: return error::bad_version;
        case http_error_t::bad_status: return error::bad_status;
        case http_error_t::bad_reason: return error::bad_reason;
        case http_error_t::bad_field: return error::bad_field;
        case http_error_t::bad_value: return error::bad_value;
        case http_error_t::bad_content_length: return error::bad_content_length;
        case http_error_t::bad_transfer_encoding: return error::bad_transfer_encoding;
        case http_error_t::bad_chunk: return error::bad_chunk;
        case http_error_t::bad_chunk_extension: return error::bad_chunk_extension;
        case http_error_t::bad_obs_fold: return error::bad_obs_fold;
        case http_error_t::multiple_content_length: return error::multiple_content_length;
        case http_error_t::stale_parser: return error::stale_parser;
        case http_error_t::short_read: return error::short_read;

        // TODO: return http category generic error.
        default: return error::unknown;
    }
}

// includes json codes
code ws_to_error_code(const boost_code& ec) NOEXCEPT
{
    // TODO: use boost static initializer.
    static boost::beast::websocket::detail::error_codes category{};

    if (!ec)
        return error::success;

    if (ec.category() != category)
        return http_to_error_code(ec);

    switch (static_cast<ws_error_t>(ec.value()))
    {
        case ws_error_t::closed: return error::websocket_closed;
        case ws_error_t::buffer_overflow: return error::websocket_buffer_overflow;
        case ws_error_t::partial_deflate_block: return error::partial_deflate_block;
        case ws_error_t::message_too_big: return error::message_too_big;
        case ws_error_t::bad_http_version: return error::bad_http_version;
        case ws_error_t::bad_method: return error::websocket_bad_method;
        case ws_error_t::no_host: return error::no_host;
        case ws_error_t::no_connection: return error::no_connection;
        case ws_error_t::no_connection_upgrade: return error::no_connection_upgrade;
        case ws_error_t::no_upgrade: return error::no_upgrade;
        case ws_error_t::no_upgrade_websocket: return error::no_upgrade_websocket;
        case ws_error_t::no_sec_key: return error::no_sec_key;
        case ws_error_t::bad_sec_key: return error::bad_sec_key;
        case ws_error_t::no_sec_version: return error::no_sec_version;
        case ws_error_t::bad_sec_version: return error::bad_sec_version;
        case ws_error_t::no_sec_accept: return error::no_sec_accept;
        case ws_error_t::bad_sec_accept: return error::bad_sec_accept;
        case ws_error_t::upgrade_declined: return error::upgrade_declined;
        case ws_error_t::bad_opcode: return error::bad_opcode;
        case ws_error_t::bad_data_frame: return error::bad_data_frame;
        case ws_error_t::bad_continuation: return error::bad_continuation;
        case ws_error_t::bad_reserved_bits: return error::bad_reserved_bits;
        case ws_error_t::bad_control_fragment: return error::bad_control_fragment;
        case ws_error_t::bad_control_size: return error::bad_control_size;
        case ws_error_t::bad_unmasked_frame: return error::bad_unmasked_frame;
        case ws_error_t::bad_masked_frame: return error::bad_masked_frame;
        case ws_error_t::bad_size: return error::bad_size;
        case ws_error_t::bad_frame_payload: return error::bad_frame_payload;
        case ws_error_t::bad_close_code: return error::bad_close_code;
        case ws_error_t::bad_close_size: return error::bad_close_size;
        case ws_error_t::bad_close_payload: return error::bad_close_payload;

        // TODO: return ws category generic error.
        default: return error::unknown;
    }
}

code json_to_error_code(const boost_code& ec) NOEXCEPT
{
    // TODO: use boost static initializer.
    static boost::json::detail::error_code_category_t category{};

    if (!ec)
        return error::success;

    if (ec.category() != category)
        return asio_to_error_code(ec);

    switch (static_cast<json_error_t>(ec.value()))
    {
        case json_error_t::syntax: return error::syntax;
        case json_error_t::extra_data: return error::extra_data;
        case json_error_t::incomplete: return error::incomplete;
        case json_error_t::exponent_overflow: return error::exponent_overflow;
        case json_error_t::too_deep: return error::too_deep;
        case json_error_t::illegal_leading_surrogate: return error::illegal_leading_surrogate;
        case json_error_t::illegal_trailing_surrogate: return error::illegal_trailing_surrogate;
        case json_error_t::expected_hex_digit: return error::expected_hex_digit;
        case json_error_t::expected_utf16_escape: return error::expected_utf16_escape;
        case json_error_t::object_too_large: return error::object_too_large;
        case json_error_t::array_too_large: return error::array_too_large;
        case json_error_t::key_too_large: return error::key_too_large;
        case json_error_t::string_too_large: return error::string_too_large;
        case json_error_t::number_too_large: return error::number_too_large;
        case json_error_t::input_error: return error::input_error;
        case json_error_t::exception: return error::exception;
        case json_error_t::out_of_range: return error::out_of_range;
        case json_error_t::test_failure: return error::test_failure;
        case json_error_t::missing_slash: return error::missing_slash;
        case json_error_t::invalid_escape: return error::invalid_escape;
        case json_error_t::token_not_number: return error::token_not_number;
        case json_error_t::value_is_scalar: return error::value_is_scalar;
        case json_error_t::not_found: return error::not_found;
        case json_error_t::token_overflow: return error::token_overflow;
        case json_error_t::past_the_end: return error::past_the_end;
        case json_error_t::not_number: return error::not_number;
        case json_error_t::not_exact: return error::not_exact;
        case json_error_t::not_null: return error::not_null;
        case json_error_t::not_bool: return error::not_bool;
        case json_error_t::not_array: return error::not_array;
        case json_error_t::not_object: return error::not_object;
        case json_error_t::not_string: return error::not_string;
        case json_error_t::not_int64: return error::not_int64;
        case json_error_t::not_uint64: return error::not_uint64;
        case json_error_t::not_double: return error::not_double;
        case json_error_t::not_integer: return error::not_integer;
        case json_error_t::size_mismatch: return error::size_mismatch;
        case json_error_t::exhausted_variants: return error::exhausted_variants;
        case json_error_t::unknown_name: return error::unknown_name;

        // TODO: return json category generic error.
        default: return error::unknown;
    }
}

} // namespace error
} // namespace network
} // namespace libbitcoin

// Just for reference.
#ifdef BOOST_CODES_AND_CONDITIONS

// Boost: asio error enum (basic subset, platform specific codes).
enum boost::asio::error::basic_errors
{
  /// Permission denied.
  access_denied = BOOST_ASIO_SOCKET_ERROR(EACCES),

  /// Address family not supported by protocol.
  address_family_not_supported = BOOST_ASIO_SOCKET_ERROR(EAFNOSUPPORT),

  /// Address already in use.
  address_in_use = BOOST_ASIO_SOCKET_ERROR(EADDRINUSE),

  /// Transport endpoint is already connected.
  already_connected = BOOST_ASIO_SOCKET_ERROR(EISCONN),

  /// Operation already in progress.
  already_started = BOOST_ASIO_SOCKET_ERROR(EALREADY),

  /// Broken pipe.
  broken_pipe = BOOST_ASIO_WIN_OR_POSIX(
      BOOST_ASIO_NATIVE_ERROR(ERROR_BROKEN_PIPE),
      BOOST_ASIO_NATIVE_ERROR(EPIPE)),

  /// A connection has been aborted.
  connection_aborted = BOOST_ASIO_SOCKET_ERROR(ECONNABORTED),

  /// Connection refused.
  connection_refused = BOOST_ASIO_SOCKET_ERROR(ECONNREFUSED),

  /// Connection reset by peer.
  connection_reset = BOOST_ASIO_SOCKET_ERROR(ECONNRESET),

  /// Bad file descriptor.
  bad_descriptor = BOOST_ASIO_SOCKET_ERROR(EBADF),

  /// Bad address.
  fault = BOOST_ASIO_SOCKET_ERROR(EFAULT),

  /// No route to host.
  host_unreachable = BOOST_ASIO_SOCKET_ERROR(EHOSTUNREACH),

  /// Operation now in progress.
  in_progress = BOOST_ASIO_SOCKET_ERROR(EINPROGRESS),

  /// Interrupted system call.
  interrupted = BOOST_ASIO_SOCKET_ERROR(EINTR),

  /// Invalid argument.
  invalid_argument = BOOST_ASIO_SOCKET_ERROR(EINVAL),

  /// Message too long.
  message_size = BOOST_ASIO_SOCKET_ERROR(EMSGSIZE),

  /// The name was too long.
  name_too_long = BOOST_ASIO_SOCKET_ERROR(ENAMETOOLONG),

  /// Network is down.
  network_down = BOOST_ASIO_SOCKET_ERROR(ENETDOWN),

  /// Network dropped connection on reset.
  network_reset = BOOST_ASIO_SOCKET_ERROR(ENETRESET),

  /// Network is unreachable.
  network_unreachable = BOOST_ASIO_SOCKET_ERROR(ENETUNREACH),

  /// Too many open files.
  no_descriptors = BOOST_ASIO_SOCKET_ERROR(EMFILE),

  /// No buffer space available.
  no_buffer_space = BOOST_ASIO_SOCKET_ERROR(ENOBUFS),

  /// Cannot allocate memory.
  no_memory = BOOST_ASIO_WIN_OR_POSIX(
      BOOST_ASIO_NATIVE_ERROR(ERROR_OUTOFMEMORY),
      BOOST_ASIO_NATIVE_ERROR(ENOMEM)),

  /// Operation not permitted.
  no_permission = BOOST_ASIO_WIN_OR_POSIX(
      BOOST_ASIO_NATIVE_ERROR(ERROR_ACCESS_DENIED),
      BOOST_ASIO_NATIVE_ERROR(EPERM)),

  /// Protocol not available.
  no_protocol_option = BOOST_ASIO_SOCKET_ERROR(ENOPROTOOPT),

  /// No such device.
  no_such_device = BOOST_ASIO_WIN_OR_POSIX(
      BOOST_ASIO_NATIVE_ERROR(ERROR_BAD_UNIT),
      BOOST_ASIO_NATIVE_ERROR(ENODEV)),

  /// Transport endpoint is not connected.
  not_connected = BOOST_ASIO_SOCKET_ERROR(ENOTCONN),

  /// Socket operation on non-socket.
  not_socket = BOOST_ASIO_SOCKET_ERROR(ENOTSOCK),

  /// Operation canceled.
  operation_aborted = BOOST_ASIO_WIN_OR_POSIX(
      BOOST_ASIO_NATIVE_ERROR(ERROR_OPERATION_ABORTED),
      BOOST_ASIO_NATIVE_ERROR(ECANCELED)),

  /// Operation not supported.
  operation_not_supported = BOOST_ASIO_SOCKET_ERROR(EOPNOTSUPP),

  /// Cannot send after transport endpoint shutdown.
  shut_down = BOOST_ASIO_SOCKET_ERROR(ESHUTDOWN),

  /// Connection timed out.
  timed_out = BOOST_ASIO_SOCKET_ERROR(ETIMEDOUT),

  /// Resource temporarily unavailable.
  try_again = BOOST_ASIO_WIN_OR_POSIX(
      BOOST_ASIO_NATIVE_ERROR(ERROR_RETRY),
      BOOST_ASIO_NATIVE_ERROR(EAGAIN)),

  /// Socket is marked non-blocking and requested operation would block.
  would_block = BOOST_ASIO_SOCKET_ERROR(EWOULDBLOCK)
};

// Boost: system error enum for error_conditions (platform-independent codes).
enum boost::system::errc::errc_t
{
    success = 0,
    address_family_not_supported = EAFNOSUPPORT,
    address_in_use = EADDRINUSE,
    address_not_available = EADDRNOTAVAIL,
    already_connected = EISCONN,
    argument_list_too_long = E2BIG,
    argument_out_of_domain = EDOM,
    bad_address = EFAULT,
    bad_file_descriptor = EBADF,
    bad_message = EBADMSG,
    broken_pipe = EPIPE,
    connection_aborted = ECONNABORTED,
    connection_already_in_progress = EALREADY,
    connection_refused = ECONNREFUSED,
    connection_reset = ECONNRESET,
    cross_device_link = EXDEV,
    destination_address_required = EDESTADDRREQ,
    device_or_resource_busy = EBUSY,
    directory_not_empty = ENOTEMPTY,
    executable_format_error = ENOEXEC,
    file_exists = EEXIST,
    file_too_large = EFBIG,
    filename_too_long = ENAMETOOLONG,
    function_not_supported = ENOSYS,
    host_unreachable = EHOSTUNREACH,
    identifier_removed = EIDRM,
    illegal_byte_sequence = EILSEQ,
    inappropriate_io_control_operation = ENOTTY,
    interrupted = EINTR,
    invalid_argument = EINVAL,
    invalid_seek = ESPIPE,
    io_error = EIO,
    is_a_directory = EISDIR,
    message_size = EMSGSIZE,
    network_down = ENETDOWN,
    network_reset = ENETRESET,
    network_unreachable = ENETUNREACH,
    no_buffer_space = ENOBUFS,
    no_child_process = ECHILD,
    no_link = ENOLINK,
    no_lock_available = ENOLCK,
    no_message_available = ENODATA,
    no_message = ENOMSG,
    no_protocol_option = ENOPROTOOPT,
    no_space_on_device = ENOSPC,
    no_stream_resources = ENOSR,
    no_such_device_or_address = ENXIO,
    no_such_device = ENODEV,
    no_such_file_or_directory = ENOENT,
    no_such_process = ESRCH,
    not_a_directory = ENOTDIR,
    not_a_socket = ENOTSOCK,
    not_a_stream = ENOSTR,
    not_connected = ENOTCONN,
    not_enough_memory = ENOMEM,
    not_supported = ENOTSUP,
    operation_canceled = ECANCELED,
    operation_in_progress = EINPROGRESS,
    operation_not_permitted = EPERM,
    operation_not_supported = EOPNOTSUPP,
    operation_would_block = EWOULDBLOCK,
    owner_dead = EOWNERDEAD,
    permission_denied = EACCES,
    protocol_error = EPROTO,
    protocol_not_supported = EPROTONOSUPPORT,
    read_only_file_system = EROFS,
    resource_deadlock_would_occur = EDEADLK,
    resource_unavailable_try_again = EAGAIN,
    result_out_of_range = ERANGE,
    state_not_recoverable = ENOTRECOVERABLE,
    stream_timeout = ETIME,
    text_file_busy = ETXTBSY,
    timed_out = ETIMEDOUT,
    too_many_files_open_in_system = ENFILE,
    too_many_files_open = EMFILE,
    too_many_links = EMLINK,
    too_many_symbolic_link_levels = ELOOP,
    value_too_large = EOVERFLOW,
    wrong_protocol_type = EPROTOTYPE
};

enum netdb_errors
{
  /// Host not found (authoritative).
  host_not_found = BOOST_ASIO_NETDB_ERROR(HOST_NOT_FOUND),

  /// Host not found (non-authoritative).
  host_not_found_try_again = BOOST_ASIO_NETDB_ERROR(TRY_AGAIN),

  /// The query is valid but does not have associated address data.
  no_data = BOOST_ASIO_NETDB_ERROR(NO_DATA),

  /// A non-recoverable error occurred.
  no_recovery = BOOST_ASIO_NETDB_ERROR(NO_RECOVERY)
};

enum addrinfo_errors
{
  /// The service is not supported for the given socket type.
  service_not_found = BOOST_ASIO_WIN_OR_POSIX(
      BOOST_ASIO_NATIVE_ERROR(WSATYPE_NOT_FOUND),
      BOOST_ASIO_GETADDRINFO_ERROR(EAI_SERVICE)),

  /// The socket type is not supported.
  socket_type_not_supported = BOOST_ASIO_WIN_OR_POSIX(
      BOOST_ASIO_NATIVE_ERROR(WSAESOCKTNOSUPPORT),
      BOOST_ASIO_GETADDRINFO_ERROR(EAI_SOCKTYPE))
};

enum misc_errors
{
  /// Already open.
  already_open = 1,

  /// End of file or stream.
  eof,

  /// Element not found.
  not_found,

  /// The descriptor cannot fit into the select system call's fd_set.
  fd_set_failure
};

#endif // BOOST_CODES_AND_CONDITIONS