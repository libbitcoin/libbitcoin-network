/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#include <functional>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

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

    DELETE_COPY_MOVE(socket);

    /// Use only for incoming connections (defaults outgoing address).
    socket(const logger& log, asio::io_context& service) NOEXCEPT;

    /// Use only for outgoing connections (retains outgoing address).
    socket(const logger& log, asio::io_context& service,
        const config::address& address) NOEXCEPT;

    /// Asserts/logs stopped.
    virtual ~socket() NOEXCEPT;

    // Stop.
    // ------------------------------------------------------------------------

    /// Stop has been signaled, work is stopping.
    virtual bool stopped() const NOEXCEPT;

    /// Cancel work and close the socket (idempotent, thread safe).
    /// This action is deferred to the strand, not immediately affected.
    /// Block on threadpool.join() to ensure termination of the connection.
    virtual void stop() NOEXCEPT;

    // I/O.
    // ------------------------------------------------------------------------

    /// Accept an incoming connection, handler posted to *acceptor* strand.
    /// Concurrent calls are NOT thread safe until this handler is invoked.
    virtual void accept(asio::acceptor& acceptor,
        result_handler&& handler) NOEXCEPT;

    /// Create an outbound connection, handler posted to socket strand.
    virtual void connect(const asio::endpoints& range,
        result_handler&& handler) NOEXCEPT;

    /// Read from the socket, handler posted to socket strand.
    virtual void read(const system::data_slab& out,
        count_handler&& handler) NOEXCEPT;

    /// Write to the socket, handler posted to socket strand.
    virtual void write(const system::data_slice& in,
        count_handler&& handler) NOEXCEPT;

    // Properties.
    // ------------------------------------------------------------------------

    /// Get the authority (incoming) of the remote endpoint.
    virtual const config::authority& authority() const NOEXCEPT;

    /// Get the address (outgoing) of the remote endpoint.
    virtual const config::address& address() const NOEXCEPT;

    /// The socket was accepted (vs. connected).
    virtual bool inbound() const NOEXCEPT;

    /// The strand is running in this thread.
    virtual bool stranded() const NOEXCEPT;

    /// Get the strand of the socket.
    virtual asio::strand& strand() NOEXCEPT;

protected:
    // These are thread safe.
    const config::address address_;
    std::atomic_bool stopped_{};
    asio::strand strand_;

    // These are protected by strand (see also handle_accept).
    asio::socket socket_;
    config::authority authority_{};

private:
    void do_stop() NOEXCEPT;
    void do_connect(const asio::endpoints& range,
        const result_handler& handler) NOEXCEPT;
    void do_read(const asio::mutable_buffer& out,
        const count_handler& handler) NOEXCEPT;
    void do_write(const asio::const_buffer& in,
        const count_handler& handler) NOEXCEPT;

    void handle_accept(const error::boost_code& ec,
        const result_handler& handler) NOEXCEPT;
    void handle_connect(const error::boost_code& ec,
        const asio::endpoint& peer, const result_handler& handler) NOEXCEPT;
    void handle_io(const error::boost_code& ec, size_t size,
        const count_handler& handler) NOEXCEPT;
};

typedef std::function<void(const code&, const socket::ptr&)> socket_handler;

} // namespace network
} // namespace libbitcoin

#endif
