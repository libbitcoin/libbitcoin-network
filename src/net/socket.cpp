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

#include <memory>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Boost: "The execution context provides the I/O executor that the socket will
// use, by default, to dispatch handlers for any asynchronous operations
// performed on the socket." Calls are stranded to protect the socket member.

// Construction.
// ----------------------------------------------------------------------------

// authority_.port() zero implies inbound connection.
socket::socket(const logger& log, asio::io_context& service) NOEXCEPT
  : socket(log, service, config::address{})
{
}

// authority_.port() nonzero implies outbound connection.
socket::socket(const logger& log, asio::io_context& service,
    const config::address& address) NOEXCEPT
  : strand_(service.get_executor()),
    socket_(strand_),
    address_(address),
    reporter(log),
    tracker<socket>(log)
{
}

socket::~socket() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "socket is not stopped");
    if (!stopped_.load()) { LOGF("~socket is not stopped."); }
    if (!websocket_.has_value()) { LOGF("~socket websocket is not reset."); }
}

// Stop.
// ----------------------------------------------------------------------------
// The socket is not allowed to stop itself (internally).

void socket::stop() NOEXCEPT
{
    if (stopped_.load())
        return;

    // Stop flag can accelerate work stoppage, as it does not wait on strand.
    stopped_.store(true);

    // Stop is posted to strand to protect the socket.
    boost::asio::post(strand_,
        std::bind(&socket::do_stop, shared_from_this()));
}

// private
void socket::do_stop() NOEXCEPT
{
    BC_ASSERT(stranded());

    if (!websocket())
    {
        handle_async_close();
        return;
    }

    using namespace boost::beast;
    websocket_->async_close(websocket::close_code::normal,
        std::bind(&socket::handle_async_close,
            shared_from_this()));
}

void socket::handle_async_close() NOEXCEPT
{
    BC_ASSERT(stranded());

    boost_code ignore{};
    auto& socket = get_transport();

    // Disable future sends or receives on the socket, for graceful close.
    socket.shutdown(asio::socket::shutdown_both, ignore);

    // Cancel asynchronous I/O operations and close socket.
    // The underlying descriptor is closed regardless of error return.
    // Any asynchronous send, receive or connect operations are canceled
    // immediately, and will complete with the operation_aborted error.
    socket.close(ignore);

    // Discard the optional.
    websocket_.reset();
}

asio::socket& socket::get_transport() NOEXCEPT
{
    BC_ASSERT(stranded());

    using namespace boost::beast;
    return websocket() ? get_lowest_layer(*websocket_) : socket_;
}

// Connection.
// ----------------------------------------------------------------------------
// Boost async functions are NOT THREAD SAFE for the same socket object.
// This clarifies boost documentation: svn.boost.org/trac10/ticket/10009

void socket::accept(asio::acceptor& acceptor,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(!websocket_.has_value(), "socket is upgraded");
    BC_ASSERT_MSG(!socket_.is_open(), "accept on open socket");

    // Closure of the acceptor, not the socket, releases this handler.
    // The socket is not guarded during async_accept. This is required so the
    // acceptor may be guarded from its own strand while preserving hiding of
    // socket internals. This makes concurrent calls unsafe, however only the
    // acceptor (a socket factory) requires access to the socket at this time.
    // network::acceptor both invokes this call in the network strand and
    // initializes the asio::acceptor with the network strand. So the call to
    // acceptor.async_accept invokes its handler on that strand as well.
    try
    {
        // Dispatches on the acceptor's strand (which should be network).
        acceptor.async_accept(socket_,
            std::bind(&socket::handle_accept,
                shared_from_this(), _1, handler));
    }
    catch (const std::exception& LOG_ONLY(e))
    {
        LOGF("Exception @ accept: " << e.what());
        handler(error::accept_failed);
    }
}

void socket::connect(const asio::endpoints& range,
    result_handler&& handler) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&socket::do_connect,
            shared_from_this(), range, std::move(handler)));
}

// TCP.
// ----------------------------------------------------------------------------

void socket::read(const asio::mutable_buffer& out,
    count_handler&& handler) NOEXCEPT
{
    // asio::mutable_buffer is essentially a data_slab.
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_read,
            shared_from_this(), out, std::move(handler)));
}

void socket::write(const asio::const_buffer& in,
    count_handler&& handler) NOEXCEPT
{
    // asio::const_buffer is essentially a data_slice.
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_write,
            shared_from_this(), in, std::move(handler)));
}

// HTTP.
// ----------------------------------------------------------------------------

void socket::http_read(http::flat_buffer& buffer,
    http::request& request, count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_http_read, shared_from_this(), std::ref(buffer),
            std::ref(request), std::move(handler)));
}

void socket::http_write(http::response& response,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_http_write, shared_from_this(),
            std::ref(response), std::move(handler)));
}

// WS.
// ----------------------------------------------------------------------------

void socket::ws_read(http::flat_buffer& out,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_ws_read,
            shared_from_this(), std::ref(out), std::move(handler)));
}

void socket::ws_write(const asio::const_buffer& in, bool binary,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_ws_write,
            shared_from_this(), in, binary, std::move(handler)));
}

// connect (private).
// ----------------------------------------------------------------------------
// These execute on the strand to protect the member socket.

void socket::do_connect(const asio::endpoints& range,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT_MSG(!websocket(), "socket is upgraded");
    BC_ASSERT_MSG(!socket_.is_open(), "connect on open socket");

    try
    {
        // Establishes a socket connection by trying each endpoint in sequence.
        boost::asio::async_connect(socket_, range,
            std::bind(&socket::handle_connect,
                shared_from_this(), _1, _2, handler));
    }
    catch (const std::exception& LOG_ONLY(e))
    {
        LOGF("Exception @ do_connect: " << e.what());
        handler(error::connect_failed);
    }
}

// tcp (private).
// ----------------------------------------------------------------------------
// These execute on the strand to protect the member socket.

void socket::do_read(const asio::mutable_buffer& out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (websocket())
    {
        handler(error::service_stopped, {});
        return;
    }

    try
    {
        // This composed operation posts all intermediate handlers to the strand.
        boost::asio::async_read(socket_, out,
            std::bind(&socket::handle_io,
                shared_from_this(), _1, _2, handler));
    }
    catch (const std::exception& LOG_ONLY(e))
    {
        LOGF("Exception @ do_read: " << e.what());
        handler(error::operation_failed, {});
    }
}

void socket::do_write(const asio::const_buffer& in,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (websocket())
    {
        handler(error::service_stopped, {});
        return;
    }

    try
    {
        // This composed operation posts all intermediate handlers to the strand.
        boost::asio::async_write(socket_, in,
            std::bind(&socket::handle_io,
                shared_from_this(), _1, _2, handler));
    }
    catch (const std::exception& LOG_ONLY(e))
    {
        LOGF("Exception @ do_write: " << e.what());
        handler(error::operation_failed, {});
    }
}

// http (private).
// ----------------------------------------------------------------------------

void socket::do_http_read(std::reference_wrapper<http::flat_buffer> buffer,
    const std::reference_wrapper<http::request>& request,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (websocket())
    {
        handler(error::service_stopped, {});
        return;
    }

    try
    {
        // This operation posts handler to the strand.
        boost::beast::http::async_read(socket_, buffer.get(), request.get(),
            std::bind(&socket::handle_http_read,
                shared_from_this(), _1, _2, buffer, handler));
    }
    catch (const std::exception& LOG_ONLY(e))
    {
        LOGF("Exception @ do_http_read: " << e.what());
        handler(error::operation_failed, {});
    }
}

void socket::do_http_write(
    const std::reference_wrapper<http::response>& response,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (websocket())
    {
        handler(error::service_stopped, {});
        return;
    }

    try
    {
        // This operation posts handler to the strand.
        boost::beast::http::async_write(socket_, response.get(),
            std::bind(&socket::handle_http_write,
                shared_from_this(), _1, _2, handler));
    }
    catch (const std::exception& LOG_ONLY(e))
    {
        LOGF("Exception @ do_http_write: " << e.what());
        handler(error::operation_failed, {});
    }
}

// ws (private).
// ----------------------------------------------------------------------------

// flat_buffer is copied to allow it to be non-const.
void socket::do_ws_read(std::reference_wrapper<http::flat_buffer> out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(websocket());

    try
    {
        websocket_->async_read(out.get(),
            std::bind(&socket::handle_ws_read,
                shared_from_this(), _1, _2, handler));
    }
    catch (const std::exception& LOG_ONLY(e))
    {
        LOGF("Exception @ do_ws_read: " << e.what());
        handler(error::operation_failed, {});
    }
}

void socket::do_ws_write(const asio::const_buffer& in, bool binary,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(websocket());

    try
    {
        if (binary)
            websocket_->binary(true);
        else
            websocket_->text(true);

        websocket_->async_write(in,
            std::bind(&socket::handle_ws_write,
                shared_from_this(), _1, _2, handler));
    }
    catch (const std::exception& LOG_ONLY(e))
    {
        LOGF("Exception @ do_ws_write: " << e.what());
        handler(error::operation_failed, {});
    }
}

void socket::do_ws_event(ws::frame_type kind,
    const std::string_view& data) NOEXCEPT
{
    // Takes ownership of the string.
    boost::asio::dispatch(strand_,
        std::bind(&socket::handle_ws_event,
            shared_from_this(), kind, std::string{ data }));
}

// completion (private).
// ----------------------------------------------------------------------------
// These are invoked on strand upon failure, socket cancel, or completion.

void socket::handle_accept(const boost_code& ec,
    const result_handler& handler) NOEXCEPT
{
    // This is running in the acceptor (not socket) execution context.
    // socket_ and authority_ are not guarded here, see comments on accept.
    // address_ remains defaulted for inbound (accepted) connections.

    if (!ec)
        authority_ = { socket_.remote_endpoint() };

    if (error::asio_is_canceled(ec))
    {
        handler(error::operation_canceled);
        return;
    }

    const auto code = error::asio_to_error_code(ec);
    if (code == error::unknown)
    {
        LOGX("Raw accept code (" << ec.value() << ") " << ec.category().name()
            << ":" << ec.message());
    }

    handler(code);
}

void socket::handle_connect(const boost_code& ec,
    const asio::endpoint& peer, const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    authority_ = peer;

    // Outgoing connection requires address_ for .inbound() resolution.
    if (is_zero(address_.port()))
        address_ = { peer };

    if (error::asio_is_canceled(ec))
    {
        handler(error::operation_canceled);
        return;
    }

    const auto code = error::asio_to_error_code(ec);
    if (code == error::unknown)
    {
        LOGX("Raw connect code (" << ec.value() << ") " << ec.category().name()
            << ":" << ec.message());
    }

    handler(code);
}

void socket::handle_io(const boost_code& ec, size_t size,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    const auto code = error::asio_to_error_code(ec);
    if (code == error::unknown)
    {
        LOGX("Raw io code (" << ec.value() << ") " << ec.category().name()
            << ":" << ec.message());
    }

    handler(code, size);
}

void socket::handle_http_read(const boost_code& ec, size_t size,
    std::reference_wrapper<http::flat_buffer> buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Always consume all of the buffer as the request is fully read.
    buffer.get().consume(buffer.get().size());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    const auto code = error::beast_to_error_code(ec);
    if (code == error::unknown)
    {
        LOGX("Raw beast code (" << ec.value() << ") " << ec.category().name()
            << ":" << ec.message());
    }

    handler(code, size);
}

void socket::handle_http_write(const boost_code& ec, size_t size,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    const auto code = error::beast_to_error_code(ec);
    if (code == error::unknown)
    {
        LOGX("Raw beast code (" << ec.value() << ") " << ec.category().name()
            << ":" << ec.message());
    }

    handler(code, size);
}

// ws
void socket::handle_ws_read(const boost_code& ec, size_t size,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(websocket());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    const auto code = error::beast_to_error_code(ec);
    if (code == error::unknown)
    {
        LOGX("Raw beast code (" << ec.value() << ") " << ec.category().name()
            << ":" << ec.message());
    }

    handler(code, size /*, websocket_->got_binary() */);
}

void socket::handle_ws_write(const boost_code& ec, size_t size,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(websocket());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    const auto code = error::beast_to_error_code(ec);
    if (code == error::unknown)
    {
        LOGX("Raw beast code (" << ec.value() << ") " << ec.category().name()
            << ":" << ec.message());
    }

    handler(code, size /*, websocket_->got_binary() */);
}

void socket::handle_ws_event(ws::frame_type kind,
    const std::string& data) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(websocket());

    switch (kind)
    {
        case ws::frame_type::ping:
            LOGX("WS ping [" << authority() << "] size: " << data.size());
            break;
        case ws::frame_type::pong:
            LOGX("WS pong [" << authority() << "] size: " << data.size());
            break;
        case ws::frame_type::close:
            LOGX("WS close [" << authority() << "] " << websocket_->reason());
            stop();
            break;
    }
}

// Properties.
// ----------------------------------------------------------------------------

const config::authority& socket::authority() const NOEXCEPT
{
    return authority_;
}

const config::address& socket::address() const NOEXCEPT
{
    return address_;
}

bool socket::inbound() const NOEXCEPT
{
    // Relies on construction and address default port of zero.
    return is_zero(address_.port());
}

bool socket::stopped() const NOEXCEPT
{
    return stopped_.load();
}

// Strand properties.
// ----------------------------------------------------------------------------

bool socket::stranded() const NOEXCEPT
{
    return strand_.running_in_this_thread();
}

asio::strand& socket::strand() NOEXCEPT
{
    return strand_;
}

// Websocket properties.
// ----------------------------------------------------------------------------

// protected (requires strand)
bool socket::websocket() const NOEXCEPT
{
    BC_ASSERT(stranded());
    return websocket_.has_value();
}

void socket::set_websocket(const http::request_cptr& request) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_set_websocket,
            shared_from_this(), request));
}

void socket::do_set_websocket(const http::request_cptr& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(!websocket());

    websocket_.emplace(std::move(socket_));
    websocket_->set_option(ws::decorator{[](http::fields& header) NOEXCEPT
    {
        // Customize the response header.
        header.set(http::field::server, "libbitcoin/4.0");
    }});
    websocket_->accept(*request);
    websocket_->binary(true);

    // Handle ping, pong, close.
    websocket_->control_callback(std::bind(&socket::do_ws_event,
        shared_from_this(), _1, _2));
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
