/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_ERROR_HPP
#define LIBBITCOIN_NETWORK_ERROR_HPP

#include <system_error>
#include <bitcoin/system.hpp>
#include <bitcoin/network/boost.hpp>

namespace libbitcoin {
namespace network {

/// Alias system code.
/// std::error_code "network" category holds network::error::error_t.
typedef std::error_code code;

namespace error {

/// Alias asio code.
/// Asio implements an equivalent to std::error_code.
/// boost::system::error_code "system" category holds boost::asio::basic_errors.
typedef boost::system::error_code boost_code;

/// Alias asio code error enumeration.
/// boost::system::error_code "system" category holds boost::asio::basic_errors.
/// These are platform-specific error codes, for which we have seen variance.
/// boost::system::errc::errc_t is the boost::system error condition enum.
/// By comparing against conditions we obtain platform-independent error codes.
typedef boost::system::errc::errc_t boost_error_t;
typedef boost::asio::error::misc_errors asio_misc_error_t;
typedef boost::asio::error::netdb_errors asio_netdb_error_t;
typedef boost::asio::error::basic_errors asio_system_error_t;

/// Asio failures are normalized to the error codes below.
/// Stop by explicit call is mapped to channel_stopped or service_stopped
/// depending on the context. Asio errors returned on cancel calls are ignored.
enum error_t : uint8_t
{
    success,
    unknown,

    // addresses
    address_invalid,
    address_not_found,
    address_disabled,
    address_unsupported,
    address_insufficient,
    seeding_unsuccessful,

    // file system
    file_load,
    file_save,
    file_system,
    file_exception,

    // general I/O failures
    bad_stream,
    not_allowed,
    peer_disconnect,
    peer_unsupported,
    peer_insufficient,
    protocol_violation,
    channel_overflow,
    channel_underflow,

    // incoming connection failures
    listen_failed,
    accept_failed,
    oversubscribed,

    // incoming/outgoing connection failures
    address_blocked,

    // outgoing connection failures
    address_in_use,
    resolve_failed,
    connect_failed,

    // heading read failures
    invalid_heading,
    invalid_magic,

    // payload read failures
    oversized_payload,
    invalid_checksum,
    invalid_message,
    unknown_message,

    // general failures
    invalid_configuration,
    operation_timeout,
    operation_canceled,
    operation_failed,

    // termination
    channel_timeout,
    channel_conflict,
    channel_dropped,
    channel_expired,
    channel_inactive,
    channel_stopped,
    service_stopped,
    service_suspended,
    subscriber_exists,
    subscriber_stopped,
    desubscribed
};

// No current need for error_code equivalence mapping.
DECLARE_ERROR_T_CODE_CATEGORY(error);

/// Unfortunately std::error_code and boost::system::error_code are distinct
/// types, so they do not compare as would be expected across distinct
/// categories of one or the other type. One solution is to rely exclusively on
/// boost::system::error_code, but we prefer to normalize the public interface
/// on std::error_code, and eventually phase out boost codes. Also we prefer to
/// propagate only errors in library categories. To bridge the gap we map other
/// codes to network::error_t codes. This cannot be done using the equivalence
/// operator overloads of either, since the error_code types are distinct,
/// despite being effectively identical. So we provide this explicit mapping.
BCT_API code asio_to_error_code(const error::boost_code& ec) NOEXCEPT;

/// Shortcircuit common code mapping.
BCT_API bool asio_is_canceled(const error::boost_code& ec) NOEXCEPT;

} // namespace error
} // namespace network
} // namespace libbitcoin

DECLARE_STD_ERROR_REGISTRATION(bc::network::error::error)

#endif
