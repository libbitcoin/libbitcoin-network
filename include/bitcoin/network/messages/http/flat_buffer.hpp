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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_FLAT_BUFFER_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_FLAT_BUFFER_HPP

#include <bitcoin/network/io/io.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// Not thread safe, non-virtual.
/// A contiguous dynamic buffer: readable bytes (data) followed by writable
/// bytes (prepare), with a limit on the readable size.
class BCT_API flat_buffer
{
public:
    using const_buffers_type = io::const_buffer;
    using mutable_buffers_type = io::mutable_buffer;

    DEFAULT_COPY_MOVE_DESTRUCT(flat_buffer);

    /// Unlimited.
    flat_buffer() NOEXCEPT;

    /// Readable size limited to limit.
    explicit flat_buffer(size_t limit) NOEXCEPT;

    /// The readable bytes.
    const_buffers_type data() const NOEXCEPT;
    const_buffers_type cdata() const NOEXCEPT;
    mutable_buffers_type data() NOEXCEPT;

    /// The size of the readable bytes.
    size_t size() const NOEXCEPT;

    /// The limit on the readable size.
    size_t max_size() const NOEXCEPT;

    /// The allocated size.
    size_t capacity() const NOEXCEPT;

    /// Writable bytes of the size, throws std::length_error if the readable
    /// size would then exceed the limit.
    mutable_buffers_type prepare(size_t size) THROWS;

    /// Move size writable bytes (prepared) to the readable bytes.
    void commit(size_t size) NOEXCEPT;

    /// Remove size bytes from the front of the readable bytes.
    void consume(size_t size) NOEXCEPT;

    /// Remove all bytes.
    void clear() NOEXCEPT;

    /// Allocate at least size bytes.
    void reserve(size_t size) NOEXCEPT;

    /// Release allocation beyond the readable bytes.
    void shrink_to_fit() NOEXCEPT;

private:
    system::data_chunk buffer_{};
    size_t begin_{};
    size_t end_{};
    size_t limit_;
};

typedef std::shared_ptr<flat_buffer> flat_buffer_ptr;
typedef std::shared_ptr<const flat_buffer> flat_buffer_cptr;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
