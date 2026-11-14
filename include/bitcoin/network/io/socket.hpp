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
#ifndef LIBBITCOIN_NETWORK_IO_SOCKET_HPP
#define LIBBITCOIN_NETWORK_IO_SOCKET_HPP

#include <functional>
#include <memory>
#include <vector>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/io/buffer.hpp>
#include <bitcoin/network/io/context.hpp>

namespace libbitcoin {
namespace network {
namespace io {

namespace detail { struct descriptor; }

using endpoint = system::ip::endpoint;
using endpoints = std::vector<endpoint>;

/// The tcp protocol of an address family.
class BCT_API tcp
{
public:
    static tcp v4() NOEXCEPT;
    static tcp v6() NOEXCEPT;

    /// The protocol of the endpoint's address family.
    static tcp of(const endpoint& point) NOEXCEPT;

    bool is_v6() const NOEXCEPT;
    bool operator==(const tcp& other) const NOEXCEPT = default;

private:
    explicit tcp(bool v6) NOEXCEPT;
    bool v6_;
};

/// Socket options.
struct reuse_address { bool value; };
struct v6_only { bool value; };
struct no_delay { bool value; };
struct keep_alive { bool value; };

#if defined(HAVE_WINDOWS)
using native_handle = uintptr_t;
#else
using native_handle = int;
#endif

/// Not thread safe. A tcp stream socket. Asynchronous operations complete on
/// the executor, with operation_canceled if canceled or closed first. One
/// read and one write may be pending at a time.
class BCT_API socket
{
public:
    DELETE_COPY(socket);

    using executor_type = executor;
    using native_handle_type = io::native_handle;
    using io_handler = std::function<void(const code&, size_t)>;
    using result_handler = std::function<void(const code&)>;

    enum class shutdown_type
    {
        receive,
        send,
        both
    };

    explicit socket(const executor& executor) NOEXCEPT;
    socket(socket&& other) NOEXCEPT;
    socket& operator=(socket&& other) NOEXCEPT;

    /// Closes the socket.
    ~socket() NOEXCEPT;

    executor_type get_executor() const NOEXCEPT;
    bool is_open() const NOEXCEPT;
    native_handle_type native_handle() const NOEXCEPT;

    void open(const tcp& protocol, code& ec) NOEXCEPT;
    void bind(const endpoint& point, code& ec) NOEXCEPT;
    void close(code& ec) NOEXCEPT;
    void cancel(code& ec) NOEXCEPT;
    void shutdown(shutdown_type what, code& ec) NOEXCEPT;

    /// Close, ignoring any error (as destruction).
    void close() NOEXCEPT;
    void set_option(const no_delay& option, code& ec) NOEXCEPT;
    void set_option(const keep_alive& option, code& ec) NOEXCEPT;

    /// Synchronous operations fail with would_block if not blocking.
    void non_blocking(bool value, code& ec) NOEXCEPT;
    bool non_blocking() const NOEXCEPT;

    /// The number of received bytes that can be read without blocking.
    size_t available(code& ec) const NOEXCEPT;

    /// The peer has closed its side of the connection (FIN received),
    /// or reset it (RST received).
    bool half_closed() const NOEXCEPT;

    endpoint local_endpoint(code& ec) const NOEXCEPT;
    endpoint remote_endpoint(code& ec) const NOEXCEPT;
    endpoint local_endpoint() const NOEXCEPT;
    endpoint remote_endpoint() const NOEXCEPT;

    /// Connect (opening the socket for the endpoint's family if closed).
    void async_connect(const endpoint& point,
        result_handler&& handler) NOEXCEPT;

    template <typename Buffers>
    void async_read_some(const Buffers& buffers,
        io_handler&& handler) NOEXCEPT;

    template <typename Buffers>
    void async_write_some(const Buffers& buffers,
        io_handler&& handler) NOEXCEPT;

    /// Synchronous operations.
    void connect(const endpoint& point, code& ec) NOEXCEPT;

    template <typename Buffers>
    size_t read_some(const Buffers& buffers, code& ec) NOEXCEPT;

    template <typename Buffers>
    size_t write_some(const Buffers& buffers, code& ec) NOEXCEPT;

private:
    friend class acceptor;

    void do_read(mutable_buffers&& buffers, io_handler&& handler) NOEXCEPT;
    void do_write(const_buffers&& buffers, io_handler&& handler) NOEXCEPT;
    size_t do_read_some(const mutable_buffers& buffers, code& ec) NOEXCEPT;
    size_t do_write_some(const const_buffers& buffers, code& ec) NOEXCEPT;

    executor executor_;
    std::shared_ptr<detail::descriptor> descriptor_;
};

/// Not thread safe. A listening tcp socket.
class BCT_API acceptor
{
public:
    DELETE_COPY_MOVE(acceptor);

    using executor_type = executor;
    using native_handle_type = io::native_handle;
    using result_handler = std::function<void(const code&)>;

    explicit acceptor(const executor& executor) NOEXCEPT;

    /// Open, reuse address, bind and listen (closed if any step fails).
    acceptor(const executor& executor, const endpoint& point) NOEXCEPT;

    /// Closes the acceptor.
    ~acceptor() NOEXCEPT;

    executor_type get_executor() const NOEXCEPT;
    bool is_open() const NOEXCEPT;
    native_handle_type native_handle() const NOEXCEPT;

    void open(const tcp& protocol, code& ec) NOEXCEPT;
    void set_option(const reuse_address& option, code& ec) NOEXCEPT;
    void set_option(const v6_only& option, code& ec) NOEXCEPT;
    void bind(const endpoint& point, code& ec) NOEXCEPT;

    /// Listen with the maximum backlog of the system.
    void listen(code& ec) NOEXCEPT;
    void close(code& ec) NOEXCEPT;
    void cancel(code& ec) NOEXCEPT;

    /// Close, ignoring any error (as destruction).
    void close() NOEXCEPT;

    endpoint local_endpoint(code& ec) const NOEXCEPT;
    endpoint local_endpoint() const NOEXCEPT;

    /// Accept a connection into the closed socket (which must outlive the
    /// operation).
    void async_accept(socket& peer, result_handler&& handler) NOEXCEPT;
    void accept(socket& peer, code& ec) NOEXCEPT;

private:
    executor executor_;
    std::shared_ptr<detail::descriptor> descriptor_;
};

/// Not thread safe. Resolves host names (on a context thread).
class BCT_API resolver
{
public:
    DELETE_COPY_MOVE(resolver);

    using executor_type = executor;
    using resolve_handler = std::function<void(const code&,
        const endpoints&)>;

    explicit resolver(const executor& executor) NOEXCEPT;

    /// Cancels pending resolutions.
    ~resolver() NOEXCEPT;

    executor_type get_executor() const NOEXCEPT;

    /// Resolve the host and service (port) to tcp endpoints.
    void async_resolve(const std::string& host, const std::string& service,
        resolve_handler&& handler) NOEXCEPT;

    /// Pending resolutions complete with operation_canceled.
    void cancel() NOEXCEPT;

private:
    struct state;

    executor executor_;
    std::shared_ptr<state> state_;
};

/// Connect to the first endpoint that accepts, in sequence (the socket is
/// closed before each attempt), returning the connected endpoint.
using connect_handler = std::function<void(const code&, const endpoint&)>;
BCT_API void async_connect(socket& socket, const endpoints& range,
    connect_handler&& handler) NOEXCEPT;

} // namespace io
} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/io/socket.ipp>

#endif
