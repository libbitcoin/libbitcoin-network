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
#include <bitcoin/network/messages/http/body.hpp>

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

using namespace system;

// Field dereference is not noexcept but does not throw.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Simple test for leading "filename" assumes not other token starts with
// "filename" unless it is also an attachment (such as "filename*"). Otherwise
// the request is not valid anyway, so we can assume it has an attachment.
bool has_attachment(const http::fields& header) NOEXCEPT
{
    const auto disposition = header[field::content_disposition];
    const auto lower = ascii_to_lower(disposition);
    return std::ranges::any_of(split(lower, { ";" }, http_whitespace),
        [](auto& token) NOEXCEPT
        {
            return token.starts_with("filename");
        });

    return false;
}

bool is_websocket_upgrade(const http::fields& header) NOEXCEPT
{
    if (header[http::field::sec_websocket_key].empty() ||
        header[http::field::upgrade] != "websocket")
        return false;

    const auto lower = ascii_to_lower(header[http::field::connection]);
    return (contains(split(lower, { "," }, http_whitespace), "upgrade"));
}

std::string to_websocket_accept(const http::fields& header) NOEXCEPT
{
    constexpr auto magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    const std::string key = header[http::field::sec_websocket_key];
    return key.empty() ? key : encode_base64(sha1_hash(key + magic));
}

BC_POP_WARNING()

} // namespace http
} // namespace network
} // namespace libbitcoin
