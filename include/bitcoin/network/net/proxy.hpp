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
class BCT_API proxy
  : public enable_shared_from_base<proxy>, public reporter
{
public:
    typedef std::shared_ptr<proxy> ptr;
    typedef subscriber<> stop_subscriber;

    DELETE_COPY_MOVE(proxy);

    /// Asserts/logs stopped.
    virtual ~proxy() NOEXCEPT;

    /// Pause reading from the socket (requires strand).
    virtual void pause() NOEXCEPT;

    /// Resume reading from the socket (requires strand).
    virtual void resume() NOEXCEPT;

    /// Reading from the socket is paused (requires strand).
    virtual bool paused() const NOEXCEPT;

    /// Idempotent, may be called multiple times.
    /// Stop socket, no delay, called by stop notify when iocontext is closing.
    virtual void stop(const code& ec) NOEXCEPT;

    /// Idempotent, may be called multiple times.
    /// Same as stop but provides graceful shutdown for websocket connections.
    virtual void async_stop(const code& ec) NOEXCEPT;

    /// Subscribe to stop notification with completion handler.
    /// Completion and event handlers are always invoked on the channel strand.
    void subscribe_stop(result_handler&& handler,
        result_handler&& complete) NOEXCEPT;

    /// The channel strand.
    asio::strand& strand() NOEXCEPT;

    /// Get the network threadpool iocontext.
    asio::io_context& service() const NOEXCEPT;

    /// The strand is running in this thread.
    bool stranded() const NOEXCEPT;

    /// The proxy (socket) is stopped.
    bool stopped() const NOEXCEPT;

    /// The number of bytes in the write backlog.
    uint64_t backlog() const NOEXCEPT;

    /// The total number of bytes queued/sent to the remote endpoint.
    uint64_t total() const NOEXCEPT;

    /// The socket was accepted (vs. connected).
    bool inbound() const NOEXCEPT;

    /// Get the authority (incoming) of the remote endpoint.
    const config::authority& authority() const NOEXCEPT;

    /// Get the address (outgoing) of the remote endpoint.
    const config::address& address() const NOEXCEPT;

protected:
    proxy(const socket::ptr& socket) NOEXCEPT;

    /// Stranded event, allows timer reset.
    virtual void waiting() NOEXCEPT;

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

    /// TCP (generic).
    /// -----------------------------------------------------------------------

    /// Read fixed-size TCP message from the remote endpoint into buffer.
    virtual void read(const asio::mutable_buffer& buffer,
        count_handler&& handler) NOEXCEPT;

    /// Send a complete TCP message to the remote endpoint.
    virtual void write(const asio::const_buffer& buffer,
        count_handler&& handler) NOEXCEPT;

    /// TCP-RPC (e.g. electrum, stratum_v1).
    /// -----------------------------------------------------------------------

    /// Read full rpc request from the socket, handler posted to socket strand.
    virtual void read(rpc::in_value& out, count_handler&& handler) NOEXCEPT;

    /// Write full rpc response to the socket, handler posted to socket strand.
    virtual void write(rpc::out_value&& in, count_handler&& handler) NOEXCEPT;

    /// HTTP (generic).
    /// -----------------------------------------------------------------------

    /// Read full http variant request from the socket, using provided buffer.
    virtual void read(http::flat_buffer& buffer, http::request& request,
        count_handler&& handler) NOEXCEPT;

    /// Write full http variant response to the socket (json buffer in body).
    virtual void write(http::response& response,
        count_handler&& handler) NOEXCEPT;

    /// WS (generic).
    /// -----------------------------------------------------------------------

    /// Read full buffer from the websocket (post-upgrade).
    virtual void ws_read(http::flat_buffer& out,
        count_handler&& handler) NOEXCEPT;

    /// Write full buffer to the websocket (post-upgrade), specify binary/text.
    virtual void ws_write(const asio::const_buffer& in, bool binary,
        count_handler&& handler) NOEXCEPT;

private:
    typedef std::deque<std::pair<asio::const_buffer, count_handler>> queue;

    void do_subscribe_stop(const result_handler& handler,
        const result_handler& complete) NOEXCEPT;

    // Implement chunked write with result handler.
    void write() NOEXCEPT;
    void do_write(const asio::const_buffer& payload,
        const count_handler& handler) NOEXCEPT;
    void handle_write(const code& ec, size_t bytes,
        const asio::const_buffer& payload,
        const count_handler& handler) NOEXCEPT;

    // These are thread safe.
    std::atomic_bool paused_{ true };
    std::atomic<uint64_t> backlog_{};
    std::atomic<uint64_t> total_{};
    socket::ptr socket_;

    // These are protected by strand.
    stop_subscriber stop_subscriber_{};
    queue queue_{};
};

} // namespace network
} // namespace libbitcoin

#endif
