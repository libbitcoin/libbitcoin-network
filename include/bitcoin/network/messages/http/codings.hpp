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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_CODINGS_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_CODINGS_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/enums/coding.hpp>
#include <bitcoin/network/messages/http/fields.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// The codings applied to a body, each in order of application.
struct codings
{
    /// Content codings (rfc9110 8.4), excluding identity.
    std::vector<coding> content{};

    /// Transfer codings (rfc9112 7), excluding chunked.
    std::vector<coding> transfer{};

    /// No coding is applied.
    bool empty() const NOEXCEPT
    {
        return content.empty() && transfer.empty();
    }
};

/// The codings named by the header, ec set (bad_content_encoding or
/// bad_transfer_encoding) if any is unknown.
BCT_API codings to_codings(const fields& header, code& ec) NOEXCEPT;

/// The body encoded by the codings, content then transfer.
BCT_API system::data_chunk encode(const system::data_slice& body,
    const codings& value) NOEXCEPT;

/// The body decoded by the codings, transfer then content, each in reverse.
/// Sets ec (bad_transfer_encoding or bad_content_encoding) if a coding is
/// invalid or the decoded body would exceed limit (zero is unlimited).
BCT_API system::data_chunk decode(const system::data_slice& body,
    const codings& value, size_t limit, code& ec) NOEXCEPT;

/// The response coding preferred by the request (rfc9110 12.5.3), by quality
/// then gzip, deflate and identity, or unknown if none is acceptable.
BCT_API coding accepted_coding(const fields& request) NOEXCEPT;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
