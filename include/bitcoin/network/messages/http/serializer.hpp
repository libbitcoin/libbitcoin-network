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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_SERIALIZER_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_SERIALIZER_HPP

#include <bitcoin/network/io/io.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/header.hpp>
#include <bitcoin/network/messages/http/message.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// The start line and fields of the header, and the empty line.
BCT_API std::string serialize_header(const request_header& header) NOEXCEPT;
BCT_API std::string serialize_header(const response_header& header) NOEXCEPT;

/// The chunk size line of a chunk of the size.
BCT_API std::string chunk_header(size_t size) NOEXCEPT;

/// The line end that follows chunk data.
constexpr std::string_view chunk_end{ "\r\n" };

/// The last chunk (without trailer fields).
constexpr std::string_view last_chunk{ "0\r\n\r\n" };

/// Buffers to write in sequence.
using const_buffers = std::vector<io::const_buffer>;

/// Not thread safe, non-virtual.
/// Incrementally serialize a message, driving the body writer of the message
/// body. Each next() yields buffers valid until the following next() call.
template <bool IsRequest, class Body>
class serializer
{
public:
    using message_type = message<IsRequest, Body>;
    using writer_type = typename Body::writer;

    DELETE_COPY_MOVE(serializer);

    serializer(message_type& message) NOEXCEPT;

    /// The next buffers to write, empty once done.
    const_buffers next(code& ec) NOEXCEPT;

    /// The header has been yielded.
    bool is_header_done() const NOEXCEPT;

    /// The message has been yielded.
    bool is_done() const NOEXCEPT;

    /// The body is binary (not text).
    bool binary() const NOEXCEPT;

    /// The serialized message.
    message_type& get() NOEXCEPT;

private:
    enum class state
    {
        header,
        body,
        done
    };

    message_type& message_;
    writer_type writer_;
    const bool chunked_;
    state state_{ state::header };
    std::string header_{};
    std::string prefix_{};
};

/// Not thread safe, non-virtual.
/// Incrementally serialize a headerless message body, driving the body writer
/// of the message body. Each next() yields a buffer valid until the following
/// next() call.
template <bool IsRequest, class Body>
class body_serializer
{
public:
    using message_type = message<IsRequest, Body>;
    using writer_type = typename Body::writer;

    DELETE_COPY_MOVE(body_serializer);

    body_serializer(message_type& message) NOEXCEPT;

    /// The next buffers to write, empty once done.
    const_buffers next(code& ec) NOEXCEPT;

    /// The body has been yielded.
    bool is_done() const NOEXCEPT;

    /// The body is binary (not text).
    bool binary() const NOEXCEPT;

private:
    writer_type writer_;
    bool initialized_{};
    bool done_{};
};

} // namespace http
} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/messages/http/serializer.ipp>

#endif
