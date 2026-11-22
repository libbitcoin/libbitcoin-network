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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_PARSER_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_PARSER_HPP

#include <optional>
#include <bitcoin/network/io/io.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/codings.hpp>
#include <bitcoin/network/messages/http/fields.hpp>
#include <bitcoin/network/messages/http/header.hpp>
#include <bitcoin/network/messages/http/message.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// The body framing determined by header fields.
struct framing
{
    /// The body is chunked.
    bool chunked{};

    /// The body has a transfer coding, with chunked not the final coding.
    bool encoded{};

    /// The content-length, if the body is so delimited.
    length_type length{};
};

/// Each parse consumes element from front of its text (rfc9112), returning size
/// consumed, or zero with ec set: need_more if text is incomplete but so far
/// valid element, otherwise error.

/// Parse a request line (including its line end).
BCT_API size_t parse_start_line(request_header& out,
    const std::string_view& in, code& ec) NOEXCEPT;

/// Parse a status line (including its line end).
BCT_API size_t parse_start_line(response_header& out,
    const std::string_view& in, code& ec) NOEXCEPT;

/// Parse field line (including its line end and any folded
/// lines), adding it to out (if not null), or empty line
/// that ends fields (end). Framing fields update frame.
BCT_API size_t parse_field(fields* out, framing& frame, bool& end,
    const std::string_view& in, code& ec) NOEXCEPT;

/// Parse the chunk size digits of a chunk line.
BCT_API size_t parse_chunk_size(uint64_t& size, const std::string_view& line,
    code& ec) NOEXCEPT;

/// The chunk extensions of the line, from index, are valid.
BCT_API bool valid_chunk_extensions(const std::string_view& line,
    size_t index) NOEXCEPT;

/// Find the end of a chunk line, setting text to its line end (CR).
BCT_API size_t find_chunk_line(size_t& text, const std::string_view& in,
    code& ec) NOEXCEPT;

/// Not thread safe, non-virtual.
/// Incrementally parse message (owned) from received bytes, driving body
/// reader of message body. Start line, each field, each chunk line and
/// each trailer field are consumed once complete, and body data as it
/// arrives. Parse returns once header is complete, and body reader may
/// pause parse (need_buffer), which resumes on next put. Trailer fields
/// validated but not retained. Body with content or transfer codings is
/// decoded as it arrives, in steps that bound decoded data held for
/// paused reader, body limit applying to both coded and decoded body.
template <bool IsRequest, class Body>
class parser
{
public:
    using message_type = message<IsRequest, Body>;
    using reader_type = typename Body::reader;

    DELETE_COPY_MOVE(parser);

    /// The body value may be preselected on the message before parsing.
    parser() NOEXCEPT;
    parser(message_type&& message) NOEXCEPT;

    /// Limits of zero are unlimited (the default).
    void header_limit(size_t value) NOEXCEPT;
    void body_limit(size_t value) NOEXCEPT;

    /// Consume received bytes, returning number consumed. Sets need_more if not
    /// done and remaining bytes insufficient to proceed.
    size_t put(const io::const_buffer& buffer, code& ec) NOEXCEPT;

    /// The stream ended, which completes a body delimited by it.
    void put_eof(code& ec) NOEXCEPT;

    /// The header has been parsed.
    bool is_header_done() const NOEXCEPT;

    /// The message has been parsed.
    bool is_done() const NOEXCEPT;

    /// The response has no body (to a HEAD request).
    void skip(bool value) NOEXCEPT;

    /// The parsed message.
    message_type& get() NOEXCEPT;

    /// Move out the parsed message.
    message_type release() NOEXCEPT;

private:
    enum class state
    {
        start_line,
        fields,
        body,
        body_to_eof,
        chunk_header,
        chunk_body,
        trailer,
        decoded,
        done
    };

    size_t put_start_line(const std::string_view& data,
        code& ec) NOEXCEPT;
    size_t put_fields(const std::string_view& data, code& ec) NOEXCEPT;
    size_t put_body(const std::string_view& data, code& ec) NOEXCEPT;
    size_t put_chunk_header(const std::string_view& data,
        code& ec) NOEXCEPT;
    size_t put_trailer(const std::string_view& data, code& ec) NOEXCEPT;
    size_t put_decoded(code& ec) NOEXCEPT;
    size_t put_coded(const std::string_view& data, code& ec) NOEXCEPT;
    void init_reader(const length_type& length, code& ec) NOEXCEPT;
    bool deliver(code& ec) NOEXCEPT;
    code coding_fault() const NOEXCEPT;
    void finish_header(code& ec) NOEXCEPT;
    void complete_body(code& ec) NOEXCEPT;
    void finish(code& ec) NOEXCEPT;
    std::string_view header_window(const std::string_view& data) const NOEXCEPT;
    void header_limited(const std::string_view& data,
        code& ec) const NOEXCEPT;

    message_type message_;
    reader_type reader_;
    codings codings_{};
    std::optional<decoder> decoder_{};
    system::data_chunk decoded_{};
    size_t delivered_{};
    size_t header_limit_{};
    size_t body_limit_{};
    framing frame_{};
    state state_{ state::start_line };
    size_t header_{};
    uint64_t remaining_{};
    uint64_t total_{};
    bool expect_crlf_{};
    bool skip_{};
    bool received_{};
};

/// Not thread safe, non-virtual.
/// Incrementally parse headerless message body (referenced) from received
/// bytes, driving body reader of message body. Streamed body is done once its
/// reader finishes, otherwise received bytes are whole body.
template <bool IsRequest, class Body>
class body_parser
{
public:
    using message_type = message<IsRequest, Body>;
    using reader_type = typename Body::reader;

    DELETE_COPY_MOVE(body_parser);

    /// The body value may be preselected on the message before parsing.
    body_parser(message_type& message, bool streamed) NOEXCEPT;

    /// A limit of zero is unlimited (the default).
    void body_limit(size_t value) NOEXCEPT;

    /// Consume received bytes, returning number consumed. Sets need_more if not
    /// done and remaining bytes insufficient to proceed.
    size_t put(const io::const_buffer& buffer, code& ec) NOEXCEPT;

    /// The stream ended, which does not complete a headerless body.
    void put_eof(code& ec) NOEXCEPT;

    /// There is no header.
    bool is_header_done() const NOEXCEPT;

    /// The body has been parsed.
    bool is_done() const NOEXCEPT;

private:
    reader_type reader_;
    size_t body_limit_{};
    size_t total_{};
    bool initialized_{};
    bool done_{};
};

} // namespace http
} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/messages/http/parser.ipp>

#endif
