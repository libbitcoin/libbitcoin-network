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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_ENUMS_CODING_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_ENUMS_CODING_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// Content and transfer codings (rfc9110 8.4.1), others are unknown.
enum class coding : uint8_t
{
    unknown,
    identity,
    gzip,
    deflate
};

/// The coding of the name (case insensitive, x-gzip is gzip), or unknown.
BCT_API coding to_coding(const std::string_view& name) NOEXCEPT;

/// The canonical name of the coding, empty for unknown.
BCT_API std::string_view to_string(coding value) NOEXCEPT;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
