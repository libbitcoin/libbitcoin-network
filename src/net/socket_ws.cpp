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

#include <utility>
#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace network::rpc;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// WS (read).
// ----------------------------------------------------------------------------
// The flat_buffer's max_size() must be large enough to hold the complete ws
// framed message. The read will fail if the message exceeds this limit.

void socket::ws_read(http::flat_buffer& out,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
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
// Buffer is fully allocated before write, identical to tcp_write.

void socket::ws_write(const asio::const_buffer& in, bool binary,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_ws_write,
            shared_from_this(), in, binary, std::move(handler)));
}

// private
void socket::do_ws_write(const asio::const_buffer& in, bool binary,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Single-shot write of an already-whole buffer, always closes the
    // message (finish = true).
    async_write(in, binary, true, handler);
}

// WS (event).
// ----------------------------------------------------------------------------
// This is a unique/internal aspect of websockets.

// private
void socket::do_ws_event(ws::frame_type kind,
    const std::string& data) NOEXCEPT
{
    // Must not post to the iocontext once closed, and this is under control of
    // the websocket, so must be guarded here. Otherwise the socket will leak.
    if (stopped())
        return;

    // Takes ownership of the string.
    boost::asio::dispatch(strand_,
        std::bind(&socket::handle_ws_event,
            shared_from_this(), kind, std::string{ data }));
}

// private
void socket::handle_ws_event(ws::frame_type kind,
    const std::string& data) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Beast sends the necessary responses during our read.
    // Close will be picked up in our async read/write handlers.
    switch (kind)
    {
        case ws::frame_type::ping:
            LOGX("WS ping [" << endpoint() << "] size: " << data.size());
            break;
        case ws::frame_type::pong:
            LOGX("WS pong [" << endpoint() << "] size: " << data.size());
            break;
        case ws::frame_type::close:
            std::visit([&](auto&& value) NOEXCEPT
            {
                LOGX("WS close [" << endpoint() << "] "
                    << value.get().reason());
            }, get_ws());
            break;
    }
}

//  WS (http upgrade).
// ----------------------------------------------------------------------------
// This is a unique aspect of websockets. Encodng is set to text by default.
// This allows full generalization between tcp and websockets for json-rpc.

// TODO: inject server name from config.
code socket::accept_websocket(const http::request& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(!websocket());

    try
    {
        if (secure())
        {
            // Extract before emplacing back to same variant.
            auto ws = std::move(std::get<asio::ssl::socket>(socket_));
            socket_.emplace<ws::ssl::socket>(std::move(ws));

            auto& sock = std::get<ws::ssl::socket>(socket_);
            sock.read_message_max(maximum_);
            sock.set_option(ws::decorator
            {
                [](http::fields& header) NOEXCEPT
                {
                    header.set(http::field::server, BC_HTTP_SERVER_NAME);
                }
            });
            sock.control_callback(std::bind(&socket::do_ws_event,
                shared_from_this(), _1, _2));
            sock.text(true);
            sock.accept(request);
        }
        else
        {
            // Extract before emplacing back to same variant.
            auto ws = std::move(std::get<asio::socket>(socket_));
            socket_.emplace<ws::socket>(std::move(ws));

            auto& sock = std::get<ws::socket>(socket_);
            sock.read_message_max(maximum_);
            sock.set_option(ws::decorator
            {
                [](http::fields& header) NOEXCEPT
                {
                    header.set(http::field::server, BC_HTTP_SERVER_NAME);
                }
            });
            sock.control_callback(std::bind(&socket::do_ws_event,
                shared_from_this(), _1, _2));
            sock.text(true);
            sock.accept(request);
        }

        websocket_.store(true);
        return error::success;
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ accept_websocket: " << e.what());
        return error::operation_failed;
    }
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
