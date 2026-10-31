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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_ENUMS_FIELD_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_ENUMS_FIELD_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// Header field names, those not enumerated are unknown (by name only).
enum class field : uint8_t
{
    unknown,
    accept,
    access_control_allow_headers,
    access_control_allow_methods,
    access_control_allow_origin,
    access_control_max_age,
    access_control_request_headers,
    access_control_request_method,
    authorization,
    connection,
    content_disposition,
    content_length,
    content_type,
    date,
    host,
    keep_alive,
    location,
    origin,
    sec_websocket_accept,
    sec_websocket_extensions,
    sec_websocket_key,
    sec_websocket_protocol,
    sec_websocket_version,
    server,
    transfer_encoding,
    upgrade,
    user_agent,
    vary
};

/// The field of the name (case insensitive), or unknown.
BCT_API field to_field(const std::string_view& name) NOEXCEPT;

/// The canonical name of the field, empty for unknown.
BCT_API std::string_view to_string(field name) NOEXCEPT;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
