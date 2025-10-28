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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_FIELD_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_FIELD_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/enums/verb.hpp>

/// Type-differentiation for request message distribution.

namespace libbitcoin {
namespace network {
namespace http {

inline bool has_attachment(http::fields& head) NOEXCEPT
{
    const auto& disposition = head[field::content_disposition];
    const auto content = system::ascii_to_lower(disposition);
    return content.find("filename=") != std::string::npos;
}

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
