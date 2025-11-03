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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_FIELDS_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_FIELDS_HPP

#include <algorithm>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// http header fields: OWS is SP and HTAB (less than ascii).
const system::string_list http_whitespace{ " ", "\t" };

/// Does the request have an attachment.
BCT_API bool has_attachment(const fields& header) NOEXCEPT;

/// Does the header include the required websocket upgrade request values.
BCT_API bool is_websocket_upgrade(const http::fields& header) NOEXCEPT;

/// Generate the required sec_websocket_key response value.
BCT_API std::string to_websocket_accept(const http::fields& header) NOEXCEPT;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
