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
#include <bitcoin/network/net/proxy.hpp>

#include <utility>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace system;
using namespace std::placeholders;

// Wait (all).
// ----------------------------------------------------------------------------

void proxy::wait(result_handler&& handler) NOEXCEPT
{
    socket_->wait(std::move(handler));
}

void proxy::cancel(result_handler&& handler) NOEXCEPT
{
    socket_->cancel(std::move(handler));
}

//  WS (generic, framed).
// ----------------------------------------------------------------------------

// flat_buffer must have configured max_size, which will be allocated.
void proxy::read(http::flat_buffer& out, count_handler&& handler) NOEXCEPT
{
    do_reading();
    socket_->ws_read(out, std::move(handler));
}

void proxy::write(const asio::const_buffer& in, bool binary,
    count_handler&& handler) NOEXCEPT
{
    writer call = std::bind(&proxy::do_ws_write,
        shared_from_this(), in, binary, std::move(handler));

    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_write,
            shared_from_this(), std::move(call)));
}

// private
void proxy::do_ws_write(const asio::const_buffer& payload, bool binary,
    const count_handler& handler) NOEXCEPT
{
    socket_->ws_write({ payload.data(), payload.size() }, binary,
        std::bind(&proxy::handle_write,
            shared_from_this(), _1, _2, handler));
}

//  TCP (generic, fixed size).
// ----------------------------------------------------------------------------

void proxy::read(const asio::mutable_buffer& out,
    count_handler&& handler) NOEXCEPT
{
    do_reading();
    socket_->tcp_read(out, std::move(handler));
}

void proxy::write(const asio::const_buffer& in,
    count_handler&& handler) NOEXCEPT
{
    writer call = std::bind(&proxy::do_tcp_write,
        shared_from_this(), in, std::move(handler));

    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_write,
            shared_from_this(), std::move(call)));
}

// private
void proxy::do_tcp_write(const asio::const_buffer& payload,
    const count_handler& handler) NOEXCEPT
{
    socket_->tcp_write({ payload.data(), payload.size() },
        std::bind(&proxy::handle_write,
            shared_from_this(), _1, _2, handler));
}

// RPC (TCP: electrum/stratum_v1, WS: btcd).
// ----------------------------------------------------------------------------
// Batch normalization: the channel is batch-blind. The proxy stamps batch
// state on reads and response parts, absorbs the batch close (writing the
// close part and re-arming the read), and defers notifications while open.

// flat_buffer must have configured max_size, which will be allocated.
void proxy::read(http::flat_buffer& buffer, rpc::request& request,
    count_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    do_reading();

    // Stamp current batch state (the parse is always lax).
    request.batch = batched_;
    request.changed = false;

    socket_->rpc_read(buffer, request,
        std::bind(&proxy::handle_rpc_read,
            shared_from_this(), _1, _2, std::ref(request), std::ref(buffer),
            std::move(handler)));
}

// private
void proxy::handle_rpc_read(const code& ec, size_t bytes,
    const ref<rpc::request>& request, const ref<http::flat_buffer>& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec)
    {
        handler(ec, bytes);
        return;
    }

    auto& value = request.get();

    // Batch open rides along with the first element (message delivered).
    if (value.changed && !value.batch)
        batched_ = true;

    // Batch close carries no message, absorbed here (channel never sees it).
    if (value.changed && value.batch)
    {
        batched_ = false;
        parted_ = false;

        // Queue the batch close part (do_response_write does not restamp).
        const auto out = to_shared<rpc::response>();
        out->batch = true;
        out->changed = true;

        const count_handler complete = std::bind(&proxy::handle_close_write,
            shared_from_this(), _1, _2, request, buffer, handler);

        do_write(std::bind(&proxy::do_response_write,
            shared_from_this(), out, complete));
        return;
    }

    handler(ec, bytes);
}

// private
void proxy::handle_close_write(const code& ec, size_t bytes,
    const ref<rpc::request>& request, const ref<http::flat_buffer>& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec)
    {
        handler(ec, bytes);
        return;
    }

    // Drain notifications deferred while the batch was open.
    while (!deferred_.empty())
    {
        do_write(deferred_.front());
        deferred_.pop_front();
    }

    // Re-arm the read (the channel read handler remains pending).
    auto& value = request.get();
    value.batch = batched_;
    value.changed = false;

    socket_->rpc_read(buffer.get(), value,
        std::bind(&proxy::handle_rpc_read,
            shared_from_this(), _1, _2, request, buffer, handler));
}

void proxy::write(rpc::response&& response, count_handler&& handler) NOEXCEPT
{
    // Pointer ships moveable message through the send queue.
    const auto out = move_shared(std::move(response));
    writer call = std::bind(&proxy::do_response_write,
        shared_from_this(), out, std::move(handler));

    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_write,
            shared_from_this(), std::move(call)));
}

void proxy::write(rpc::request&& notification, count_handler&& handler) NOEXCEPT
{
    // Pointer ships moveable message through the send queue.
    const auto out = move_shared(std::move(notification));
    writer call = std::bind(&proxy::do_notification_write,
        shared_from_this(), out, std::move(handler));

    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_defer_write,
            shared_from_this(), std::move(call)));
}

// private
void proxy::do_defer_write(const writer& call) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Notifications are deferred while a batch is open (drained on close).
    if (batched_)
    {
        deferred_.push_back(call);
        return;
    }

    do_write(call);
}

// private
void proxy::do_response_write(const rpc::response_ptr& response,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Stamp the response part with current batch state (open rides on the
    // first part). The close part is proxy-created after the batch resets.
    if (batched_)
    {
        response->batch = parted_;
        response->changed = !parted_;
        parted_ = true;
    }

    socket_->rpc_write(std::move(*response),
        std::bind(&proxy::handle_write,
            shared_from_this(), _1, _2, handler));
}

// private
void proxy::do_notification_write(const rpc::request_ptr& notification,
    const count_handler& handler) NOEXCEPT
{
    socket_->rpc_notify(std::move(*notification),
        std::bind(&proxy::handle_write,
            shared_from_this(), _1, _2, handler));
}

// HTTP/WS (generic/rpc).
// ----------------------------------------------------------------------------
// Batch normalization (http): the header is read once per message, then the
// body delivers each message from the paused parse (a singleton completes in
// one body read). The batch close is absorbed (writing the close part chunk
// and completing the message), and the response header is written once with
// each response part written as chunk data (http has no notifications).

// flat_buffer must have configured max_size, which will be allocated.
void proxy::read(http::flat_buffer& buffer, http::request& request,
    count_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    do_reading();

    if (socket_->websocket())
    {
        socket_->http_read(buffer, request, std::move(handler));
        return;
    }

    // Continue the message in progress (batched body), else next message.
    if (parser_)
    {
        socket_->http_read_some(buffer, *parser_,
            std::bind(&proxy::handle_http_body,
                shared_from_this(), _1, _2, std::ref(request),
                std::ref(buffer), std::move(handler)));
        return;
    }

    do_http_request_read(std::ref(request), std::ref(buffer),
        std::move(handler));
}

// private
void proxy::do_http_request_read(const ref<http::request>& request,
    const ref<http::flat_buffer>& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    parser_ = to_shared<socket::http_parser>();
    socket_->http_read_header(buffer.get(), *parser_,
        std::bind(&proxy::handle_http_header,
            shared_from_this(), _1, _2, request, buffer, handler));
}

// private
void proxy::handle_http_header(const code& ec, size_t bytes,
    const ref<http::request>& request, const ref<http::flat_buffer>& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec)
    {
        // Includes error::upgraded (channel restarts read as websocket).
        parser_.reset();
        handler(ec, bytes);
        return;
    }

    socket_->http_read_body(buffer.get(), *parser_,
        std::bind(&proxy::handle_http_body,
            shared_from_this(), _1, _2, request, buffer, handler));
}

// private
void proxy::handle_http_body(const code& ec, size_t bytes,
    const ref<http::request>& request, const ref<http::flat_buffer>& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // A batched body delivers each message by pausing the parse.
    const auto paused = (ec == error::need_buffer);

    if (ec && !paused)
    {
        parser_.reset();
        handler(ec, bytes);
        return;
    }

    if (paused)
    {
        auto& value = std::get<rpc::request>(parser_->get().body().value());

        // Batch open rides along with the first element (delivered below).
        if (value.changed && !value.batch)
            batched_ = true;

        // Batch close carries no message, absorbed here (close flags remain
        // set on the parser value for message completion detection).
        if (value.changed && value.batch)
        {
            batched_ = false;
            parted_ = false;

            // Write the close part chunk, then complete the message.
            rpc::response close{};
            close.batch = true;
            close.changed = true;

            socket_->rpc_write_chunk(std::move(close),
                std::bind(&proxy::handle_http_close_write,
                    shared_from_this(), _1, _2, request, buffer, handler));
            return;
        }

        // Deliver the element as a request (headers from the message).
        request.get().base() = parser_->get().base();
        request.get().body() = std::move(value);
        value.batch = true;
        value.changed = false;
        handler(error::success, bytes);
        return;
    }

    // Progress without delivery or completion, continue the body read.
    if (!parser_->is_done())
    {
        socket_->http_read_some(buffer.get(), *parser_,
            std::bind(&proxy::handle_http_body,
                shared_from_this(), _1, _2, request, buffer, handler));
        return;
    }

    // Message complete following an absorbed batch close, read the next.
    const auto& body = parser_->get().body();
    if (body.contains<rpc::request>() && body.get<rpc::request>().changed)
    {
        parser_.reset();
        do_http_request_read(request, buffer, handler);
        return;
    }

    // Message complete (singleton), deliver it.
    request.get() = parser_->release();
    parser_.reset();
    handler(error::success, bytes);
}

// private
void proxy::handle_http_close_write(const code& ec, size_t bytes,
    const ref<http::request>& request, const ref<http::flat_buffer>& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec)
    {
        parser_.reset();
        handler(ec, bytes);
        return;
    }

    // Complete the message (consumes trailing body whitespace).
    socket_->http_read_some(buffer.get(), *parser_,
        std::bind(&proxy::handle_http_body,
            shared_from_this(), _1, _2, request, buffer, handler));
}

void proxy::write(http::response&& response,
    count_handler&& handler) NOEXCEPT
{
    if (socket_->websocket())
    {
        // Pointer ships moveable message through the send queue.
        const auto out = move_shared(std::move(response));
        writer call = std::bind(&proxy::do_http_write,
            shared_from_this(), out, std::move(handler));

        boost::asio::dispatch(strand(),
            std::bind(&proxy::do_write,
                shared_from_this(), std::move(call)));
        return;
    }

    // http batch response: header once (chunked), then response parts.
    if (batched_)
    {
        auto& body = response.body();
        if (!body.contains<rpc::response>())
        {
            handler(error::bad_stream, zero);
            return;
        }

        // Stamp the response part with current batch state.
        rpc::response part{ std::move(std::get<rpc::response>(body.value())) };
        part.batch = parted_;
        part.changed = !parted_;

        if (parted_)
        {
            socket_->rpc_write_chunk(std::move(part), std::move(handler));
            return;
        }

        // First part: write the header (chunked), then the open part.
        parted_ = true;
        response.body() = http::empty_value{};
        response.chunked(true);

        const auto out = move_shared(std::move(part));
        socket_->http_write_header(std::move(response),
            std::bind(&proxy::handle_http_header_write,
                shared_from_this(), _1, _2, out, std::move(handler)));
        return;
    }

    // http is half duplex so there is no interleave risk.
    socket_->http_write(std::move(response), std::move(handler));
}

// private
void proxy::handle_http_header_write(const code& ec, size_t bytes,
    const rpc::response_ptr& part, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec)
    {
        handler(ec, bytes);
        return;
    }

    socket_->rpc_write_chunk(std::move(*part), move_copy(handler));
}

// private
void proxy::do_http_write(const http::response_ptr& response,
    const count_handler& handler) NOEXCEPT
{
    socket_->http_write(std::move(*response),
        std::bind(&proxy::handle_write,
            shared_from_this(), _1, _2, handler));
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
