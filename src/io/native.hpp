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
#ifndef LIBBITCOIN_NETWORK_IO_NATIVE_HPP
#define LIBBITCOIN_NETWORK_IO_NATIVE_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/io/buffer.hpp>

#if defined(HAVE_WINDOWS)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <mswsock.h>
    #include <mstcpip.h>
    #if defined(HAVE_WINDOWS_LIBS)
        #pragma comment(lib, "ws2_32.lib")
        #pragma comment(lib, "mswsock.lib")
    #endif
#else
    #include <cerrno>
    #include <fcntl.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <poll.h>
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <sys/uio.h>
    #include <unistd.h>
#endif

#if defined(HAVE_APPLE)
    #include <netinet/tcp_fsm.h>
#endif

namespace libbitcoin {
namespace network {
namespace io {
namespace detail {

#if defined(HAVE_WINDOWS)
using native_socket = SOCKET;
constexpr native_socket invalid_socket = INVALID_SOCKET;
#else
using native_socket = int;
constexpr native_socket invalid_socket = -1;
#endif

/// The error of the last failed socket call on this thread.
int last_error() NOEXCEPT;

/// The network code of a native (errno, winsock or win32) error.
code to_code(int native) NOEXCEPT;

/// The would-block (try again) native error.
bool is_would_block(int native) NOEXCEPT;

/// The connect-in-progress native error (nonblocking connect).
bool is_in_progress(int native) NOEXCEPT;

/// Socket address conversions.
struct socket_address
{
    sockaddr_storage storage{};
    socklen_t size{};

    sockaddr* data() NOEXCEPT;
    const sockaddr* data() const NOEXCEPT;
};

socket_address to_socket_address(const system::ip::endpoint& point) NOEXCEPT;
system::ip::endpoint to_endpoint(const sockaddr* address) NOEXCEPT;

/// Close the socket, ignoring errors.
void close_socket(native_socket socket) NOEXCEPT;

/// Create a stream socket of the family (nonblocking on posix).
native_socket create_socket(bool v6, int& native) NOEXCEPT;

/// Make a socket nonblocking (or blocking).
bool set_blocking(native_socket socket, bool blocking, int& native) NOEXCEPT;

/// Wait until the socket is readable (or writable), true unless failed.
bool wait_ready(native_socket socket, bool write, int& native) NOEXCEPT;

} // namespace detail
} // namespace io
} // namespace network
} // namespace libbitcoin

#endif
