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
#ifndef LIBBITCOIN_NETWORK_NET_PROXY_HPP
#define LIBBITCOIN_NETWORK_NET_PROXY_HPP

#include <atomic>
#include <deque>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/memory.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/deadline.hpp>
#include <bitcoin/network/net/socket.hpp>

namespace libbitcoin {
namespace network {

/// Abstract, thread safe except some methods requiring strand.
/// Handles all channel communication, error handling, and logging.
/// Caller must retain ownership of read/write buffers until handler invoked.
/// All writes are queued to prevent interleaving.
/// Completion handler is invoked once write is complete, at which point next
/// queued write is invoked. Write with another queued behind it may be held by
/// socket, completing before it is written with later write. When channel stops
/// with pending writes, write queue is purged without invoke of purged
/// handlers. Each send is allocated (bytes/rate_limit) of time, and its
/// completion is deferred by whatever portion of that allocation write did not
/// consume. Since nothing is produced until completion handler is invoked, this
/// throttles channel without queueing. Reads are not metered, as they are
/// bounded by protocol correctness. Zero rate_limit disables throttle.
class BCT_API proxy
  : public enable_shared_from_base<proxy>, public reporter
{
public:
    typedef std::shared_ptr<proxy> ptr;
    typedef uint64_t object_key;
    typedef desubscriber<object_key> stop_subscriber;

    DELETE_COPY_MOVE(proxy);

    /// Asserts/logs stopped.
    virtual ~proxy() NOEXCEPT;

    /// Stop.
    /// -----------------------------------------------------------------------

    /// Idempotent, may be called multiple times.
    /// Stop socket, no delay, called by stop notify when iocontext is closing.
    /// Open batch response is closed (written) before socket stops.
    virtual void stop(const code& ec) NOEXCEPT;

    /// Subscribe to stop notification with completion handler.
    /// Completion and event handlers always invoked on channel strand.
    void subscribe_stop(result_handler&& handler,
        result_handler&& complete) NOEXCEPT;

    /// Subscribe to stop notification, key for unsubscribe (requires strand).
    object_key subscribe_stop(result_handler&& handler) NOEXCEPT;

    /// Unsubscribe from stop notification, handler invoked with desubscribed.
    /// True if subscription key found (requires strand).
    bool unsubscribe_stop(object_key key) NOEXCEPT;

    /// Pause.
    /// -----------------------------------------------------------------------

    /// Properties.
    /// -----------------------------------------------------------------------

    /// Get the network threadpool iocontext.
    io::context& service() const NOEXCEPT;

    /// The channel strand.
    io::strand& strand() NOEXCEPT;

    /// The strand is running in this thread.
    bool stranded() const NOEXCEPT;

    /// The proxy (socket) is stopped.
    bool stopped() const NOEXCEPT;

    /// The socket was accepted (vs. connected).
    bool inbound() const NOEXCEPT;

    /// Connection is currently secured (TLS or comparable for socket type).
    bool secure() const NOEXCEPT;

    /// Connection was upgraded to encrypted transport (bip324).
    bool encrypted() const NOEXCEPT;

    /// The socket was upgraded to a websocket.
    bool websocket() const NOEXCEPT;

    /// Downgraded from http to tcp by the initial json-rpc request.
    bool downgraded() const NOEXCEPT;

    /// The transport has been detected (downgraded is otherwise undefined).
    bool detected() const NOEXCEPT;

    /// The total number of bytes queued/sent to the remote endpoint.
    uint64_t sent() const NOEXCEPT;

    /// The total number of bytes received from the remote endpoint.
    uint64_t received() const NOEXCEPT;

    /// Get the address of the outgoing endpoint passed via construct.
    const config::address& address() const NOEXCEPT;

    /// Get the endpoint of the remote host.
    const config::endpoint& endpoint() const NOEXCEPT;

    /// Get the endpoint of the local socket.
    const config::endpoint& binding() const NOEXCEPT;

    /// Get the outbound slot of the socket (max_size_t if none).
    size_t slot() const NOEXCEPT;

protected:
    proxy(const socket::ptr& socket, uint32_t rate_limit,
        size_t maximum_backlog) NOEXCEPT;

    /// Accept a websocket upgrade request (requires strand).
    code accept_websocket(const http::request& request) NOEXCEPT;

    /// Stranded event, allows timer reset.
    virtual void reading() NOEXCEPT;

    /// Signal write activity (called on strand upon send completion).
    virtual void writing() NOEXCEPT;

    /// Stranded handler invoked from stop().
    virtual void stopping(const code& ec) NOEXCEPT;

    /// True if the stop closes the connection gracefully.
    virtual bool lazy(const code& ec) const NOEXCEPT;

    /// Unconsumed portion of throttle byte allocation (requires strand).
    steady_clock::duration unconsumed(size_t bytes,
        const steady_clock::time_point& start) const NOEXCEPT;

    /// Wait.
    /// -----------------------------------------------------------------------

    /// Check the peer for close after one interval, no data capture/loss.
    void poll(result_handler&& handler) NOEXCEPT;

    /// WS (generic, framed).
    /// -----------------------------------------------------------------------

    /// Read complete logical message for websockets (not for tcp).
    /// Read available buffer from socket, handler posted to socket strand.
    void read(http::flat_buffer& out,
        count_handler&& handler) NOEXCEPT;

    /// Binary or text mode applies to websockets (no-op for tcp).
    /// Write provided buffer to socket, handler posted to socket strand.
    void write(const io::const_buffer& in, bool binary,
        count_handler&& handler) NOEXCEPT;

    /// TCP (generic, fixed size).
    /// -----------------------------------------------------------------------

    /// Read fixed-size TCP message from the remote endpoint into buffer.
    void read(const io::mutable_buffer& buffer,
        count_handler&& handler) NOEXCEPT;

    /// Write the provided buffer to socket, handler posted to socket strand.
    void write(const io::const_buffer& buffer,
        count_handler&& handler) NOEXCEPT;

    /// RPC (TCP: electrum/stratum_v1, WS: btcd).
    /// -----------------------------------------------------------------------
    /// Channel is batch-blind: proxy stamps batch state on reads and response
    /// parts, absorbs batch close (writing close part and re-arming read), and
    /// defers notifications while batch is open.

    /// Read rpc request from socket, using provided buffer.
    /// Proxy stamps batch state (parse is always lax).
    void read(http::flat_buffer& buffer, rpc::request& request,
        count_handler&& handler) NOEXCEPT;

    /// PEER (TCP: bitcoin p2p).
    /// -----------------------------------------------------------------------

    /// Read peer message from socket, using provided buffer.
    /// Caller stamps parse context (magic/version/witness) on frame.
    void read(system::data_chunk& buffer,
        messages::peer::frame& message, count_handler&& handler) NOEXCEPT;

    /// Write peer message to the socket (peer::serialize frame in body).
    void write(messages::peer::frame&& message,
        count_handler&& handler) NOEXCEPT;

    /// Notify peer message to the socket (unsolicited, so bounded).
    void notify(messages::peer::frame&& message,
        count_handler&& handler) NOEXCEPT;

    /// Write rpc response to the socket (json buffer in body).
    void write(rpc::response&& response,
        count_handler&& handler) NOEXCEPT;

    /// Notify rpc notification (request) to socket (json buffer in body).
    /// Deferred while batch is open, drained following close part.
    void notify(rpc::request&& notification,
        count_handler&& handler) NOEXCEPT;

    /// HTTP/WS (generic/rpc).
    /// -----------------------------------------------------------------------

    /// Read http request from socket, using provided buffer.
    /// If socket is websocket request body type must have been set by caller.
    void read(http::flat_buffer& buffer, http::request& request,
        count_handler&& handler) NOEXCEPT;

    /// Write http response to socket (json buffer in body).
    /// If socket is websocket body is written (headers ignored).
    void write(http::response&& response,
        count_handler&& handler) NOEXCEPT;

    /// Notify http response to the socket (unsolicited, so bounded).
    void notify(http::response&& notification,
        count_handler&& handler) NOEXCEPT;

private:
    typedef std::function<void()> writer;

    // Queued write retains memory, charged as backlog of channel.
    // Entry owns handler, invoked on completion or refused write.
    struct pending
    {
        size_t cost;
        writer call;
        count_handler handler;
    };

    typedef std::deque<pending> queue;

    // Solicited writes are unbounded, as they cannot accumulate.
    void write(messages::peer::frame&& message, count_handler&& handler,
        bool bounded) NOEXCEPT;
    void write(http::response&& response, count_handler&& handler,
        bool bounded) NOEXCEPT;

    // For batch stamping at issue (requires strand).
    void do_response_queue(const rpc::response_ptr& response,
        const count_handler& handler) NOEXCEPT;
    void do_http_queue(const http::response_ptr& response,
        const count_handler& handler, bool bounded) NOEXCEPT;

    // For write buffering.
    void do_http_write(const http::response_ptr& response) NOEXCEPT;
    void do_ws_write(const io::const_buffer& payload,
        bool binary) NOEXCEPT;
    void do_tcp_write(const io::const_buffer& payload) NOEXCEPT;
    void do_peer_write(const messages::peer::frame_ptr& message) NOEXCEPT;
    void do_response_write(const rpc::response_ptr& response) NOEXCEPT;
    void do_notification_write(
        const rpc::request_ptr& notification) NOEXCEPT;
    void do_subscribe_stop(const result_handler& handler,
        const result_handler& complete) NOEXCEPT;
    object_key create_key() NOEXCEPT;
    static bool handle_stop(const code& ec,
        const result_handler& handler) NOEXCEPT;

    // For rpc batch normalization.
    void do_defer_write(pending& write) NOEXCEPT;
    void queue_close(count_handler&& complete) NOEXCEPT;
    bool absorb_keepalive(const rpc::request& value) NOEXCEPT;
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

    // For http vs. tcp transport detection.
    void handle_detect(const code& ec, size_t bytes,
        const ref<http::request>& request, const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;

    // For rpc batch normalization (http).
    void do_rpc_request_read(const ref<rpc::request>& request,
        const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;
    void do_downgrade_read(const ref<http::request>& request,
        const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;

    void do_http_request_read(const ref<http::request>& request,
        const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;
    void handle_http_header(const code& ec, size_t bytes,
        const ref<http::request>& request, const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;
    void do_http_continue(const ref<http::request>& request,
        const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;
    void handle_http_continue(const code& ec, size_t bytes,
        const ref<http::request>& request, const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;
    void do_http_body_read(const ref<http::request>& request,
        const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;
    void handle_http_body(const code& ec, size_t bytes,
        const ref<http::request>& request, const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;
    void handle_http_element(size_t bytes,
        const ref<http::request>& request, const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;
    void handle_http_close_write(const code& ec, size_t bytes,
        const ref<http::request>& request, const ref<http::flat_buffer>& buffer,
        const count_handler& handler) NOEXCEPT;
    void handle_http_header_write(const code& ec, size_t bytes,
        const rpc::response_ptr& part) NOEXCEPT;

    // Implement chunked write with result handler.
    void write() NOEXCEPT;
    bool charge(pending& write) NOEXCEPT;
    void do_write(pending& write, bool notification) NOEXCEPT;
    void handle_write(const code& ec, size_t bytes) NOEXCEPT;

    // Meter sent bytes and defer the completion by the unconsumed allocation.
    void count_received(size_t bytes) NOEXCEPT;
    count_handler counted(count_handler&& handler) NOEXCEPT;
    void handle_counted(const code& ec, size_t bytes,
        const count_handler& handler) NOEXCEPT;
    count_handler metered(count_handler&& handler) NOEXCEPT;
    void handle_metered(const code& ec, size_t bytes,
        const steady_clock::time_point& start,
        const count_handler& handler) NOEXCEPT;
    void handle_charge(const code&, const code& ec, size_t bytes,
        const count_handler& handler) NOEXCEPT;

    // Invoke reading() on strand.
    void do_reading() NOEXCEPT;

    // These are thread safe.
    std::atomic<uint64_t> sent_{};
    std::atomic<uint64_t> received_{};
    const uint32_t rate_limit_;
    const size_t maximum_backlog_;
    socket::ptr socket_;

    // These are protected by strand.
    deadline::ptr throttle_;
    stop_subscriber stop_subscriber_{};
    object_key keys_{};
    socket::http_parser_ptr parser_{};
    queue queue_{};
    queue deferred_{};
    size_t backlog_{};
    bool writing_{};
    bool looping_{};
    bool again_{};
    bool batched_{};
    bool parted_{};
    bool opened_{};
};

} // namespace network
} // namespace libbitcoin

#endif
