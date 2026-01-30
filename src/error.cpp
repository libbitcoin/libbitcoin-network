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

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

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

    // boost
    { system_unknown, "system error" },
    { misc_unknown, "misc error" },
    { errc_unknown, "errc error" },
    { ssl_unknown, "ssl error" },

    // boost tls
    { tls_set_options, "failed to set tls options" },
    { tls_use_certificate, "failed to set tls certificate" },
    { tls_use_private_key, "failed to set tls private key" },
    { tls_set_password, "failed to set tls private key password" },
    { tls_set_default_verify, "failed to set tls default certificate authority" },
    { tls_set_add_verify, "failed to set tls certificate authority" },
    { tls_stream_truncated, "tls stream truncated" },
    { tls_unspecified_system_error, "tls unspecified system error" },
    { tls_unexpected_result, "tls unexpected result" },

    // boost beast http 4xx client error
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

    // boost beast http 5xx server error
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

    // boost beast http error
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
    { http_unknown, "http error" },

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
    { websocket_unknown, "websocket error" },

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
    { json_unknown, "json error" },

    // query string parse error
    { message_overflow, "message overflow" },
    { undefined_type, "undefined type" },
    { unexpected_method, "unexpected method" },
    { unexpected_type, "unexpected type" },
    { extra_positional, "extra positional" },
    { extra_named, "extra named" },
    { missing_array, "missing array" },
    { missing_object, "missing object" },
    { missing_parameter, "missing parameter" },

    // json-rpc error
    { jsonrpc_requires_method, "jsonrpc requires method" },
    { jsonrpc_v1_requires_params, "jsonrpc v1 requires params" },
    { jsonrpc_v1_requires_array_params, "jsonrpc v1 requires array params" },
    { jsonrpc_v1_requires_id, "jsonrpc v1 requires id" },
    { jsonrpc_reader_bad_buffer, "jsonrpc reader bad buffer" },
    { jsonrpc_reader_stall, "jsonrpc reader stall" },
    { jsonrpc_reader_exception, "jsonrpc reader exception" },
    { jsonrpc_writer_exception, "jsonrpc writer exception" }
};

DEFINE_ERROR_T_CATEGORY(error, "network", "network code")

bool asio_is_canceled(const boost_code& ec) NOEXCEPT
{
    return ec == asio_basic_error_t::operation_aborted
        || ec == boost::system::errc::errc_t::operation_canceled;
}

// includes asio_is_canceled
code asio_to_error_code(const boost_code& ec) NOEXCEPT
{
    if (!ec)
        return error::success;

    if (asio_is_canceled(ec))
        return error::operation_canceled;

    // boost::system::system_category() is aliased for netdb and addrinfo.
    if (ec.category() == boost::asio::error::get_system_category())
    {
        switch (static_cast<asio_basic_error_t>(ec.value()))
        {
            case asio_basic_error_t::connection_aborted:
                return error::operation_canceled;

            case asio_basic_error_t::operation_aborted:
                return error::operation_failed;

            case asio_basic_error_t::address_family_not_supported:
                return error::resolve_failed;

            case asio_basic_error_t::address_in_use:
            case asio_basic_error_t::already_connected:
            case asio_basic_error_t::already_started:
            case asio_basic_error_t::in_progress:
                return error::address_in_use;

            case asio_basic_error_t::shut_down:
            case asio_basic_error_t::would_block:
            case asio_basic_error_t::broken_pipe:
            case asio_basic_error_t::connection_refused:
            case asio_basic_error_t::host_unreachable:
            case asio_basic_error_t::network_down:
            case asio_basic_error_t::network_reset:
            case asio_basic_error_t::network_unreachable:
            case asio_basic_error_t::no_protocol_option:
            case asio_basic_error_t::not_connected:
            case asio_basic_error_t::not_socket:
                return error::connect_failed;

            case asio_basic_error_t::connection_reset:
                return error::peer_disconnect;

            case asio_basic_error_t::bad_descriptor:
            case asio_basic_error_t::message_size:
                return error::bad_stream;

            case asio_basic_error_t::no_memory:
            case asio_basic_error_t::fault:
            case asio_basic_error_t::interrupted:
            case asio_basic_error_t::invalid_argument:
            case asio_basic_error_t::name_too_long:
            case asio_basic_error_t::no_descriptors:
            case asio_basic_error_t::no_buffer_space:
                return error::invalid_configuration;

            case asio_basic_error_t::try_again:
            case asio_basic_error_t::no_such_device:
                return error::file_system;

            case asio_basic_error_t::access_denied:
            case asio_basic_error_t::no_permission:
            case asio_basic_error_t::operation_not_supported:
                return error::not_allowed;

            case asio_basic_error_t::timed_out:
                return error::channel_timeout;
        }

        switch (static_cast<asio_netdb_error_t>(ec.value()))
        {
            // connect-resolve
            case asio_netdb_error_t::host_not_found:
            case asio_netdb_error_t::host_not_found_try_again:
                return error::resolve_failed;

            case asio_netdb_error_t::no_data:
            case asio_netdb_error_t::no_recovery:
                return error::operation_failed;
        }

        switch (static_cast<asio_addrinfo_error_t>(ec.value()))
        {
            // connect-resolve
            case asio_addrinfo_error_t::service_not_found:
            case asio_addrinfo_error_t::socket_type_not_supported:
                return error::resolve_failed;
        }
    }

    // Independent category.
    if (ec.category() == boost::asio::error::get_misc_category())
    {
        switch (static_cast<asio_misc_error_t>(ec.value()))
        {
            // peer termination
            // stackoverflow.com/a/19891985/1172329
            case asio_misc_error_t::eof:
                return error::peer_disconnect;
            case asio_misc_error_t::already_open:
                return error::connect_failed;
            case asio_misc_error_t::not_found:
                return error::resolve_failed;
            case asio_misc_error_t::fd_set_failure:
                return error::file_system;
        }
    }

    return error::system_unknown;
}

// includes json codes
code http_to_error_code(const boost_code& ec) NOEXCEPT
{
    static const auto& category = to_http_code(
        boost::beast::http::error::end_of_stream).category();

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
        default: return error::http_unknown;
    }
}

// includes asio codes
code ssl_to_error_code(const boost_code& ec) NOEXCEPT
{
    if (!ec)
        return error::success;

    namespace stream = boost::asio::ssl::error;
    if (ec.category() == stream::get_stream_category())
    {
        switch (static_cast<asio_ssl_stream_error_t>(ec.value()))
        {
            case asio_ssl_stream_error_t::stream_truncated:
                return error::tls_stream_truncated;
            case asio_ssl_stream_error_t::unspecified_system_error:
                return error::tls_unspecified_system_error;
            case asio_ssl_stream_error_t::unexpected_result:
                return error::tls_unexpected_result;
            default: return error::tls_unknown;
        }
    }

    // Boost defines this for error numbers produced by openssl. But we get
    // generic error codes because WOLFSSL_HAVE_ERROR_QUEUE is not defined.
    // This is because otherwise in some cases we fail to get failure results
    // when necessary. These are openssl-in-boost-category codes, for
    // simplified transport.

    if (ec.category() == boost::asio::error::get_ssl_category())
    {
        // TODO: map openssl native code values to network.
        ////switch (ec.value())
        ////{
        ////    default: return error::ssl_unknown;
        ////}

        return error::ssl_unknown;
    }

    return asio_to_error_code(ec);
}


// includes json codes
code ws_to_error_code(const boost_code& ec) NOEXCEPT
{
    static const auto& category = to_websocket_code(
        boost::beast::websocket::error::closed).category();

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
        default: return error::websocket_unknown;
    }
}

// includes asio codes
code json_to_error_code(const boost_code& ec) NOEXCEPT
{
    if (!ec)
        return error::success;

    if (ec.category() != boost::json::detail::error_code_category)
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
        default: return error::json_unknown;
    }
}

// includes http codes
code rpc_to_error_code(const boost_code& ec) NOEXCEPT
{
    if (!ec)
        return error::success;

    // These are libbitcoin-category-in-boost codes, for simplified transport.
    if (ec.category() == network::error::error_category::singleton)
        return { static_cast<error_t>(ec.value()) };

    return http_to_error_code(ec);
}

// Boost cross platform codes (not asio).
code errc_to_error_code(const boost_code& ec) NOEXCEPT
{
    if (!ec)
        return error::success;

    if (ec.category() == boost::system::generic_category())
    {
        switch (static_cast<boost_errc_t>(ec.value()))
        {
            case boost_errc_t::connection_aborted:
            case boost_errc_t::operation_canceled:
                return error::operation_canceled;
                
            // peer termination
            // stackoverflow.com/a/19891985/1172329
            case boost_errc_t::connection_reset:
                return error::peer_disconnect;

            // learn.microsoft.com/en-us/troubleshoot/windows-client/networking/
            // connect-tcp-greater-than-5000-error-wsaenobufs-10055
            case boost_errc_t::no_buffer_space:
                return error::invalid_configuration;

            // network
            case boost_errc_t::operation_not_permitted:
            case boost_errc_t::operation_not_supported:
            case boost_errc_t::owner_dead:
            case boost_errc_t::permission_denied:
                return error::not_allowed;

            // connect-resolve
            case boost_errc_t::address_family_not_supported:
            case boost_errc_t::bad_address:
            case boost_errc_t::destination_address_required:
                return error::resolve_failed;

            // connect-connect
            case boost_errc_t::address_not_available:
            case boost_errc_t::not_connected:
            case boost_errc_t::connection_refused:
            case boost_errc_t::broken_pipe:
            case boost_errc_t::host_unreachable:
            case boost_errc_t::network_down:
            case boost_errc_t::network_reset:
            case boost_errc_t::network_unreachable:
            case boost_errc_t::no_link:
            case boost_errc_t::no_protocol_option:
            case boost_errc_t::no_such_file_or_directory:
            case boost_errc_t::not_a_socket:
            case boost_errc_t::protocol_not_supported:
            case boost_errc_t::wrong_protocol_type:
                return error::connect_failed;

            // connect-address
            case boost_errc_t::address_in_use:
            case boost_errc_t::already_connected:
            case boost_errc_t::connection_already_in_progress:
            case boost_errc_t::operation_in_progress:
                return error::address_in_use;

            // I/O (bad_file_descriptor if socket is not initialized)
            case boost_errc_t::bad_file_descriptor:
            case boost_errc_t::bad_message:
            case boost_errc_t::illegal_byte_sequence:
            case boost_errc_t::io_error:
            case boost_errc_t::message_size:
            case boost_errc_t::no_message_available:
            case boost_errc_t::no_message:
            case boost_errc_t::no_stream_resources:
            case boost_errc_t::not_a_stream:
            case boost_errc_t::protocol_error:
                return error::bad_stream;

            // timeout
            case boost_errc_t::stream_timeout:
            case boost_errc_t::timed_out:
                return error::channel_timeout;

            // file system errors (bad_file_descriptor used in I/O)
            case boost_errc_t::cross_device_link:
            case boost_errc_t::device_or_resource_busy:
            case boost_errc_t::directory_not_empty:
            case boost_errc_t::executable_format_error:
            case boost_errc_t::file_exists:
            case boost_errc_t::file_too_large:
            case boost_errc_t::filename_too_long:
            case boost_errc_t::invalid_seek:
            case boost_errc_t::is_a_directory:
            case boost_errc_t::no_space_on_device:
            case boost_errc_t::no_such_device:
            case boost_errc_t::no_such_device_or_address:
            case boost_errc_t::read_only_file_system:
            case boost_errc_t::resource_unavailable_try_again:
            case boost_errc_t::text_file_busy:
            case boost_errc_t::too_many_files_open:
            case boost_errc_t::too_many_files_open_in_system:
            case boost_errc_t::too_many_links:
            case boost_errc_t::too_many_symbolic_link_levels:
                return error::file_system;

            default:
                return error::errc_unknown;
        }
    }

    return error::unknown;
}

BC_POP_WARNING()

} // namespace error
} // namespace network
} // namespace libbitcoin
