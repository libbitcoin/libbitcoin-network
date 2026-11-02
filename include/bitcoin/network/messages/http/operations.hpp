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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_OPERATIONS_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_OPERATIONS_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/flat_buffer.hpp>
#include <bitcoin/network/messages/http/message.hpp>
#include <bitcoin/network/messages/http/parser.hpp>
#include <bitcoin/network/messages/http/serializer.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// The most bytes read from the stream by one call.
constexpr size_t transfer_size = system::power2<size_t>(16u);

/// Read one message from the stream (blocking read_some), using the buffer
/// for received bytes. Bytes beyond the message are retained in the buffer.
template <typename Stream, bool IsRequest, class Body>
void read(Stream& stream, flat_buffer& buffer,
    message<IsRequest, Body>& out, code& ec) NOEXCEPT;

/// Write the message to the stream (blocking write_some).
template <typename Stream, bool IsRequest, class Body>
void write(Stream& stream, message<IsRequest, Body>& in, code& ec) NOEXCEPT;
template <typename Stream, bool IsRequest, class Body>
void write(Stream& stream, message<IsRequest, Body>&& in, code& ec) NOEXCEPT;

} // namespace http
} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/messages/http/operations.ipp>

#endif
