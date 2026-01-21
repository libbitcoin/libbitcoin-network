/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

// WS Read.
// ----------------------------------------------------------------------------

void socket::ws_read(http::flat_buffer& out,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_ws_read,
            shared_from_this(), std::ref(out), std::move(handler)));
}

// flat_buffer is copied to allow it to be non-const.
void socket::do_ws_read(ref<http::flat_buffer> out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(is_websocket());

    // Consume full previous buffer (no bytes are left behind by ws read).
    out.get().consume(out.get().size());

    try
    {
        VARIANT_DISPATCH_METHOD(get_ws(),
            async_read(out.get(), std::bind(&socket::handle_ws_read,
                shared_from_this(), _1, _2, handler)));
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ do_ws_read: " << e.what());
        handler(error::operation_failed, {});
    }
}

void socket::handle_ws_read(const boost_code& ec, size_t size,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    const auto code = error::ws_to_error_code(ec);
    if (code == error::unknown) logx("ws-read", ec);
    handler(code, size);
}

// WS Write.
// ----------------------------------------------------------------------------

void socket::ws_write(const asio::const_buffer& in, bool raw,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_ws_write,
            shared_from_this(), in, raw, std::move(handler)));
}

void socket::do_ws_write(const asio::const_buffer& in, bool raw,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(is_websocket());

    try
    {
        if (raw)
        {
            VARIANT_DISPATCH_METHOD(get_ws(), binary(true));
        }
        else
        {
            VARIANT_DISPATCH_METHOD(get_ws(), text(true));
        }

        VARIANT_DISPATCH_METHOD(get_ws(),
            async_write(in, std::bind(&socket::handle_ws_write,
                shared_from_this(), _1, _2, handler)));
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ do_ws_write: " << e.what());
        handler(error::operation_failed, {});
    }
}

void socket::handle_ws_write(const boost_code& ec, size_t size,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    const auto code = error::ws_to_error_code(ec);
    if (code == error::unknown) logx("ws-write", ec);
    handler(code, size);
}

// WS Event.
// ----------------------------------------------------------------------------

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

//  Upgrade.
// ----------------------------------------------------------------------------

// TODO: inject server name from config.
code socket::set_websocket(const http::request& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(!is_websocket());

    try
    {
        if (secure())
        {
            // Upgrade to ws::ssl::socket.
            socket_.emplace<ws::ssl::socket>(
                std::move(std::get<asio::ssl::socket>(socket_)));

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
            sock.binary(true);
            sock.accept(request);
        }
        else
        {
            // Upgrade to ws::socket.
            socket_.emplace<ws::socket>(
                std::move(std::get<asio::socket>(socket_)));

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
            sock.binary(true);
            sock.accept(request);
        }

        return error::upgraded;
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ set_websocket: " << e.what());
        return error::operation_failed;
    }
}

void socket::do_ws_event(ws::frame_type kind,
    const std::string_view& data) NOEXCEPT
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

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
