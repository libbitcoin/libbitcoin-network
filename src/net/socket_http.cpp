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
#include <bitcoin/network/net/socket.hpp>

#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {
    
using namespace system;
using namespace std::placeholders;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// DETECT (first read).
// ----------------------------------------------------------------------------

// local
constexpr bool is_json_open(char byte) NOEXCEPT
{
    // A json value opens with an object or array, and neither is a valid
    // http method token character (rfc9110 tchar).
    return byte == '{' || byte == '[';
}

// local
constexpr bool is_line_break(char byte) NOEXCEPT
{
    // A server tolerates empty lines preceding a request line (rfc9112).
    return byte == '\r' || byte == '\n';
}

// local
// False implies inconclusive (the buffer holds only empty lines).
inline bool detect_json(bool& json, const http::flat_buffer& buffer) NOEXCEPT
{
    const auto data = buffer.data();
    const std::string_view bytes{ static_cast<const char*>(data.data()),
        data.size() };

    for (const auto byte: bytes)
    {
        if (is_line_break(byte))
            continue;

        json = is_json_open(byte);
        return true;
    }

    return false;
}

void socket::detect(http::flat_buffer& buffer,
    count_handler&& handler) NOEXCEPT
{
    io::dispatch(strand_,
        std::bind(&socket::do_detect, shared_from_this(),
            std::ref(buffer), std::move(handler)));
}

// private
void socket::do_detect(const ref<http::flat_buffer>& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    bool json{};
    if (detect_json(json, buffer.get()))
    {
        detected_.store(true);
        downgraded_.store(json);
        handler(error::success, zero);
        return;
    }

    // Tolerated empty lines only, so read more (bounded by the buffer).
    async_read(buffer.get(),
        std::bind(&socket::handle_detect,
            shared_from_this(), _1, _2, buffer, handler));
}

// private
void socket::handle_detect(const code& ec, size_t size,
    const ref<http::flat_buffer>& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec)
    {
        handler(ec, size);
        return;
    }

    // async_read prepares, the caller commits (as does handle_body_read).
    buffer.get().commit(size);
    do_detect(buffer, handler);
}

// HTTP/WS (read).
// ----------------------------------------------------------------------------

void socket::http_read(http::flat_buffer& buffer,
    http::request& request, count_handler&& handler) NOEXCEPT
{
    io::dispatch(strand_,
        std::bind(&socket::do_http_read, shared_from_this(),
            std::ref(buffer), std::ref(request), std::move(handler)));
}

// private
void socket::do_http_read(ref<http::flat_buffer> buffer,
    const ref<http::request>& request,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    async_read_http(buffer.get(), request.get(), handler);
}

// private
void socket::handle_http_read(const code& ec, size_t size,
    const ref<http::request>& request, const http_parser_ptr& parser,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (!ec && http::is_upgrade(parser->get()))
    {
        request.get() = parser->release();
        handler(error::upgrade, size);
        return;
    }

    if (!ec)
    {
        request.get() = parser->release();
        if (request.get().method() == http::verb::unknown)
        {
            handler(error::bad_method, size);
            return;
        }
    }

    handler(ec, size);
}

// HTTP/WS (write).
// ----------------------------------------------------------------------------

void socket::http_write(http::response&& response,
    count_handler&& handler) NOEXCEPT
{
    const auto out = move_shared(std::move(response));
    io::dispatch(strand_,
        std::bind(&socket::do_http_write, shared_from_this(),
            out, std::move(handler)));
}

// private
void socket::do_http_write(const http::response_ptr& response,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    async_write_http(std::move(*response), handler);
}

// private
void socket::handle_http_write(const code& ec, size_t size,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    reclaim_buffer();
    handler(ec, size);
}

// HTTP (parse).
// ----------------------------------------------------------------------------
// The buffered bytes are parsed before reading more. The header parse returns
// once the header is complete, the some parse once it progresses, and the
// full parse once the message is complete. A body reader may pause any parse
// (need_buffer), which is returned to the caller.

// private
void socket::do_parse(const ref<http::flat_buffer>& buffer,
    const http_parser_ptr& parser, parse_mode mode, size_t total,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    auto& in = buffer.get();

    while (true)
    {
        code ec{};
        const auto used = parser->put(in.data(), ec);
        in.consume(used);
        total = ceilinged_add(total, used);

        if (ec == error::http_error_t::need_more)
            break;

        if (ec)
        {
            const auto code = error::http_to_error_code(ec);
            if (code == error::unknown) logx("http-parse", ec);
            handler(code, total);
            return;
        }

        if (parser->is_done() || (mode == parse_mode::header &&
            parser->is_header_done()) || (mode == parse_mode::some &&
            !is_zero(used)))
        {
            handler(error::success, total);
            return;
        }

        if (is_zero(used))
            break;
    }

    // Read into available capacity, growing by up to the header limit.
    const auto limit = floored_subtract(in.max_size(), in.size());
    const auto size = std::min(limit, std::max(in.capacity() - in.size(),
        header_));

    if (is_zero(size))
    {
        handler(error::buffer_overflow, total);
        return;
    }

    async_read_some(in.prepare(size),
        std::bind(&socket::handle_parse,
            shared_from_this(), _1, _2, buffer, parser, mode, total,
            handler));
}

// private
void socket::handle_parse(const code& ec, size_t size,
    const ref<http::flat_buffer>& buffer, const http_parser_ptr& parser,
    parse_mode mode, size_t total, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // The end of the stream completes a body so delimited, or is premature.
    if (ec == error::peer_disconnect)
    {
        code eof{};
        parser->put_eof(eof);
        handler(error::http_to_error_code(eof), total);
        return;
    }

    if (ec)
    {
        handler(ec, total);
        return;
    }

    buffer.get().commit(size);
    do_parse(buffer, parser, mode, total, handler);
}

// HTTP (serialize).
// ----------------------------------------------------------------------------
// Each set of buffers is written in turn.

// private
void socket::do_serialize(const serialize_state::ptr& out, size_t total,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    while (true)
    {
        code ec{};
        out->buffers = out->serializer.next(ec);
        if (ec)
        {
            const auto code = error::http_to_error_code(ec);
            if (code == error::unknown) logx("http-serialize", ec);
            handler(code, total);
            return;
        }

        if (!out->buffers.empty())
            break;

        if (out->serializer.is_done())
        {
            handler(error::success, total);
            return;
        }
    }

    VARIANT_DISPATCH_FUNCTION(io::async_write, get_tcp(),
        out->buffers, std::bind(&socket::handle_serialize,
            shared_from_this(), _1, _2, out, total, handler));
}

// private
void socket::handle_serialize(const code& ec, size_t size,
    const serialize_state::ptr& out, size_t total,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    total = ceilinged_add(total, size);
    if (ec == error::operation_canceled)
    {
        handler(error::channel_stopped, total);
        return;
    }

    if (ec)
    {
        if (ec == error::unknown) logx("http-write", ec);
        handler(ec, total);
        return;
    }

    do_serialize(out, total, handler);
}

// HTTP (progressive read).
// ----------------------------------------------------------------------------
// The caller owns the parser, which owns http framing (content length and
// chunk decoding) across the message. A batched json-rpc body delivers each
// message from the paused parse (need_buffer), mapped to success here.

void socket::http_read_header(http::flat_buffer& buffer,
    const http_parser_ptr& parser, count_handler&& handler) NOEXCEPT
{
    io::dispatch(strand_,
        std::bind(&socket::do_http_read_header, shared_from_this(),
            std::ref(buffer), parser, std::move(handler)));
}

// private
void socket::do_http_read_header(ref<http::flat_buffer> buffer,
    const http_parser_ptr& parser, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Websocket messages have no http headers.
    if (websocket())
    {
        handler(error::operation_failed, {});
        return;
    }

    parser->body_limit(maximum_);
    parser->header_limit(header_);
    do_parse(buffer, parser, parse_mode::header, zero,
        std::bind(&socket::handle_http_read_header,
            shared_from_this(), _1, _2, parser, handler));
}

// private
void socket::handle_http_read_header(const code& ec, size_t size,
    const http_parser_ptr& parser, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (!ec && http::is_upgrade(parser->get()))
    {
        handler(error::upgrade, size);
        return;
    }

    // Dispatch of the unknown verb is reserved for websocket.
    if (!ec && parser->get().method() == http::verb::unknown)
    {
        handler(error::bad_method, size);
        return;
    }

    handler(ec, size);
}

void socket::http_read_some(http::flat_buffer& buffer,
    const http_parser_ptr& parser, count_handler&& handler) NOEXCEPT
{
    io::dispatch(strand_,
        std::bind(&socket::do_http_read_body, shared_from_this(),
            std::ref(buffer), parser, parse_mode::some,
            std::move(handler)));
}

void socket::http_read_body(http::flat_buffer& buffer,
    const http_parser_ptr& parser, count_handler&& handler) NOEXCEPT
{
    io::dispatch(strand_,
        std::bind(&socket::do_http_read_body, shared_from_this(),
            std::ref(buffer), parser, parse_mode::all, std::move(handler)));
}

// private
void socket::do_http_read_body(ref<http::flat_buffer> buffer,
    const http_parser_ptr& parser, parse_mode mode,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Websocket messages have no http framing.
    if (websocket())
    {
        handler(error::operation_failed, {});
        return;
    }

    do_parse(buffer, parser, mode, zero, handler);
}

// HTTP (progressive write).
// ----------------------------------------------------------------------------
// The header is written once (chunked, no content length), and each response
// part is then written as chunk data, the batch close part terminating the
// chunked body. This mirrors the progressive read (nothing is materialized).

void socket::http_write_header(http::response&& response,
    count_handler&& handler) NOEXCEPT
{
    const auto out = emplace_shared<header_state>(
        http::serialize_header(response.base()));

    io::dispatch(strand_,
        std::bind(&socket::do_http_write_header, shared_from_this(),
            out, std::move(handler)));
}

// private
void socket::do_http_write_header(const header_state::ptr& out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Websocket messages have no http headers.
    if (websocket())
    {
        handler(error::operation_failed, {});
        return;
    }

    const io::const_buffer data{ out->header.data(),
        out->header.size() };

    VARIANT_DISPATCH_FUNCTION(io::async_write, get_tcp(),
        data, std::bind(&socket::handle_http_write_header,
            shared_from_this(), _1, _2, out, handler));
}

// private
void socket::handle_http_write_header(const code& ec, size_t size,
    const header_state::ptr&, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec == error::operation_canceled)
    {
        handler(error::channel_stopped, size);
        return;
    }

    if (ec == error::unknown) logx("http-write-header", ec);
    handler(ec, size);
}

void socket::rpc_write_chunk(rpc::response&& response,
    count_handler&& handler) NOEXCEPT
{
    http::response wrapper{};
    wrapper.body() = std::move(response);
    assign_buffer(wrapper.body());

    code ec{};
    const auto out = emplace_shared<chunk_state>(std::move(wrapper));
    out->writer.init(ec);

    io::dispatch(strand_,
        std::bind(&socket::do_chunk_write,
            shared_from_this(), ec, zero, out, std::move(handler)));
}

// private
void socket::do_chunk_write(code ec, size_t total,
    const chunk_state::ptr& out, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Websocket messages have no http framing.
    if (websocket())
    {
        handler(error::operation_failed, total);
        return;
    }

    const auto buffer = ec ? chunk_state::out_buffer{} : out->writer.get(ec);
    if (ec)
    {
        const auto code = error::http_to_error_code(ec);
        if (code == error::unknown) logx("chunk-write", ec);
        handler(code, total);
        return;
    }

    if (!buffer.has_value())
    {
        out->more = false;
        handle_chunk_write({}, zero, total, out, handler);
        return;
    }

    out->more = buffer.value().second;
    const auto& data = buffer.value().first;

    // Frame the part buffer as http chunk data (chunk_state lifetime).
    out->prefix = http::chunk_header(data.size());
    out->buffers =
    {
        io::const_buffer{ out->prefix.data(), out->prefix.size() },
        data,
        io::const_buffer{ http::chunk_end.data(), http::chunk_end.size() }
    };

    VARIANT_DISPATCH_FUNCTION(io::async_write, get_tcp(),
        out->buffers, std::bind(&socket::handle_chunk_write,
            shared_from_this(), _1, _2, total, out, handler));
}

// private
void socket::handle_chunk_write(const code& ec, size_t size,
    size_t total, const chunk_state::ptr& out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    total = ceilinged_add(total, size);
    if (ec == error::operation_canceled)
    {
        reclaim_buffer();
        handler(error::channel_stopped, total);
        return;
    }

    if (ec)
    {
        reclaim_buffer();
        if (ec == error::unknown) logx("chunk-write", ec);
        handler(ec, total);
        return;
    }

    if (out->more)
    {
        do_chunk_write({}, total, out, handler);
        return;
    }

    reclaim_buffer();

    // Only the batch close part terminates the chunked body.
    if (!out->last())
    {
        handler(error::success, total);
        return;
    }

    const io::const_buffer last{ http::last_chunk.data(),
        http::last_chunk.size() };

    VARIANT_DISPATCH_FUNCTION(io::async_write, get_tcp(),
        last, std::bind(&socket::handle_chunk_last,
            shared_from_this(), _1, _2, total, handler));
}

// private
void socket::handle_chunk_last(const code& ec, size_t size,
    size_t total, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    total = ceilinged_add(total, size);
    if (ec == error::operation_canceled)
    {
        handler(error::channel_stopped, total);
        return;
    }

    if (ec == error::unknown) logx("chunk-last", ec);
    handler(ec, total);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
