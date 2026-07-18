/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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

#include <memory>
#include <utility>
#include <variant>
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

// HTTP/WS (read).
// ----------------------------------------------------------------------------

void socket::http_read(http::flat_buffer& buffer,
    http::request& request, count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
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
void socket::handle_http_read(const boost_code& ec, size_t size,
    const ref<http::request>& request, const http_parser_ptr& parser,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    if (!ec && boost::beast::websocket::is_upgrade(parser->get()))
    {
        handler(set_websocket(parser->get()), size);
        return;
    }

    if (!ec)
    {
        request.get() = parser->release();

        // Dispatch of `verb::unknown` is reserved for websocket.
        if (request.get().method() == http::verb::unknown)
        {
            handler(error::bad_method, size);
            return;
        }
    }

    const auto code = error::http_to_error_code(ec);
    if (code == error::unknown) logx("http-read", ec);
    handler(code, size);
}

// HTTP/WS (write).
// ----------------------------------------------------------------------------

void socket::http_write(http::response&& response,
    count_handler&& handler) NOEXCEPT
{
    const auto out = move_shared(std::move(response));
    boost::asio::dispatch(strand_,
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
void socket::handle_http_write(const boost_code& ec, size_t size,
    const http::response_ptr&, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    const auto code = error::http_to_error_code(ec);
    if (code == error::unknown) logx("http-write", ec);
    handler(code, size);
}

// HTTP (progressive read).
// ----------------------------------------------------------------------------
// The caller owns the parser, which owns http framing (content length and
// chunk decoding) across the message. A batched json-rpc body delivers each
// message from the paused parse (need_buffer), mapped to success here.

void socket::http_read_header(http::flat_buffer& buffer, http_parser& parser,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_http_read_header, shared_from_this(),
            std::ref(buffer), std::ref(parser), std::move(handler)));
}

// private
void socket::do_http_read_header(ref<http::flat_buffer> buffer,
    const ref<http_parser>& parser, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    try
    {
        // Websocket messages have no http headers.
        if (websocket())
        {
            handler(error::operation_failed, {});
            return;
        }

        auto& parse = parser.get();
        parse.body_limit(maximum_);
        parse.header_limit(limit<uint32_t>(maximum_));

        VARIANT_DISPATCH_FUNCTION(boost::beast::http::async_read_header,
            get_tcp(), buffer.get(), parse,
            std::bind(&socket::handle_http_read_header,
                shared_from_this(), _1, _2, parser, handler));
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ http_read_header: " << e.what());
        handler(error::operation_failed, {});
    }
}

// private
void socket::handle_http_read_header(const boost_code& ec, size_t size,
    const ref<http_parser>& parser, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    if (!ec && boost::beast::websocket::is_upgrade(parser.get().get()))
    {
        handler(set_websocket(parser.get().get()), size);
        return;
    }

    // Dispatch of `verb::unknown` is reserved for websocket.
    if (!ec && parser.get().get().method() == http::verb::unknown)
    {
        handler(error::bad_method, size);
        return;
    }

    const auto code = error::http_to_error_code(ec);
    if (code == error::unknown) logx("http-read-header", ec);
    handler(code, size);
}

void socket::http_read_some(http::flat_buffer& buffer, http_parser& parser,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_http_read_some, shared_from_this(),
            std::ref(buffer), std::ref(parser), std::move(handler)));
}

// private
void socket::do_http_read_some(ref<http::flat_buffer> buffer,
    const ref<http_parser>& parser, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    try
    {
        // Websocket messages have no http framing.
        if (websocket())
        {
            handler(error::operation_failed, {});
            return;
        }

        VARIANT_DISPATCH_FUNCTION(boost::beast::http::async_read_some,
            get_tcp(), buffer.get(), parser.get(),
            std::bind(&socket::handle_http_read_some,
                shared_from_this(), _1, _2, handler));
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ http_read_some: " << e.what());
        handler(error::operation_failed, {});
    }
}

// private
void socket::handle_http_read_some(const boost_code& ec, size_t size,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    // The batched body reader delivers a message by pausing the parse.
    if (error::http_to_error_code(ec) == error::need_buffer)
    {
        handler(error::success, size);
        return;
    }

    const auto code = error::http_to_error_code(ec);
    if (code == error::unknown) logx("http-read-some", ec);
    handler(code, size);
}

// HTTP (progressive write).
// ----------------------------------------------------------------------------
// The header is written once (chunked, no content length), and each response
// part is then written as chunk data, the batch close part terminating the
// chunked body. This mirrors the progressive read (nothing is materialized).

void socket::http_write_header(http::response&& response,
    count_handler&& handler) NOEXCEPT
{
    const auto out = emplace_shared<header_state>(std::move(response));
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_http_write_header, shared_from_this(),
            out, std::move(handler)));
}

// private
void socket::do_http_write_header(const header_state::ptr& out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    try
    {
        // Websocket messages have no http headers.
        if (websocket())
        {
            handler(error::operation_failed, {});
            return;
        }

        VARIANT_DISPATCH_FUNCTION(boost::beast::http::async_write_header,
            get_tcp(), out->serializer,
            std::bind(&socket::handle_http_write_header,
                shared_from_this(), _1, _2, out, handler));
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ http_write_header: " << e.what());
        handler(error::operation_failed, {});
    }
}

// private
void socket::handle_http_write_header(const boost_code& ec, size_t size,
    const header_state::ptr&, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    const auto code = error::http_to_error_code(ec);
    if (code == error::unknown) logx("http-write-header", ec);
    handler(code, size);
}

void socket::rpc_write_chunk(rpc::response&& response,
    count_handler&& handler) NOEXCEPT
{
    http::response wrapper{};
    wrapper.body() = std::move(response);

    boost_code ec{};
    const auto out = emplace_shared<chunk_state>(std::move(wrapper));
    out->writer.init(ec);

    boost::asio::dispatch(strand_,
        std::bind(&socket::do_chunk_write,
            shared_from_this(), ec, zero, out, std::move(handler)));
}

// private
void socket::do_chunk_write(boost_code ec, size_t total,
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
    if (!buffer.has_value())
    {
        handler(error::bad_stream, total);
        return;
    }

    if (ec)
    {
        const auto code = error::http_to_error_code(ec);
        if (code == error::unknown) logx("chunk-write", ec);
        handler(code, total);
        return;
    }

    out->more = buffer.value().second;
    const auto& data = buffer.value().first;

    try
    {
        // Frame the part buffer as http chunk data (chunk_state lifetime).
        out->part.emplace(boost::beast::http::make_chunk(data));

        VARIANT_DISPATCH_FUNCTION(boost::asio::async_write, get_tcp(),
            out->part.value(), std::bind(&socket::handle_chunk_write,
                shared_from_this(), _1, _2, total, out, handler));
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ chunk_write: " << e.what());
        handler(error::operation_failed, total);
    }
}

// private
void socket::handle_chunk_write(const boost_code& ec, size_t size,
    size_t total, const chunk_state::ptr& out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    total = ceilinged_add(total, size);
    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, total);
        return;
    }

    if (ec)
    {
        const auto code = error::asio_to_error_code(ec);
        if (code == error::unknown) logx("chunk-write", ec);
        handler(code, total);
        return;
    }

    if (out->more)
    {
        do_chunk_write({}, total, out, handler);
        return;
    }

    // Only the batch close part terminates the chunked body.
    if (!out->last())
    {
        handler(error::success, total);
        return;
    }

    try
    {
        static constexpr std::string_view terminal{ "0\r\n\r\n" };
        const asio::const_buffer last{ terminal.data(), terminal.size() };

        VARIANT_DISPATCH_FUNCTION(boost::asio::async_write, get_tcp(),
            last, std::bind(&socket::handle_chunk_last,
                shared_from_this(), _1, _2, total, handler));
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ chunk_last: " << e.what());
        handler(error::operation_failed, total);
    }
}

// private
void socket::handle_chunk_last(const boost_code& ec, size_t size,
    size_t total, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    total = ceilinged_add(total, size);
    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, total);
        return;
    }

    const auto code = error::asio_to_error_code(ec);
    if (code == error::unknown) logx("chunk-last", ec);
    handler(code, total);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
