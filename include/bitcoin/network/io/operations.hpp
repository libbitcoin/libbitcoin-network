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
#ifndef LIBBITCOIN_NETWORK_IO_OPERATIONS_HPP
#define LIBBITCOIN_NETWORK_IO_OPERATIONS_HPP

#include <functional>
#include <string>
#include <string_view>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/io/buffer.hpp>

namespace libbitcoin {
namespace network {
namespace io {

/// Composed operations over a stream (socket or upgraded stream).
/// A stream provides get_executor(), async_read_some(buffers, handler) and
/// async_write_some(buffers, handler), or read_some(buffers, ec) and
/// write_some(buffers, ec), with handlers of (const code&, size_t).

using transfer_handler = std::function<void(const code&, size_t)>;

/// Write all bytes of the buffers (completes on the stream's executor).
template <typename Stream, typename Buffers>
void async_write(Stream& stream, const Buffers& buffers,
    transfer_handler&& handler) NOEXCEPT;

/// Read until the buffers are full (completes on the stream's executor).
template <typename Stream, typename Buffers>
void async_read(Stream& stream, const Buffers& buffers,
    transfer_handler&& handler) NOEXCEPT;

/// Write all bytes of the buffers.
template <typename Stream, typename Buffers>
size_t write(Stream& stream, const Buffers& buffers, code& ec) NOEXCEPT;

/// Read until the buffers are full.
template <typename Stream, typename Buffers>
size_t read(Stream& stream, const Buffers& buffers, code& ec) NOEXCEPT;

/// Read into the text until it contains the delimiter, returning the size of
/// the text through the delimiter (bytes beyond it are retained in the text).
template <typename Stream>
size_t read_until(Stream& stream, std::string& text,
    const std::string_view& delimiter, code& ec) NOEXCEPT;

} // namespace io
} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/io/operations.ipp>

#endif
