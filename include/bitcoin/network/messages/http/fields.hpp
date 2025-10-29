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

/// Does the request have an attachment.
/// Simple test for leading "filename" assumes not other token starts with
/// "filename" unless it is also an attachment (such as "filename*"). Otherwise
/// the request is not valid anyway, so we can assume it has an attachment.
inline bool has_attachment(const http::fields& header) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const auto disposition = header[field::content_disposition];
    BC_POP_WARNING()

    const auto lower = system::ascii_to_lower(disposition);

    // http header fields: OWS is SP and HTAB (less than ascii).
    return std::ranges::any_of(system::split(lower, { ";" }, { " ", "\t" }),
        [](auto& token) NOEXCEPT
        {
            return token.starts_with("filename");
        });

    return false;
}

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
