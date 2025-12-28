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
////#include <bitcoin/network/error.hpp>

#include <bitcoin/network/async/async.hpp>
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
    { bad_method, "bad method" },
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

bool asio_is_canceled(const boost_code& ec) NOEXCEPT
{
    // self termination
    return ec == boost_error_t::operation_canceled
        || ec == boost::asio::error::operation_aborted;
}

// The success and operation_canceled codes are the only expected in normal
// operation, so these are first, to optimize the case where asio_is_canceled
// is not used.
code asio_to_error_code(const boost_code& ec) NOEXCEPT
{
    if (ec == boost_error_t::success)
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
    return error::unknown;
}

code http_to_error_code(const boost_code& ec) NOEXCEPT
{
    namespace http = boost::beast::http;

    if (!ec)
        return {};

    if (ec == http::error::end_of_stream)
        return error::end_of_stream;
    if (ec == http::error::partial_message)
        return error::partial_message;
    if (ec == http::error::need_more)
        return error::need_more;
    if (ec == http::error::unexpected_body)
        return error::unexpected_body;
    if (ec == http::error::need_buffer)
        return error::need_buffer;
    if (ec == http::error::end_of_chunk)
        return error::end_of_chunk;
    if (ec == http::error::buffer_overflow)
        return error::buffer_overflow;
    if (ec == http::error::header_limit)
        return error::header_limit;
    if (ec == http::error::body_limit)
        return error::body_limit;
    if (ec == http::error::bad_alloc)
        return error::bad_alloc;
    if (ec == http::error::bad_line_ending)
        return error::bad_line_ending;
    if (ec == http::error::bad_method)
        return error::bad_method;
    if (ec == http::error::bad_target)
        return error::bad_target;
    if (ec == http::error::bad_version)
        return error::bad_version;
    if (ec == http::error::bad_status)
        return error::bad_status;
    if (ec == http::error::bad_reason)
        return error::bad_reason;
    if (ec == http::error::bad_field)
        return error::bad_field;
    if (ec == http::error::bad_value)
        return error::bad_value;
    if (ec == http::error::bad_content_length)
        return error::bad_content_length;
    if (ec == http::error::bad_transfer_encoding)
        return error::bad_transfer_encoding;
    if (ec == http::error::bad_chunk)
        return error::bad_chunk;
    if (ec == http::error::bad_chunk_extension)
        return error::bad_chunk_extension;
    if (ec == http::error::bad_obs_fold)
        return error::bad_obs_fold;
    if (ec == http::error::multiple_content_length)
        return error::multiple_content_length;
    if (ec == http::error::stale_parser)
        return error::stale_parser;
    if (ec == http::error::short_read)
        return error::short_read;

    return asio_to_error_code(ec);
}

code ws_to_error_code(const boost_code& ec) NOEXCEPT
{
    namespace ws = boost::beast::websocket;

    if (!ec)
        return {};

    if (ec == ws::error::closed)
        return error::websocket_closed;
    if (ec == ws::error::buffer_overflow)
        return error::websocket_buffer_overflow;
    if (ec == ws::error::partial_deflate_block)
        return error::partial_deflate_block;
    if (ec == ws::error::message_too_big)
        return error::message_too_big;
    if (ec == ws::error::bad_http_version)
        return error::bad_http_version;
    if (ec == ws::error::bad_method)
        return error::websocket_bad_method;
    if (ec == ws::error::no_host)
        return error::no_host;
    if (ec == ws::error::no_connection)
        return error::no_connection;
    if (ec == ws::error::no_connection_upgrade)
        return error::no_connection_upgrade;
    if (ec == ws::error::no_upgrade)
        return error::no_upgrade;
    if (ec == ws::error::no_upgrade_websocket)
        return error::no_upgrade_websocket;
    if (ec == ws::error::no_sec_key)
        return error::no_sec_key;
    if (ec == ws::error::bad_sec_key)
        return error::bad_sec_key;
    if (ec == ws::error::no_sec_version)
        return error::no_sec_version;
    if (ec == ws::error::bad_sec_version)
        return error::bad_sec_version;
    if (ec == ws::error::no_sec_accept)
        return error::no_sec_accept;
    if (ec == ws::error::bad_sec_accept)
        return error::bad_sec_accept;
    if (ec == ws::error::upgrade_declined)
        return error::upgrade_declined;
    if (ec == ws::error::bad_opcode)
        return error::bad_opcode;
    if (ec == ws::error::bad_data_frame)
        return error::bad_data_frame;
    if (ec == ws::error::bad_continuation)
        return error::bad_continuation;
    if (ec == ws::error::bad_reserved_bits)
        return error::bad_reserved_bits;
    if (ec == ws::error::bad_control_fragment)
        return error::bad_control_fragment;
    if (ec == ws::error::bad_control_size)
        return error::bad_control_size;
    if (ec == ws::error::bad_unmasked_frame)
        return error::bad_unmasked_frame;
    if (ec == ws::error::bad_masked_frame)
        return error::bad_masked_frame;
    if (ec == ws::error::bad_size)
        return error::bad_size;
    if (ec == ws::error::bad_frame_payload)
        return error::bad_frame_payload;
    if (ec == ws::error::bad_close_code)
        return error::bad_close_code;
    if (ec == ws::error::bad_close_size)
        return error::bad_close_size;
    if (ec == ws::error::bad_close_payload)
        return error::bad_close_payload;

    return http_to_error_code(ec);
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