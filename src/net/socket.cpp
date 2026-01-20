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
#include <variant>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {
    
using namespace system;
using namespace network::rpc;
using namespace std::placeholders;
namespace beast = boost::beast;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Boost: "The execution context provides the I/O executor that the socket will
// use, by default, to dispatch handlers for any asynchronous operations
// performed on the socket." Calls are stranded to protect the socket member.

// Construction.
// ----------------------------------------------------------------------------

socket::socket(const logger& log, asio::context& service,
    secure_context& context, size_t maximum) NOEXCEPT
  : socket(log, service, context, maximum, {}, {}, false, true)
{
}

socket::socket(const logger& log, asio::context& service,
    secure_context& context, size_t maximum,
    const config::address& address, const config::endpoint& endpoint,
    bool proxied) NOEXCEPT
  : socket(log, service, context, maximum, address, endpoint, proxied, false)
{
}

// protected
socket::socket(const logger& log, asio::context& service,
    secure_context& context, size_t maximum,
    const config::address& address, const config::endpoint& endpoint,
    bool proxied, bool inbound) NOEXCEPT
  : inbound_(inbound),
    proxied_(proxied),
    maximum_(maximum),
    strand_(service.get_executor()),
    service_(service),
    context_(context),
    address_(address),
    endpoint_(endpoint),
    transport_(std::in_place_type<asio::socket>, strand_),
    reporter(log),
    tracker<socket>(log)
{
}

socket::~socket() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "socket is not stopped");
    if (!stopped_.load()) { LOGF("~socket is not stopped."); }
}

// Stop.
// ----------------------------------------------------------------------------
// The socket does not (must not) stop itself.

// Immediate stop (no graceful websocket closing).
void socket::stop() NOEXCEPT
{
    if (stopped_.load())
        return;

    // Stop flag accelerates work stoppage, as it does not wait on strand.
    stopped_.store(true);

    // Stop is posted to strand to protect the socket.
    boost::asio::post(strand_,
        std::bind(&socket::do_stop, shared_from_this()));
}

// private
void socket::do_stop() NOEXCEPT
{
    BC_ASSERT(stranded());

    // Release the callback closure before shutdown/close.
    if (websocket())
    {
        std::visit([](auto&& ref) NOEXCEPT
        {
            ref.get().control_callback();
        }, get_ws());
    }

    auto tcp_close = [this]() NOEXCEPT
    {
        boost_code ignore{};
        auto& layer = get_lowest_layer();
        layer.shutdown(asio::socket::shutdown_both, ignore);
        layer.close(ignore);
    };

    auto ssl_close = [this, tcp_close](const boost_code& ec) NOEXCEPT
    {
        if (ec && ec != boost::asio::error::eof)
            logx("ssl_shutdown", ec);

        tcp_close();
    };

    if (secure())
    {
        std::get<asio::ssl::socket>(transport_).async_shutdown(ssl_close);
        return;
    }

    tcp_close();
}

// Lazy stop (graceful websocket closing).
void socket::async_stop() NOEXCEPT
{
    if (stopped_.load())
        return;

    // Stop flag accelerates work stoppage, as it does not wait on strand.
    stopped_.store(true);

    // Async stop is dispatched to strand to protect the socket.
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_async_stop, shared_from_this()));
}

void socket::do_async_stop() NOEXCEPT
{
    BC_ASSERT(stranded());

    if (!websocket())
    {
        do_stop();
        return;
    }

    auto async_close = [this](const boost_code& ec) NOEXCEPT
    {
        if (ec)
        {
            logx("ws_close", ec);
        }
        do_stop();
    };

    std::visit([&](auto&& ref) NOEXCEPT
    {
        ref.get().async_close(beast::websocket::close_code::normal,
            async_close);
    }, get_ws());
}

// Wait.
// ----------------------------------------------------------------------------

void socket::wait(result_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_wait,
            shared_from_this(), std::move(handler)));
}

// private
void socket::do_wait(const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    get_lowest_layer().async_wait(asio::socket::wait_read,
        std::bind(&socket::handle_wait,
            shared_from_this(), _1, handler));
}

// private
void socket::handle_wait(const boost_code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Only wait cancel results in caller not calling stop.
    if (error::asio_is_canceled(ec))
    {
        handler(error::success);
        return;
    }

    if (ec)
    {
        logx("wait", ec);
        handler(error::asio_to_error_code(ec));
        return;
    }

    handler(error::operation_canceled);
}

void socket::cancel(result_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_cancel,
            shared_from_this(), std::move(handler)));
}

// private
void socket::do_cancel(const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped())
    {
        handler(error::success);
        return;
    }

    try
    {
        // Causes connect, send, and receive calls to quit with
        // asio::error::operation_aborted passed to handlers.
        get_lowest_layer().cancel();
        handler(error::success);
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ do_cancel: " << e.what());
        handler(error::service_stopped);
    }
}

// Connection.
// ----------------------------------------------------------------------------
// Boost async functions are NOT THREAD SAFE for the same socket object.
// This clarifies boost documentation: svn.boost.org/trac10/ticket/10009

void socket::accept(asio::acceptor& acceptor,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(!std::get<asio::socket>(transport_).is_open(),
        "accept on open socket");

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
        // Cannot move handler due to catch block invocation.
        acceptor.async_accept(std::get<asio::socket>(transport_),
            std::bind(&socket::handle_accept,
                shared_from_this(), _1, handler));
    }
    catch (const std::exception& e)
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

/// TCP-RPC.
// ----------------------------------------------------------------------------

// Buffer could be passed via request, so this is for interface consistency.
void socket::rpc_read(http::flat_buffer& buffer, rpc::request& request,
    count_handler&& handler) NOEXCEPT
{
    boost_code ec{};
    const auto in = emplace_shared<read_rpc>(request);
    in->value.buffer = emplace_shared<http::flat_buffer>(buffer);
    in->reader.init({}, ec);

    boost::asio::dispatch(strand_,
        std::bind(&socket::do_rpc_read,
            shared_from_this(), ec, zero, in, std::move(handler)));
}

void socket::rpc_write(rpc::response& response,
    count_handler&& handler) NOEXCEPT
{
    boost_code ec{};
    const auto out = emplace_shared<write_rpc>(response);
    out->writer.init(ec);

    // Dispatch success or fail, for handler invoke on strand.
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_rpc_write,
            shared_from_this(), ec, zero, out, std::move(handler)));
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

// HTTP.
// ----------------------------------------------------------------------------

void socket::http_read(http::flat_buffer& buffer,
    http::request& request, count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_http_read, shared_from_this(),
            std::ref(buffer), std::ref(request), std::move(handler)));
}

void socket::http_write(http::response& response,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_http_write, shared_from_this(),
            std::ref(response), std::move(handler)));
}

// connect (private).
// ----------------------------------------------------------------------------

void socket::do_connect(const asio::endpoints& range,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT_MSG(!websocket(), "socket is upgraded");
    BC_ASSERT_MSG(!std::get<asio::socket>(transport_).is_open(),
        "connect on open socket");

    try
    {
        // Establishes a socket connection by trying each endpoint in sequence.
        boost::asio::async_connect(std::get<asio::socket>(transport_), range,
            std::bind(&socket::handle_connect,
                shared_from_this(), _1, _2, handler));
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ do_connect: " << e.what());
        handler(error::connect_failed);
    }
}

// tcp (generic).
// ----------------------------------------------------------------------------

void socket::do_read(const asio::mutable_buffer& out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    try
    {
        std::visit([&](auto&& ref) NOEXCEPT
        {
            // This composed operation posts all intermediate handlers to strand.
            boost::asio::async_read(ref.get(), out,
                std::bind(&socket::handle_tcp,
                    shared_from_this(), _1, _2, handler));
        }, get_tcp());
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ do_read: " << e.what());
        handler(error::operation_failed, {});
    }
}

void socket::do_write(const asio::const_buffer& in,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    try
    {
        std::visit([&](auto&& ref) NOEXCEPT
        {
            // This composed operation posts all intermediate handlers to strand.
            boost::asio::async_write(ref.get(), in,
                std::bind(&socket::handle_tcp,
                    shared_from_this(), _1, _2, handler));
        }, get_tcp());
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ do_write: " << e.what());
        handler(error::operation_failed, {});
    }
}

// tcp (rpc).
// ----------------------------------------------------------------------------

void socket::do_rpc_read(boost_code ec, size_t total, const read_rpc::ptr& in,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    constexpr auto size = rpc::writer::default_buffer;

    if (ec)
    {
        // Json parser emits http and json codes.
        const auto code = error::http_to_error_code(ec);
        if (code == error::unknown) logx("rpc-read", ec);
        handler(code, total);
        return;
    }

    std::visit([&](auto&& ref) NOEXCEPT
    {
        ref.get().async_read_some(in->value.buffer->prepare(size),
            std::bind(&socket::handle_rpc_read,
                shared_from_this(), _1, _2, total, in, handler));
    }, get_tcp());
}

void socket::do_rpc_write(boost_code ec, size_t total,
    const write_rpc::ptr& out, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    const auto buffer = ec ? write_rpc::out_buffer{} : out->writer.get(ec);
    if (ec)
    {
        // Json serializer emits http and json codes.
        const auto code = error::http_to_error_code(ec);
        if (code == error::unknown) logx("rpc-write", ec);
        handler(code, total);
        return;
    }

    // Finished.
    if (!buffer->second)
    {
        handler(error::success, total);
        return;
    }

    std::visit([&](auto&& ref) NOEXCEPT
    {
        ref.get().async_write_some(buffer->first,
            std::bind(&socket::handle_rpc_write,
                shared_from_this(), _1, _2, total, out, handler));
    }, get_tcp());
}

// ws (generic).
// ----------------------------------------------------------------------------

// flat_buffer is copied to allow it to be non-const.
void socket::do_ws_read(std::reference_wrapper<http::flat_buffer> out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(websocket());

    // Consume full previous buffer (no bytes are left behind by ws read).
    out.get().consume(out.get().size());

    try
    {
        std::visit([&](auto&& ref) NOEXCEPT
        {
            ref.get().async_read(out.get(),
                std::bind(&socket::handle_ws_read,
                    shared_from_this(), _1, _2, handler));
        }, get_ws());
    }
    catch (const std::exception& e)
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
        std::visit([&](auto&& ref) NOEXCEPT
        {
            auto& sock = ref.get();
            if (binary)
                sock.binary(true);
            else
                sock.text(true);

            sock.async_write(in,
                std::bind(&socket::handle_ws_write,
                    shared_from_this(), _1, _2, handler));
        }, get_ws());
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ do_ws_write: " << e.what());
        handler(error::operation_failed, {});
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

// http (generic).
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
        // Explicit parser override gives access to limits.
        auto parser = to_shared<http_parser>();

        // Causes http::error::body_limit on completion.
        parser->body_limit(maximum_);

        // Causes http::error::header_limit on completion.
        parser->header_limit(limit<uint32_t>(maximum_));

        std::visit([&](auto&& ref) NOEXCEPT
        {
            // This operation posts handler to the strand.
            beast::http::async_read(ref.get(), buffer.get(), *parser,
                std::bind(&socket::handle_http_read,
                    shared_from_this(), _1, _2, request, parser, handler));
        }, get_tcp());
    }
    catch (const std::exception& e)
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
        std::visit([&](auto&& ref) NOEXCEPT
        {
            // This operation posts handler to the strand.
            beast::http::async_write(ref.get(), response.get(),
                std::bind(&socket::handle_http_write,
                    shared_from_this(), _1, _2, handler));
        }, get_tcp());
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ do_http_write: " << e.what());
        handler(error::operation_failed, {});
    }
}

// completion (private).
// ----------------------------------------------------------------------------
// These are invoked on strand upon failure, socket cancel, or completion.

void socket::handle_accept(boost_code ec,
    const result_handler& handler) NOEXCEPT
{
    // This is running in the acceptor (not socket) execution context.
    // socket_ and endpoint_ are not guarded here, see comments on accept.
    if (!ec)
    {
        const auto& sock = std::get<asio::socket>(transport_);
        endpoint_ = { sock.remote_endpoint(ec) };
    }

    // TODO: conflated with ws::ssl.
    if (secure())
    {
        if (ec)
        {
            handler(error::asio_to_error_code(ec));
            return;
        }

        auto& plain = std::get<asio::socket>(transport_);
        transport_.emplace<asio::ssl::socket>(std::move(plain),
            std::get<asio::ssl::context>(context_));

        std::get<asio::ssl::socket>(transport_)
            .async_handshake(boost::asio::ssl::stream_base::server,
                std::bind(&socket::handle_handshake,
                    shared_from_this(), _1, handler));
        return;
    }

    if (error::asio_is_canceled(ec))
    {
        handler(error::operation_canceled);
        return;
    }

    const auto code = error::asio_to_error_code(ec);
    if (code == error::unknown) logx("accept", ec);
    handler(code);
}

void socket::handle_connect(const boost_code& ec, const asio::endpoint& peer,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // For socks proxy, peer will be the server's local binding.
    if (!proxied_)
        endpoint_ = peer;

    // TODO: conflated with ws::ssl.
    if (secure())
    {
        if (ec)
        {
            handler(error::asio_to_error_code(ec));
            return;
        }

        auto& plain = std::get<asio::socket>(transport_);
        transport_.emplace<asio::ssl::socket>(std::move(plain),
            std::get<asio::ssl::context>(context_));

        std::get<asio::ssl::socket>(transport_)
            .async_handshake(boost::asio::ssl::stream_base::client,
                std::bind(&socket::handle_handshake,
                    shared_from_this(), _1, handler));
        return;
    }

    if (error::asio_is_canceled(ec))
    {
        handler(error::operation_canceled);
        return;
    }

    const auto code = error::asio_to_error_code(ec);
    if (code == error::unknown) logx("connect", ec);
    handler(code);
}

void socket::handle_handshake(const boost_code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::operation_canceled);
        return;
    }

    const auto code = error::asio_to_error_code(ec);
    if (code == error::unknown) logx("handshake", ec);
    handler(code);
}

// tcp (generic)
// ----------------------------------------------------------------------------

void socket::handle_tcp(const boost_code& ec, size_t size,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    const auto code = error::asio_to_error_code(ec);
    if (code == error::unknown) logx("tcp", ec);
    handler(code, size);
}

// tcp (rpc)
// ----------------------------------------------------------------------------

void socket::handle_rpc_read(boost_code ec, size_t size, size_t total,
    const read_rpc::ptr& in, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    total = ceilinged_add(total, size);
    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, total);
        return;
    }

    if (total > maximum_)
    {
        handler(error::message_overflow, total);
        return;
    }

    if (!ec)
    {
        in->value.buffer->commit(size);
        const auto data = in->value.buffer->data();
        const auto parsed = in->reader.put(data, ec);
        if (!ec)
        {
            in->value.buffer->consume(parsed);
            if (in->reader.done())
            {
                in->reader.finish(ec);
                if (!ec)
                {
                    handler(error::success, total);
                    return;
                }
            }
        }
    }

    do_rpc_read(ec, total, in, handler);
}

void socket::handle_rpc_write(boost_code ec, size_t size, size_t total,
    const write_rpc::ptr& out, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    total = ceilinged_add(total, size);
    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, total);
        return;
    }

    do_rpc_write(ec, total, out, handler);
}

// ws (generic)
// ----------------------------------------------------------------------------

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
            std::visit([&](auto&& ref) NOEXCEPT
            {
                LOGX("WS close [" << endpoint() << "] " << ref.get().reason());
            }, get_ws());
            break;
    }
}

// http (generic)
// ----------------------------------------------------------------------------

void socket::handle_http_read(const boost_code& ec, size_t size,
    const std::reference_wrapper<http::request>& request,
    const http_parser_ptr& parser, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    if (!ec && beast::websocket::is_upgrade(parser->get()))
    {
        handler(set_websocket(parser->get()), size);
        return;
    }

    if (!ec)
    {
        request.get() = parser->release();
    }

    const auto code = error::http_to_error_code(ec);
    if (code == error::unknown) logx("http", ec);
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

    const auto code = error::http_to_error_code(ec);
    if (code == error::unknown) logx("http", ec);
    handler(code, size);
}

// Properties.
// ----------------------------------------------------------------------------

const config::address& socket::address() const NOEXCEPT
{
    return address_;
}

const config::endpoint& socket::endpoint() const NOEXCEPT
{
    return endpoint_;
}

bool socket::inbound() const NOEXCEPT
{
    return inbound_;
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

asio::context& socket::service() const NOEXCEPT
{
    return service_;
}

// Websocket properties.
// ----------------------------------------------------------------------------

// protected (requires strand)
bool socket::websocket() const NOEXCEPT
{
    BC_ASSERT(stranded());
    return std::holds_alternative<ws::socket>(transport_) ||
           std::holds_alternative<ws::ssl::socket>(transport_);
}

// protected (requires strand)
bool socket::secure() const NOEXCEPT
{
    BC_ASSERT(stranded());
    return std::holds_alternative<asio::ssl::socket>(transport_) ||
           std::holds_alternative<ws::ssl::socket>(transport_);
}

code socket::set_websocket(const http::request& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(!websocket());

    try
    {
        // TODO: conflated with asio::ssl.
        if (secure())
        {
            auto& tcp = std::get<asio::ssl::socket>(transport_);
            transport_.emplace<ws::ssl::socket>(std::move(tcp));
            auto& sock = std::get<ws::ssl::socket>(transport_);
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
            auto& tcp = std::get<asio::socket>(transport_);
            transport_.emplace<ws::socket>(std::move(tcp));
            auto& sock = std::get<ws::socket>(transport_);
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

// Variant helpers.
// ----------------------------------------------------------------------------

socket::ws_variant socket::get_ws() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(websocket());

    if (secure())
        return std::ref(std::get<ws::ssl::socket>(transport_));
    else
        return std::ref(std::get<ws::socket>(transport_));
}

socket::tcp_variant socket::get_tcp() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(!websocket());

    if (secure())
        return std::ref(std::get<asio::ssl::socket>(transport_));
    else
        return std::ref(std::get<asio::socket>(transport_));
}

asio::socket& socket::get_lowest_layer() NOEXCEPT
{
    BC_ASSERT(stranded());

    return std::visit(overload
    {
        [](asio::socket& value) NOEXCEPT -> asio::socket&
        {
            return value;
        },
        [](asio::ssl::socket& value) NOEXCEPT -> asio::socket&
        {
            return beast::get_lowest_layer(value);
        },
        [](ws::socket& value) NOEXCEPT -> asio::socket&
        {
            return beast::get_lowest_layer(value);
        },
        [](ws::ssl::socket& value) NOEXCEPT -> asio::socket&
        {
            return beast::get_lowest_layer(value);
        }
    }, transport_);
}

void socket::logx(const std::string& context,
    const boost_code& ec) const NOEXCEPT
{
    LOGX("Socket " << context << " error (" << ec.value() << ") "
        << ec.category().name() << " : " << ec.message());
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
