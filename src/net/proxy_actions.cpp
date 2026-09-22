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
#include <bitcoin/network/net/proxy.hpp>

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
using namespace messages::peer;
using namespace std::placeholders;

// The cost of a queued message is the memory that it pins, so a streaming body
// (file) is charged only for the queue entry that retains it.
// The estimate is cached into the message, so the writer does not repeat it.
template <typename Message>
static size_t to_estimate(Message& message) NOEXCEPT
{
    if (is_zero(message.size_hint))
        message.size_hint = rpc::to_size(message.message);

    return message.size_hint;
}

static size_t to_cost(http::body::value_type& body) NOEXCEPT
{
    if (body.contains<http::json_value>())
        return body.get<http::json_value>().size_hint;

    if (body.contains<rpc::request>())
        return to_estimate(body.get<rpc::request>());

    if (body.contains<rpc::response>())
        return to_estimate(body.get<rpc::response>());

    if (body.contains<http::string_value>())
        return body.get<http::string_value>().size();

    if (body.contains<http::data_value>())
        return body.get<http::data_value>().size();

    if (body.contains<http::span_value>())
        return body.get<http::span_value>().size();

    if (body.contains<http::buffer_value>())
        return body.get<http::buffer_value>().size;

    if (body.contains<http::peer_value>())
        return body.get<http::peer_value>().size;

    return zero;
}

// Wait (all).
// ----------------------------------------------------------------------------

void proxy::watch(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    socket_->watch(std::move(handler));
}

void proxy::unwatch() NOEXCEPT
{
    BC_ASSERT(stranded());
    socket_->unwatch();
}

//  WS (generic, framed).
// ----------------------------------------------------------------------------

// flat_buffer must have configured max_size, which will be allocated.
void proxy::read(http::flat_buffer& out, count_handler&& handler) NOEXCEPT
{
    do_reading();
    socket_->ws_read(out, counted(std::move(handler)));
}

void proxy::write(const asio::const_buffer& in, bool binary,
    count_handler&& handler) NOEXCEPT
{
    writer call = std::bind(&proxy::do_ws_write,
        shared_from_this(), in, binary);

    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_write, shared_from_this(),
            pending{ in.size(), std::move(call), std::move(handler) }, true));
}

// private
void proxy::do_ws_write(const asio::const_buffer& payload,
    bool binary) NOEXCEPT
{
    socket_->ws_write({ payload.data(), payload.size() }, binary,
        metered(std::bind(&proxy::handle_write,
            shared_from_this(), _1, _2)));
}

//  TCP (generic, fixed size).
// ----------------------------------------------------------------------------

void proxy::read(const asio::mutable_buffer& out,
    count_handler&& handler) NOEXCEPT
{
    do_reading();
    socket_->tcp_read(out, counted(std::move(handler)));
}

void proxy::write(const asio::const_buffer& in,
    count_handler&& handler) NOEXCEPT
{
    writer call = std::bind(&proxy::do_tcp_write,
        shared_from_this(), in);

    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_write, shared_from_this(),
            pending{ in.size(), std::move(call), std::move(handler) }, true));
}

// private
void proxy::do_tcp_write(const asio::const_buffer& payload) NOEXCEPT
{
    socket_->tcp_write({ payload.data(), payload.size() },
        metered(std::bind(&proxy::handle_write,
            shared_from_this(), _1, _2)));
}

// PEER (TCP: bitcoin p2p).
// ----------------------------------------------------------------------------

void proxy::read(data_chunk& buffer, frame& message,
    count_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    do_reading();

    socket_->peer_read(buffer, message, counted(std::move(handler)));
}

void proxy::write(frame&& message, count_handler&& handler) NOEXCEPT
{
    write(std::move(message), std::move(handler), false);
}

void proxy::notify(frame&& message, count_handler&& handler) NOEXCEPT
{
    write(std::move(message), std::move(handler), true);
}

// private
void proxy::write(frame&& message, count_handler&& handler,
    bool bounded) NOEXCEPT
{
    // Pointer ships moveable message through the send queue.
    const auto out = move_shared(std::move(message));
    const auto cost = out->size;
    writer call = std::bind(&proxy::do_peer_write,
        shared_from_this(), out);

    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_write, shared_from_this(),
            pending{ cost, std::move(call), std::move(handler) }, bounded));
}

// private
void proxy::do_peer_write(const frame_ptr& message) NOEXCEPT
{
    BC_ASSERT(stranded());

    socket_->peer_write(std::move(*message),
        metered(std::bind(&proxy::handle_write,
            shared_from_this(), _1, _2)));
}

// RPC (TCP: electrum/stratum_v1, WS: btcd).
// ----------------------------------------------------------------------------
// Batch normalization: the channel is batch-blind. The proxy stamps batch
// state on reads and response parts, absorbs the batch close (writing the
// close part and re-arming the read), and defers notifications while open.

// flat_buffer must have configured max_size, which will be allocated.
// The PING context is the second param (the first is the TTL).
static rpc::value_t to_context(const rpc::params_option& params) NOEXCEPT
{
    if (params && std::holds_alternative<rpc::array_t>(*params))
    {
        const auto& values = std::get<rpc::array_t>(*params);
        if (values.size() > one)
            return values.at(one);
    }

    return rpc::null_t{};
}

// Batch open rides on the first element, close on a message with no element.
static bool batch_open(const rpc::request& value) NOEXCEPT
{
    return value.changed && !value.batch;
}

static bool batch_close(const rpc::request& value) NOEXCEPT
{
    return value.changed && value.batch;
}

void proxy::read(http::flat_buffer& buffer, rpc::request& request,
    count_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    do_reading();
    do_rpc_request_read(std::ref(request), std::ref(buffer),
        std::move(handler));
}

// private
void proxy::do_rpc_request_read(const ref<rpc::request>& request,
    const ref<http::flat_buffer>& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Stamp current batch state (the parse is always lax).
    auto& value = request.get();
    value.batch = batched_;
    value.changed = false;

    socket_->rpc_read(buffer.get(), value,
        std::bind(&proxy::handle_rpc_read,
            shared_from_this(), _1, _2, request, buffer, handler));
}

// private
void proxy::handle_rpc_read(const code& ec, size_t bytes,
    const ref<rpc::request>& request, const ref<http::flat_buffer>& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    count_received(bytes);

    if (ec)
    {
        handler(ec, bytes);
        return;
    }

    auto& value = request.get();

    // ZMTP is not batched, a keepalive is absorbed and the read re-armed.
    if (socket_->zeromq())
    {
        if (absorb_keepalive(value))
            do_rpc_request_read(request, buffer, handler);
        else
            handler(ec, bytes);

        return;
    }

    if (batch_open(value))
        batched_ = true;

    // Batch close carries no message, absorbed here (channel never sees it).
    if (batch_close(value))
    {
        queue_close(std::bind(&proxy::handle_close_write,
            shared_from_this(), _1, _2, request, buffer, handler));
        return;
    }

    handler(ec, bytes);
}

// private
// ZMTP control (the channel is keepalive-blind): a PING is answered with a
// PONG through the write queue, a PONG is dropped (the read handler pends).
bool proxy::absorb_keepalive(const rpc::request& value) NOEXCEPT
{
    BC_ASSERT(stranded());

    const auto& message = value.message;
    const auto ping = (message.method == "ping");
    if (!ping && message.method != "pong")
        return false;

    if (ping)
    {
        const auto pong = to_shared<rpc::request>();
        pong->message.method = "pong";
        pong->message.params = rpc::array_t{ to_context(message.params) };

        // The peer paces this answer and the read is re-armed without it,
        // so it is subject to the backlog bound.
        pending entry
        {
            zero,
            std::bind(&proxy::do_notification_write, shared_from_this(), pong),
            [](const code&, size_t) NOEXCEPT {}
        };

        do_write(entry, true);
    }

    return true;
}

// private
// The close part is proxy-created after the batch resets (not stamped).
void proxy::queue_close(count_handler&& complete) NOEXCEPT
{
    BC_ASSERT(stranded());
    batched_ = false;
    parted_ = false;

    rpc::response close{};
    close.batch = true;
    close.changed = true;

    // The close part is queued per framing (http chunk or stream).
    if (parser_)
    {
        const auto out = to_shared<http::response>();
        out->body() = std::move(close);
        pending entry
        {
            zero,
            std::bind(&proxy::do_http_write, shared_from_this(), out),
            std::move(complete)
        };

        do_write(entry, false);
        return;
    }

    const auto out = move_shared(std::move(close));
    pending entry
    {
        zero,
        std::bind(&proxy::do_response_write, shared_from_this(), out),
        std::move(complete)
    };

    do_write(entry, false);
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
    const auto started = !queue_.empty();
    while (!deferred_.empty())
    {
        queue_.push_back(std::move(deferred_.front()));
        deferred_.pop_front();
    }

    if (!started && !queue_.empty())
        write();

    // Re-arm the read (the channel read handler remains pending).
    do_rpc_request_read(request, buffer, handler);
}

void proxy::write(rpc::response&& response, count_handler&& handler) NOEXCEPT
{
    // Pointer ships moveable message through the send queue.
    const auto out = move_shared(std::move(response));
    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_response_queue, shared_from_this(), out,
            std::move(handler)));
}

// private
void proxy::do_response_queue(const rpc::response_ptr& response,
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

    pending entry
    {
        to_estimate(*response),
        std::bind(&proxy::do_response_write, shared_from_this(), response),
        handler
    };

    do_write(entry, false);
}

void proxy::notify(rpc::request&& notification, count_handler&& handler) NOEXCEPT
{
    // Pointer ships moveable message through the send queue.
    const auto out = move_shared(std::move(notification));
    const auto cost = to_estimate(*out);
    writer call = std::bind(&proxy::do_notification_write,
        shared_from_this(), out);

    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_defer_write, shared_from_this(),
            pending{ cost, std::move(call), std::move(handler) }));
}

// private
void proxy::do_defer_write(pending& write_) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Notifications are deferred while a batch is open (drained on close).
    // Charged on defer, as the deferral retains the entry just as the queue.
    if (batched_)
    {
        if (charge(write_)) deferred_.push_back(std::move(write_));
        return;
    }

    do_write(write_, true);
}

// private
void proxy::do_response_write(const rpc::response_ptr& response) NOEXCEPT
{
    BC_ASSERT(stranded());
    socket_->rpc_write(std::move(*response),
        metered(std::bind(&proxy::handle_write,
            shared_from_this(), _1, _2)));
}

// private
void proxy::do_notification_write(
    const rpc::request_ptr& notification) NOEXCEPT
{
    socket_->rpc_notify(std::move(*notification),
        metered(std::bind(&proxy::handle_write,
            shared_from_this(), _1, _2)));
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
        socket_->http_read(buffer, request, counted(std::move(handler)));
        return;
    }

    // A downgrade is the json-rpc transport, so it reads as one. The message
    // lands in the body alternative of the caller's (channel's) request.
    if (socket_->downgraded())
    {
        do_downgrade_read(std::ref(request), std::ref(buffer),
            std::move(handler));
        return;
    }

    // The preselected body is the read/write control, so a json-rpc body
    // implies detection, performed before the first message is read.
    if (!socket_->detected() && request.body().contains<rpc::request>())
    {
        socket_->detect(buffer,
            std::bind(&proxy::handle_detect,
                shared_from_this(), _1, _2, std::ref(request),
                std::ref(buffer), std::move(handler)));
        return;
    }

    // Continue the message in progress (batched body), else next message.
    if (parser_)
    {
        do_http_body_read(std::ref(request), std::ref(buffer),
            std::move(handler));
        return;
    }

    do_http_request_read(std::ref(request), std::ref(buffer),
        std::move(handler));
}

// private
void proxy::handle_detect(const code& ec, size_t bytes,
    const ref<http::request>& request, const ref<http::flat_buffer>& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec)
    {
        handler(ec, bytes);
        return;
    }

    if (socket_->downgraded())
    {
        do_downgrade_read(request, buffer, handler);
        return;
    }

    do_http_request_read(request, buffer, handler);
}

// private
void proxy::do_downgrade_read(const ref<http::request>& request,
    const ref<http::flat_buffer>& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // A downgraded message carries no request line, so synthesize an unknown
    // method for dispatch, as does the websocket upgrade.
    auto& in = request.get();
    in.method_string("stream");
    if (!in.body().contains<rpc::request>())
        in.body() = rpc::request{};

    // Read as json-rpc, which applies batch normalization.
    do_rpc_request_read(std::ref(std::get<rpc::request>(in.body().value())),
        buffer, handler);
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
    count_received(bytes);

    if (ec)
    {
        // The upgrade request is published, the channel accepts or refuses.
        if (ec == error::upgrade)
            request.get() = parser_->release();

        parser_.reset();
        handler(ec, bytes);
        return;
    }

    socket_->http_read_body(buffer.get(), *parser_,
        std::bind(&proxy::handle_http_body,
            shared_from_this(), _1, _2, request, buffer, handler));
}

// private
void proxy::do_http_body_read(const ref<http::request>& request,
    const ref<http::flat_buffer>& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    socket_->http_read_some(buffer.get(), *parser_,
        std::bind(&proxy::handle_http_body,
            shared_from_this(), _1, _2, request, buffer, handler));
}

// private
void proxy::handle_http_body(const code& ec, size_t bytes,
    const ref<http::request>& request, const ref<http::flat_buffer>& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    count_received(bytes);

    // A batched body pauses the parse to deliver each element.
    if (ec == error::need_buffer)
    {
        handle_http_element(bytes, request, buffer, handler);
        return;
    }

    if (ec)
    {
        parser_.reset();
        handler(ec, bytes);
        return;
    }

    // Progress without delivery or completion, continue the body read.
    if (!parser_->is_done())
    {
        do_http_body_read(request, buffer, handler);
        return;
    }

    // Message complete following an absorbed batch close, read the next.
    const auto& body = parser_->get().body();
    if (body.contains<rpc::request>() && batch_close(body.get<rpc::request>()))
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
void proxy::handle_http_element(size_t bytes,
    const ref<http::request>& request, const ref<http::flat_buffer>& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    auto& value = std::get<rpc::request>(parser_->get().body().value());

    if (batch_open(value))
        batched_ = true;

    // Batch close carries no message, absorbed here (close flags remain set
    // on the parser value for message completion detection).
    if (batch_close(value))
    {
        queue_close(std::bind(&proxy::handle_http_close_write,
            shared_from_this(), _1, _2, request, buffer, handler));
        return;
    }

    // Deliver the element as a request (headers from the message).
    request.get().base() = parser_->get().base();
    request.get().body() = std::move(value);
    value.batch = true;
    value.changed = false;
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
    do_http_body_read(request, buffer, handler);
}

void proxy::write(http::response&& response,
    count_handler&& handler) NOEXCEPT
{
    write(std::move(response), std::move(handler), false);
}

void proxy::notify(http::response&& notification,
    count_handler&& handler) NOEXCEPT
{
    write(std::move(notification), std::move(handler), true);
}

// private
void proxy::write(http::response&& response, count_handler&& handler,
    bool bounded) NOEXCEPT
{
    // A downgrade is the json-rpc transport, so it writes as one (batch
    // stamping for a response, deferral while open for a notification).
    if (socket_->downgraded())
    {
        auto& body = response.body();
        if (body.contains<rpc::response>())
        {
            write(std::move(std::get<rpc::response>(body.value())),
                std::move(handler));
            return;
        }

        if (body.contains<rpc::request>())
        {
            notify(std::move(std::get<rpc::request>(body.value())),
                std::move(handler));
            return;
        }

        handler(error::bad_stream, zero);
        return;
    }

    // Pointer ships moveable message through the send queue.
    const auto out = move_shared(std::move(response));
    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_http_queue, shared_from_this(), out,
            std::move(handler), bounded));
}

// private
void proxy::do_http_queue(const http::response_ptr& response,
    const count_handler& handler, bool bounded) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Stamp an http batch part with current batch state (ws does not batch).
    if (batched_ && !socket_->websocket())
    {
        auto& body = response->body();
        if (!body.contains<rpc::response>())
        {
            handler(error::bad_stream, zero);
            return;
        }

        auto& part = std::get<rpc::response>(body.value());
        part.batch = parted_;
        part.changed = !parted_;
        parted_ = true;
    }

    pending entry
    {
        to_cost(response->body()),
        std::bind(&proxy::do_http_write, shared_from_this(), response),
        handler
    };

    do_write(entry, bounded);
}

// private
void proxy::handle_http_header_write(const code& ec, size_t bytes,
    const rpc::response_ptr& part) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec)
    {
        handle_write(ec, bytes);
        return;
    }

    socket_->rpc_write_chunk(std::move(*part),
        metered(std::bind(&proxy::handle_write,
            shared_from_this(), _1, _2)));
}

// private
void proxy::do_http_write(const http::response_ptr& response) NOEXCEPT
{
    BC_ASSERT(stranded());

    // A stamped batch part is chunked, the first following the header.
    auto& body = response->body();
    if (!socket_->websocket() && body.contains<rpc::response>())
    {
        auto& part = std::get<rpc::response>(body.value());
        if (part.batch)
        {
            socket_->rpc_write_chunk(std::move(part),
                metered(std::bind(&proxy::handle_write,
                    shared_from_this(), _1, _2)));
            return;
        }

        if (part.changed)
        {
            const auto out = move_shared(std::move(part));
            response->body() = http::empty_value{};
            response->chunked(true);
            socket_->http_write_header(std::move(*response),
                metered(std::bind(&proxy::handle_http_header_write,
                    shared_from_this(), _1, _2, out)));
            return;
        }
    }

    socket_->http_write(std::move(*response),
        metered(std::bind(&proxy::handle_write,
            shared_from_this(), _1, _2)));
}

// ZMTP (TCP: publisher).
// ----------------------------------------------------------------------------
// Keepalive absorption: the channel is keepalive-blind. PING is answered
// with a PONG through the write queue and the read re-armed (as the rpc
// batch close is absorbed above). Every other frame is delivered; the
// subscription protocol is the channel's concern.

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
