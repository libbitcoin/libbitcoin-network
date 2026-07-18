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
#ifndef LIBBITCOIN_NETWORK_NET_SOCKET_HPP
#define LIBBITCOIN_NETWORK_NET_SOCKET_HPP

#include <atomic>
#include <memory>
#include <optional>
#include <variant>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/deadline.hpp>

namespace libbitcoin {
namespace network {

/// Virtual, thread safe (see comments on accept).
/// Stop is thread safe and idempotent, may be called multiple times.
/// All handlers (except accept) are posted to the internal strand.
class BCT_API socket
  : public std::enable_shared_from_this<socket>, public reporter,
    protected tracker<socket>
{
public:
    typedef std::shared_ptr<socket> ptr;

    // TODO: zmq::context, p2p::context(?).
    using context = std::variant
    <
        std::monostate,
        ref<asio::ssl::context>
    >;

    struct parameters
    {
        using duration = steady_clock::duration;

        duration connect_timeout{};
        size_t maximum_request{};
        socket::context context{};
    };

    /// Construct.
    /// -----------------------------------------------------------------------

    DELETE_COPY_MOVE(socket);

    /// Inbound connections.
    socket(const logger& log, asio::context& service,
        const parameters& params) NOEXCEPT;

    /// Outbound connections.
    /// Endpoint represents the peer or client (non-proxy) that the connector
    /// attempted to reach. Address holds a copy of the tcp address associated
    /// with the connection (or empty).
    socket(const logger& log, asio::context& service, const parameters& params,
        const config::address& address, const config::endpoint& endpoint,
        bool proxied) NOEXCEPT;

    /// Asserts/logs stopped.
    virtual ~socket() NOEXCEPT;

    /// Stop.
    /// -----------------------------------------------------------------------

    /// Cancel work and close the socket (idempotent, thread safe).
    /// This action is deferred to the strand, not immediately affected.
    /// Block on threadpool.join() to ensure termination of the connection.
    virtual void stop() NOEXCEPT;

    /// Same as stop but provides graceful shutdown for websocket connections.
    virtual void lazy_stop() NOEXCEPT;

    /// Wait (all).
    /// -----------------------------------------------------------------------

    /// Wait on a peer close/cancel/send, no data capture/loss.
    virtual void wait(result_handler&& handler) NOEXCEPT;

    /// Cancel wait or any asynchronous read/write operation, handlers posted.
    virtual void cancel(result_handler&& handler) NOEXCEPT;

    /// Connect (all).
    /// -----------------------------------------------------------------------

    /// Accept an incoming connection, handler posted to *acceptor* strand.
    /// Concurrent calls are NOT thread safe until this handler is invoked.
    virtual void accept(asio::acceptor& acceptor,
        result_handler&& handler) NOEXCEPT;

    /// Create an outbound connection, handler posted to socket strand.
    /// Authority will be set to the connected endpoint unless proxied is set.
    virtual void connect(const asio::endpoints& range,
        result_handler&& handler) NOEXCEPT;

    /// WS (generic, framed).
    /// -----------------------------------------------------------------------

    /// Read framed message from websocket, handler posted to socket strand.
    virtual void ws_read(http::flat_buffer& out,
        count_handler&& handler) NOEXCEPT;

    /// Write buffer as frame to websocket, handler posted to socket strand.
    virtual void ws_write(const asio::const_buffer& in, bool binary,
        count_handler&& handler) NOEXCEPT;

    /// TCP (generic, fixed size).
    /// -----------------------------------------------------------------------

    /// Read fixed buffer from tcp socket, handler posted to socket strand.
    virtual void tcp_read(const asio::mutable_buffer& out,
        count_handler&& handler) NOEXCEPT;

    /// Write fixed buffer to socket, handler posted to socket strand.
    virtual void tcp_write(const asio::const_buffer& in,
        count_handler&& handler) NOEXCEPT;

    /// RPC (TCP: electrum/stratum_v1, WS: btcd).
    /// -----------------------------------------------------------------------

    /// Read rpc request from the socket, handler posted to socket strand.
    virtual void rpc_read(http::flat_buffer& buffer, rpc::request& request,
        count_handler&& handler) NOEXCEPT;

    /// Write rpc response to the socket, handler posted to socket strand.
    virtual void rpc_write(rpc::response&& response,
        count_handler&& handler) NOEXCEPT;

    /// Write rpc notification to the socket, handler posted to socket strand.
    virtual void rpc_notify(rpc::request&& notification,
        count_handler&& handler) NOEXCEPT;

    /// BODY (non-http body read/write).
    /// -----------------------------------------------------------------------

    /// Read body request from the socket, handler posted to socket strand.
    virtual void body_read(http::flat_buffer& buffer, http::request& request,
        count_handler&& handler) NOEXCEPT;

    /// Write body response to the socket, handler posted to socket strand.
    virtual void body_write(http::response&& response,
        count_handler&& handler) NOEXCEPT;

    /// Write body notification to the socket, handler posted to socket strand.
    virtual void body_notify(http::request&& notification,
        count_handler&& handler) NOEXCEPT;

    /// HTTP/WS (generic/rpc).
    /// -----------------------------------------------------------------------

    /// Caller-owned parser for progressive (batchable) http reads.
    using http_parser = boost::beast::http::request_parser<http::body>;
    using http_parser_ptr = std::shared_ptr<http_parser>;

    /// Read http request from the socket, handler posted to socket strand.
    virtual void http_read(http::flat_buffer& buffer, http::request& request,
        count_handler&& handler) NOEXCEPT;

    /// Write http response to the socket, handler posted to socket strand.
    virtual void http_write(http::response&& response,
        count_handler&& handler) NOEXCEPT;

    /// HTTP (progressive read/write, for batchable json-rpc bodies).
    /// -----------------------------------------------------------------------

    /// Read http request header from the socket (body remains unread).
    virtual void http_read_header(http::flat_buffer& buffer,
        http_parser& parser, count_handler&& handler) NOEXCEPT;

    /// Read from the http request body (each message pauses the parse).
    virtual void http_read_some(http::flat_buffer& buffer,
        http_parser& parser, count_handler&& handler) NOEXCEPT;

    /// Write http response header to the socket (body remains unwritten).
    virtual void http_write_header(http::response&& response,
        count_handler&& handler) NOEXCEPT;

    /// Write rpc response part to the socket as http chunk data, terminating
    /// the chunked body when the part closes its batch.
    virtual void rpc_write_chunk(rpc::response&& response,
        count_handler&& handler) NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// Get the network threadpool iocontext.
    virtual asio::context& service() const NOEXCEPT;

    /// Get the strand of the socket.
    virtual asio::strand& strand() NOEXCEPT;

    /// The strand is running in this thread.
    virtual bool stranded() const NOEXCEPT;

    /// Stop has been signaled, work is stopping.
    virtual bool stopped() const NOEXCEPT;

    /// The socket was accepted (vs. connected).
    virtual bool inbound() const NOEXCEPT;

    /// The socket upgrades to its secure configuration upon connect.
    virtual bool secure() const NOEXCEPT;

    /// The socket was upgraded to a websocket.
    virtual bool websocket() const NOEXCEPT;

    /// Get the address of the outgoing endpoint passed via construct, or the
    /// resolved endpoint address for incoming connections.
    virtual const config::address& address() const NOEXCEPT;

    /// Get the endpoint of the remote host. Established by connection
    /// resolution for incoming and non-proxied outgoing. For a proxied
    /// connection (outgoing only) this is the value passed via construct.
    virtual const config::endpoint& endpoint() const NOEXCEPT;

protected:
    using ws_t = std::variant<ref<ws::socket>, ref<ws::ssl::socket>>;
    using tcp_t = std::variant<ref<asio::socket>, ref<asio::ssl::socket>>;
    using socket_t = std::variant<asio::socket, asio::ssl::socket, ws::socket,
        ws::ssl::socket>;

    /// Construct.
    /// -----------------------------------------------------------------------
    socket(const logger& log, asio::context& service, const parameters& params,
        const config::address& address, const config::endpoint& endpoint,
        bool proxied, bool inbound) NOEXCEPT;

    /// Context.
    /// -----------------------------------------------------------------------

    /// The socket was upgraded to ssl (requires strand).
    bool is_secure() const NOEXCEPT;

    /// The socket is not upgraded (asio::socket).
    bool is_base() const NOEXCEPT;

    /// Variant accessors (protected by strand).
    /// -----------------------------------------------------------------------
    ws_t get_ws() NOEXCEPT;
    tcp_t get_tcp() NOEXCEPT;
    asio::socket& get_base() NOEXCEPT;
    asio::ssl::socket& get_ssl() NOEXCEPT;

    /// Variant (ws vs. tcp) helpers (protected by strand).
    /// -----------------------------------------------------------------------
    void async_read(http::flat_buffer& buffer,
        const count_handler& handler) NOEXCEPT;
    void async_read(const asio::mutable_buffer& buffer,
        const count_handler& handler) NOEXCEPT;
    void async_read_some(const asio::mutable_buffer& buffer,
        const count_handler& handler) NOEXCEPT;
    void async_write(const asio::const_buffer& buffer, bool binary,
        const count_handler& handler) NOEXCEPT;
    void async_read_http(http::flat_buffer& buffer, http::request& request,
        const count_handler& handler) NOEXCEPT;
    void async_write_http(http::response&& response,
        const count_handler& handler) NOEXCEPT;

private:
    struct read_state
    {
        typedef std::shared_ptr<read_state> ptr;

        read_state(http::request& request, http::flat_buffer& buffer) NOEXCEPT
          : value{ request }, reader{ value.base(), value.body() },
            buffer{ buffer }
        {
        }

        http::request& value;
        http::body::reader reader;
        http::flat_buffer& buffer;
    };

    struct write_state
    {
        typedef std::shared_ptr<write_state> ptr;
        using out_buffer = http::body::writer::out_buffer;

        write_state(http::response&& response) NOEXCEPT
          : value{ std::move(response) }, writer{ value.base(), value.body() }
        {
        }

        bool more{ true };
        http::response value;
        http::body::writer writer;
    };

    struct notify_state
    {
        typedef std::shared_ptr<notify_state> ptr;
        using out_buffer = http::body::writer::out_buffer;

        notify_state(http::request&& request) NOEXCEPT
          : value{ std::move(request) }, writer{ value.base(), value.body() }
        {
        }

        bool more{ true };
        http::request value;
        http::body::writer writer;
    };

    struct header_state
    {
        typedef std::shared_ptr<header_state> ptr;
        using http_serializer =
            boost::beast::http::response_serializer<http::body>;

        header_state(http::response&& response) NOEXCEPT
          : value{ std::move(response) }, serializer{ value }
        {
        }

        http::response value;
        http_serializer serializer;
    };

    struct chunk_state
    {
        typedef std::shared_ptr<chunk_state> ptr;
        using out_buffer = http::body::writer::out_buffer;
        using chunk = boost::beast::http::chunk_body<asio::const_buffer>;

        chunk_state(http::response&& response) NOEXCEPT
          : value{ std::move(response) }, writer{ value.base(), value.body() }
        {
        }

        // The part that closes its batch terminates the chunked body.
        inline bool last() const NOEXCEPT
        {
            const auto& body = value.body();
            return body.contains<rpc::response>() &&
                rpc::closes_batch(body.get<rpc::response>());
        }

        bool more{ true };
        http::response value;
        http::body::writer writer;
        std::optional<chunk> part{};
    };

    // do
    // ------------------------------------------------------------------------

    // stop
    void do_stop() NOEXCEPT;
    void do_ws_stop() NOEXCEPT;
    void do_ssl_stop() NOEXCEPT;

    // config
    void do_ws_event(ws::frame_type kind, const std::string& data) NOEXCEPT;

    // wait
    void do_wait(const result_handler& handler) NOEXCEPT;
    void do_cancel(const result_handler& handler) NOEXCEPT;

    // connection
    void do_connect(const asio::endpoints& range,
        const result_handler& handler) NOEXCEPT;
    void do_handshake(const result_handler& handler) NOEXCEPT;

    // ws (framed)
    void do_ws_read(ref<http::flat_buffer> out,
        const count_handler& handler) NOEXCEPT;
    void do_ws_write(const asio::const_buffer& in, bool binary,
        const count_handler& handler) NOEXCEPT;

    // tcp (fixed)
    void do_tcp_read(const asio::mutable_buffer& out,
        const count_handler& handler) NOEXCEPT;

    // body
    void do_body_read(boost_code ec, size_t total,
        const read_state::ptr& in, const count_handler& handler) NOEXCEPT;
    void do_body_write(boost_code ec, size_t total,
        const write_state::ptr& out, const count_handler& handler) NOEXCEPT;
    void do_body_notify(boost_code ec, size_t total,
        const notify_state::ptr& out, const count_handler& handler) NOEXCEPT;

    // http
    void do_http_read(ref<http::flat_buffer> buffer,
        const ref<http::request>& request,
        const count_handler& handler) NOEXCEPT;
    void do_http_write(const http::response_ptr& response,
        const count_handler& handler) NOEXCEPT;

    // http (progressive)
    void do_http_read_header(ref<http::flat_buffer> buffer,
        const ref<http_parser>& parser, const count_handler& handler) NOEXCEPT;
    void do_http_read_some(ref<http::flat_buffer> buffer,
        const ref<http_parser>& parser, const count_handler& handler) NOEXCEPT;
    void do_http_write_header(const header_state::ptr& out,
        const count_handler& handler) NOEXCEPT;
    void do_chunk_write(boost_code ec, size_t total,
        const chunk_state::ptr& out, const count_handler& handler) NOEXCEPT;

    // handle
    // ------------------------------------------------------------------------

    // stop (lazy)
    void handle_ws_close(const boost_code& ec) NOEXCEPT;
    void handle_ssl_close(const boost_code& ec) NOEXCEPT;

    // config
    void handle_ws_event(ws::frame_type kind,
        const std::string& data) NOEXCEPT;

    // wait
    void handle_wait(const boost_code& ec,
        const result_handler& handler) NOEXCEPT;

    // connect/accept
    void handle_accept(boost_code ec,
        const result_handler& handler) NOEXCEPT;
    void handle_connect(const boost_code& ec, const asio::endpoint& peer,
        const result_handler& handler) NOEXCEPT;
    void handle_handshake(const boost_code& ec,
        const result_handler& handler) NOEXCEPT;

    // read/write (tcp/ws)
    void handle_async(const boost_code& ec, size_t size,
        const count_handler& handler,const std::string& operation) NOEXCEPT;

    // rpc
    void handle_rpc_read(const code& ec, size_t bytes,
        const ref<rpc::request>& out, const http::request_ptr& in,
        const count_handler& handler) NOEXCEPT;

    // body
    void handle_body_read(const code& ec, size_t size, size_t total,
        const read_state::ptr& in, const count_handler& handler) NOEXCEPT;
    void handle_body_write(const code& ec, size_t size, size_t total,
        const write_state::ptr& out, const count_handler& handler) NOEXCEPT;
    void handle_body_notify(const code& ec, size_t size, size_t total,
        const notify_state::ptr& out, const count_handler& handler) NOEXCEPT;

    // http/ws (native/rpc)
    void handle_http_read(const boost_code& ec, size_t size,
        const ref<http::request>& request, const http_parser_ptr& parser,
        const count_handler& handler) NOEXCEPT;
    void handle_http_write(const boost_code& ec, size_t size,
        const http::response_ptr& request,
        const count_handler& handler) NOEXCEPT;

    // http (progressive)
    void handle_http_read_header(const boost_code& ec, size_t size,
        const ref<http_parser>& parser, const count_handler& handler) NOEXCEPT;
    void handle_http_read_some(const boost_code& ec, size_t size,
        const count_handler& handler) NOEXCEPT;
    void handle_http_write_header(const boost_code& ec, size_t size,
        const header_state::ptr& out, const count_handler& handler) NOEXCEPT;
    void handle_chunk_write(const boost_code& ec, size_t size, size_t total,
        const chunk_state::ptr& out, const count_handler& handler) NOEXCEPT;
    void handle_chunk_last(const boost_code& ec, size_t size, size_t total,
        const count_handler& handler) NOEXCEPT;

    // utility
    // ------------------------------------------------------------------------

    code set_websocket(const http::request& request) NOEXCEPT;
    void logx(const std::string& context, const boost_code& ec) const NOEXCEPT;

protected:
    // These are thread safe.
    const bool inbound_;
    const bool proxied_;
    const size_t maximum_;
    asio::strand strand_;
    asio::context& service_;
    const context& context_;
    std::atomic_bool stopped_{};
    std::atomic_bool websocket_{};

    // These are protected by strand (see also handle_accept).
    config::address address_;
    config::endpoint endpoint_;
    deadline::ptr timer_;
    socket_t socket_;
};

typedef std::function<void(const code&, const socket::ptr&)> socket_handler;

// Should not be noexcept.
#define VARIANT_DISPATCH_METHOD(object, method) \
std::visit([&](auto&& value) \
{ \
    value.get().method; \
}, object)

// Should not be noexcept.
#define VARIANT_DISPATCH_FUNCTION(function, object, ...) \
std::visit([&](auto&& value) \
{ \
    function(value.get(), __VA_ARGS__); \
}, object)

} // namespace network
} // namespace libbitcoin

#endif
