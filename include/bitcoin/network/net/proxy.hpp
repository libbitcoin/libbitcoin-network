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
#ifndef LIBBITCOIN_NETWORK_NET_PROXY_HPP
#define LIBBITCOIN_NETWORK_NET_PROXY_HPP

#include <atomic>
#include <deque>
#include <memory>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/memory.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/socket.hpp>

namespace libbitcoin {
namespace network {

/// Abstract, thread safe except some methods requiring strand.
/// Handles all channel communication, error handling, and logging.
/// Caller must retain ownership of read/write buffers until handler invoked.
/// Full duplex writes (non-http) are queued to prevent interleaving.
/// Completion handler is invoked once write is complete, at which point the
/// next queued write is invoked. When a channel stops with pending writes the
/// write queue is purged without invoke of the purged handlers.
class BCT_API proxy
  : public enable_shared_from_base<proxy>, public reporter
{
public:
    typedef std::shared_ptr<proxy> ptr;
    typedef subscriber<> stop_subscriber;

    DELETE_COPY_MOVE(proxy);

    /// Asserts/logs stopped.
    virtual ~proxy() NOEXCEPT;

    /// Stop.
    /// -----------------------------------------------------------------------

    /// Idempotent, may be called multiple times.
    /// Stop socket, no delay, called by stop notify when iocontext is closing.
    /// An open batch response is closed (written) before the socket stops.
    virtual void stop(const code& ec) NOEXCEPT;

    /// Subscribe to stop notification with completion handler.
    /// Completion and event handlers are always invoked on the channel strand.
    void subscribe_stop(result_handler&& handler,
        result_handler&& complete) NOEXCEPT;

    /// Pause.
    /// -----------------------------------------------------------------------

    /// Pause reading from the socket (requires strand).
    virtual void pause() NOEXCEPT;

    /// Resume reading from the socket (requires strand).
    virtual void resume() NOEXCEPT;

    /// Reading from the socket is paused (requires strand).
    virtual bool paused() const NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// Get the network threadpool iocontext.
    asio::context& service() const NOEXCEPT;

    /// The channel strand.
    asio::strand& strand() NOEXCEPT;

    /// The strand is running in this thread.
    bool stranded() const NOEXCEPT;

    /// The proxy (socket) is stopped.
    bool stopped() const NOEXCEPT;

    /// The socket was accepted (vs. connected).
    bool inbound() const NOEXCEPT;

    /// Connection is currently secured (TLS or comparable for socket type).
    bool secure() const NOEXCEPT;

    /// The socket was upgraded to a websocket.
    bool websocket() const NOEXCEPT;

    /// The total number of bytes queued/sent to the remote endpoint.
    uint64_t total() const NOEXCEPT;

    /// Get the address of the outgoing endpoint passed via construct.
    const config::address& address() const NOEXCEPT;

    /// Get the endpoint of the remote host.
    const config::endpoint& endpoint() const NOEXCEPT;

protected:
    proxy(const socket::ptr& socket) NOEXCEPT;

    /// Stranded event, allows timer reset.
    virtual void reading() NOEXCEPT;

    /// Stranded handler invoked from stop().
    virtual void stopping(const code& ec) NOEXCEPT;

    /// Subscribe to stop notification (requires strand).
    void subscribe_stop(result_handler&& handler) NOEXCEPT;

    /// Wait.
    /// -----------------------------------------------------------------------

    /// Wait on a peer close/cancel/send, no data capture/loss.
    virtual void wait(result_handler&& handler) NOEXCEPT;

    /// Cancel wait or any asynchronous read/write operation, handlers posted.
    virtual void cancel(result_handler&& handler) NOEXCEPT;

    /// WS (generic, framed).
    /// -----------------------------------------------------------------------

    /// Read complete logical message for websockets (not for tcp).
    /// Read available buffer from the socket, handler posted to socket strand.
    virtual void read(http::flat_buffer& out,
        count_handler&& handler) NOEXCEPT;

    /// Binary or text mode applies to websockets (no-op for tcp).
    /// Write the provided buffer to socket, handler posted to socket strand.
    virtual void write(const asio::const_buffer& in, bool binary,
        count_handler&& handler) NOEXCEPT;

    /// TCP (generic, fixed size).
    /// -----------------------------------------------------------------------

    /// Read fixed-size TCP message from the remote endpoint into buffer.
    virtual void read(const asio::mutable_buffer& buffer,
        count_handler&& handler) NOEXCEPT;

    /// Write the provided buffer to socket, handler posted to socket strand.
    virtual void write(const asio::const_buffer& buffer,
        count_handler&& handler) NOEXCEPT;

    /// RPC (TCP: electrum/stratum_v1, WS: btcd).
    /// -----------------------------------------------------------------------
    /// The channel is batch-blind: the proxy stamps batch state on reads and
    /// response parts, absorbs the batch close (writing the close part and
    /// re-arming the read), and defers notifications while a batch is open.

    /// Read rpc request from the socket, using provided buffer.
    /// The proxy stamps batch state (the parse is always lax).
    virtual void read(http::flat_buffer& buffer, rpc::request& request,
        count_handler&& handler) NOEXCEPT;

    /// Write rpc response to the socket (json buffer in body).
    virtual void write(rpc::response&& response,
        count_handler&& handler) NOEXCEPT;

    /// Write rpc notification (request) to the socket (json buffer in body).
    /// Deferred while a batch is open, drained following the close part.
    virtual void write(rpc::request&& notification,
        count_handler&& handler) NOEXCEPT;

    /// HTTP/WS (generic/rpc).
    /// -----------------------------------------------------------------------

    /// Read http request from the socket, using provided buffer.
    /// If socket is websocket request body type must have been set by caller.
    virtual void read(http::flat_buffer& buffer, http::request& request,
        count_handler&& handler) NOEXCEPT;

    /// Write http response to the socket (json buffer in body).
    /// If socket is websocket body is written (headers ignored).
    virtual void write(http::response&& response,
        count_handler&& handler) NOEXCEPT;

private:
    typedef std::function<void()> writer;
    typedef std::deque<writer> queue;

    // For write buffering.
    void do_http_write(const http::response_ptr& response,
        const count_handler& handler) NOEXCEPT;
    void do_ws_write(const asio::const_buffer& payload, bool binary,
        const count_handler& handler) NOEXCEPT;
    void do_tcp_write(const asio::const_buffer& payload,
        const count_handler& handler) NOEXCEPT;
    void do_response_write(const rpc::response_ptr& response,
        const count_handler& handler) NOEXCEPT;
    void do_notification_write(const rpc::request_ptr& notification,
        const count_handler& handler) NOEXCEPT;
    void do_subscribe_stop(const result_handler& handler,
        const result_handler& complete) NOEXCEPT;

    // For rpc batch normalization.
    void do_defer_write(const writer& call) NOEXCEPT;
    void handle_rpc_read(const code& ec, size_t bytes,
        const ref<rpc::request>& request, const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;
    void handle_close_write(const code& ec, size_t bytes,
        const ref<rpc::request>& request, const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;

    // For graceful batch closure on stop.
    void do_stop(const code& ec) NOEXCEPT;
    void finish_stop(const code& ec) NOEXCEPT;
    void handle_stop_write(const code& ec, size_t bytes,
        const code& reason) NOEXCEPT;

    // For rpc batch normalization (http).
    void do_http_request_read(const ref<http::request>& request,
        const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;
    void handle_http_header(const code& ec, size_t bytes,
        const ref<http::request>& request, const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;
    void handle_http_body(const code& ec, size_t bytes,
        const ref<http::request>& request, const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;
    void handle_http_close_write(const code& ec, size_t bytes,
        const ref<http::request>& request, const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;
    void handle_http_header_write(const code& ec, size_t bytes,
        const rpc::response_ptr& part, const count_handler& handler) NOEXCEPT;

    // Implement chunked write with result handler.
    void write() NOEXCEPT;
    void do_write(const writer& call) NOEXCEPT;
    void handle_write(const code& ec, size_t bytes,
        const count_handler& handler) NOEXCEPT;

    // Invoke reading() on strand.
    void do_reading() NOEXCEPT;

    // These are thread safe.
    std::atomic_bool paused_{ true };
    std::atomic<uint64_t> total_{};
    socket::ptr socket_;

    // These are protected by strand.
    stop_subscriber stop_subscriber_{};
    socket::http_parser_ptr parser_{};
    queue deferred_{};
    queue queue_{};
    bool parted_{};
    bool batched_{};
};

} // namespace network
} // namespace libbitcoin

#endif
