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

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/settings.hpp>
#include <bitcoin/network/ws/ws.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

// WS (read).
// ----------------------------------------------------------------------------
// The flat_buffer's max_size() must be large enough to hold the complete ws
// framed message. The read will fail if the message exceeds this limit.

void socket::ws_read(http::flat_buffer& out,
    count_handler&& handler) NOEXCEPT
{
    io::dispatch(strand_,
        std::bind(&socket::do_ws_read,
            shared_from_this(), std::ref(out), std::move(handler)));
}

// private
void socket::do_ws_read(ref<http::flat_buffer> out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    async_read(out.get(), handler);
}

// WS (write).
// ----------------------------------------------------------------------------

void socket::ws_write(const io::const_buffer& in, bool binary,
    count_handler&& handler) NOEXCEPT
{
    io::dispatch(strand_,
        std::bind(&socket::do_ws_write,
            shared_from_this(), in, binary, std::move(handler)));
}

// private
void socket::do_ws_write(const io::const_buffer& in, bool binary,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    async_write(in, binary, true, handler);
}

// WS (upgrade).
// ----------------------------------------------------------------------------

// The response is written before the upgrade (or refusal) is returned.
code socket::accept_websocket(const http::request& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(!websocket());

    ws::response response{};
    const auto ec = ws::accept(response, request.base());
    response.set(http::field::server, BC_HTTP_SERVER_NAME);

    http::serializer<false, http::string_body> serializer{ response };
    data_chunk bytes{};
    code error{};
    while (!serializer.is_done() && !error)
        for (const auto& buffer: serializer.next(error))
            bytes.insert(bytes.end(), pointer_cast<const uint8_t>(
                buffer.data()), std::next(pointer_cast<const uint8_t>(
                    buffer.data()), buffer.size()));

    VARIANT_DISPATCH_FUNCTION(io::write, get_tcp(),
        io::buffer(bytes), error);

    if (error || ec)
        return error::operation_failed;

    ws_reader_.emplace(maximum_, true);
    ws_input_.clear();
    ws_continuing_ = false;
    ws_closing_ = false;
    websocket_.store(true);
    return error::success;
}

// WS (framing).
// ----------------------------------------------------------------------------
// A read assembles one data message, answering pings and the peer's close.
// Frames are written in order, so control frames queue behind data frames.

// private
void socket::do_ws_read_message(const ref<http::flat_buffer>& out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    while (true)
    {
        ws::reader::event event{};
        code ec{};
        const auto data = ws_input_.data();
        const std::span<const uint8_t> bytes{ pointer_cast<const uint8_t>(
            data.data()), data.size() };

        const auto used = ws_reader_->put(bytes, out.get(), event, ec);
        ws_input_.consume(used);

        if (ec == error::need_more)
            break;

        if (ec)
        {
            handler(ec, out.get().size());
            return;
        }

        const auto& payload = ws_reader_->control();
        switch (event)
        {
            case ws::reader::event::message:
                handler(error::success, out.get().size());
                return;
            case ws::reader::event::ping:
                do_ws_event(ws::opcode::ping, payload);
                ws_pong(payload);
                continue;
            case ws::reader::event::pong:
                do_ws_event(ws::opcode::pong, payload);
                continue;
            case ws::reader::event::close:
            {
                do_ws_event(ws::opcode::close, payload);
                if (ws_closing_)
                {
                    handler(error::websocket_closed, out.get().size());
                    return;
                }

                // The close is answered with the peer's close code.
                ws_closing_ = true;
                const auto echo = payload.size() < two ? data_chunk{} :
                    data_chunk{ payload.begin(), std::next(payload.begin(),
                        two) };

                ws_send(ws::opcode::close, echo,
                    std::bind(&socket::handle_ws_echo,
                        shared_from_this(), _1, _2, out, handler));
                return;
            }
            case ws::reader::event::none:
            default:
                continue;
        }
    }

    const auto limit = floored_subtract(ws_input_.max_size(),
        ws_input_.size());
    const auto size = std::min(limit, std::max(ws_input_.capacity() -
        ws_input_.size(), header_));

    try
    {
        VARIANT_DISPATCH_METHOD(get_tcp(),
            async_read_some(ws_input_.prepare(size),
                std::bind(&socket::handle_ws_read_message,
                    shared_from_this(), _1, _2, out, handler)));
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ ws_read: " << e.what());
        handler(error::operation_failed, {});
    }
}

// private
void socket::handle_ws_read_message(const code& ec, size_t size,
    const ref<http::flat_buffer>& out, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec == error::operation_canceled)
    {
        handler(error::channel_stopped, out.get().size());
        return;
    }

    if (ec)
    {
        if (ec == error::unknown) logx("ws-read", ec);
        handler(ec, out.get().size());
        return;
    }

    ws_input_.commit(size);
    do_ws_read_message(out, handler);
}

// private
// The server closes the connection once the closing handshake is complete.
void socket::handle_ws_echo(const code&, size_t,
    const ref<http::flat_buffer>& out, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    code ignore{};
    get_base().shutdown(io::socket::shutdown_type::send, ignore);
    handler(error::websocket_closed, out.get().size());
}

// private
// A data frame continues the message of a preceding unfinished frame.
void socket::ws_send_data(const io::const_buffer& data, bool binary,
    bool finish, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    const auto code = ws_continuing_ ? ws::opcode::continuation :
        (binary ? ws::opcode::binary : ws::opcode::text);

    ws_continuing_ = !finish;
    const auto frame = emplace_shared<ws_frame>();
    frame->header = ws::serialize_header(code, finish,
        possible_wide_cast<uint64_t>(data.size()));
    frame->data = data;
    frame->handler = handler;
    ws_queue_.push_back(frame);
    ws_write_next();
}

// private
void socket::ws_send(ws::opcode code, const data_chunk& payload,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    const auto frame = emplace_shared<ws_frame>();
    frame->header = ws::serialize_header(code, true,
        possible_wide_cast<uint64_t>(payload.size()));
    frame->payload = payload;
    frame->handler = handler;
    ws_queue_.push_back(frame);
    ws_write_next();
}

// private
// An unsent pong answers only the most recent ping (rfc6455 5.5.3).
void socket::ws_pong(const data_chunk& payload) NOEXCEPT
{
    BC_ASSERT(stranded());

    const auto queued = !is_null(ws_pong_);
    if (!queued)
        ws_pong_ = emplace_shared<ws_frame>();

    ws_pong_->header = ws::serialize_header(ws::opcode::pong, true,
        possible_wide_cast<uint64_t>(payload.size()));
    ws_pong_->payload = payload;

    if (!queued)
    {
        ws_queue_.push_back(ws_pong_);
        ws_write_next();
    }
}

// private
void socket::ws_write_next() NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ws_writing_ || ws_queue_.empty())
        return;

    ws_writing_ = true;
    const auto frame = ws_queue_.front();
    if (frame == ws_pong_)
        ws_pong_.reset();
    const auto payload = frame->payload.empty() ? frame->data :
        io::const_buffer{ frame->payload.data(), frame->payload.size() };

    frame->buffers =
    {
        io::const_buffer{ frame->header.data(), frame->header.size() },
        payload
    };

    VARIANT_DISPATCH_FUNCTION(io::async_write, get_tcp(),
        frame->buffers, std::bind(&socket::handle_ws_frame,
            shared_from_this(), _1, _2, frame));
}

// private
void socket::handle_ws_frame(const code& ec, size_t,
    const ws_frame::ptr& frame) NOEXCEPT
{
    BC_ASSERT(stranded());

    ws_writing_ = false;
    ws_queue_.pop_front();
    const auto code = (ec == error::operation_canceled) ? error::channel_stopped :
        ec;

    if (code == error::unknown)
        logx("ws-write", ec);

    // A failed write fails the frames that follow it. These are removed before
    // any handler runs, as a handler may queue (and write) another frame.
    std::deque<ws_frame::ptr> failed{};
    if (code)
    {
        ws_pong_.reset();
        std::swap(failed, ws_queue_);
    }

    // The payload size is reported (as with stream writes).
    const auto size = frame->payload.empty() ? frame->data.size() :
        frame->payload.size();

    if (frame->handler)
        frame->handler(code, size);

    for (const auto& next: failed)
        if (next->handler)
            next->handler(code, zero);

    if (!code)
        ws_write_next();
}

// WS (events).
// ----------------------------------------------------------------------------

// private
void socket::do_ws_event(ws::opcode kind, const data_chunk& data) NOEXCEPT
{
    BC_ASSERT(stranded());

    switch (kind)
    {
        case ws::opcode::ping:
            LOGX("WS ping [" << endpoint() << "] size: " << data.size());
            break;
        case ws::opcode::pong:
            LOGX("WS pong [" << endpoint() << "] size: " << data.size());
            break;
        case ws::opcode::close:
        {
            uint16_t value{};
            std::string reason{};
            code ignore{};
            ws::parse_close(value, reason, data, ignore);
            LOGX("WS close [" << endpoint() << "] " << value << " "
                << reason);
            break;
        }
        default:
            break;
    }
}

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
