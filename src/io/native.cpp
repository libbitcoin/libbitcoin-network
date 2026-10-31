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
#include "native.hpp"

#include <algorithm>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace io {
namespace detail {

using namespace system;

#if defined(HAVE_WINDOWS)
    #define NATIVE(windows, posix) windows
#else
    #define NATIVE(windows, posix) posix
#endif

int last_error() NOEXCEPT
{
#if defined(HAVE_WINDOWS)
    return ::WSAGetLastError();
#else
    return errno;
#endif
}

bool is_would_block(int native) NOEXCEPT
{
#if defined(HAVE_WINDOWS)
    return native == WSAEWOULDBLOCK;
#else
    return native == EAGAIN || native == EWOULDBLOCK;
#endif
}

bool is_in_progress(int native) NOEXCEPT
{
#if defined(HAVE_WINDOWS)
    return native == WSAEWOULDBLOCK || native == WSAEINPROGRESS;
#else
    return native == EINPROGRESS;
#endif
}

code to_code(int native) NOEXCEPT
{
    switch (native)
    {
        case 0:
            return error::success;

#if defined(HAVE_WINDOWS)
        // Overlapped (win32) completion errors.
        case ERROR_OPERATION_ABORTED:
        case ERROR_CONNECTION_ABORTED:
            return error::operation_canceled;
        case ERROR_NETNAME_DELETED:
            return error::peer_disconnect;
        case ERROR_CONNECTION_REFUSED:
        case ERROR_PORT_UNREACHABLE:
            return error::connection_refused;
        case ERROR_HOST_UNREACHABLE:
            return error::host_unreachable;
        case ERROR_NETWORK_UNREACHABLE:
            return error::net_unreachable;
        case ERROR_SEM_TIMEOUT:
            return error::channel_timeout;
#else
        case ECANCELED:
            return error::operation_canceled;
#endif

        case NATIVE(WSAECONNABORTED, ECONNABORTED):
            return error::operation_canceled;

        case NATIVE(WSAEAFNOSUPPORT, EAFNOSUPPORT):
        case NATIVE(WSAEDESTADDRREQ, EDESTADDRREQ):
            return error::resolve_failed;

        case NATIVE(WSAEADDRINUSE, EADDRINUSE):
        case NATIVE(WSAEISCONN, EISCONN):
        case NATIVE(WSAEALREADY, EALREADY):
        case NATIVE(WSAEINPROGRESS, EINPROGRESS):
            return error::address_in_use;

        case NATIVE(WSAEADDRNOTAVAIL, EADDRNOTAVAIL):
        case NATIVE(WSAENETDOWN, ENETDOWN):
        case NATIVE(WSAENETUNREACH, ENETUNREACH):
            return error::net_unreachable;

        case NATIVE(WSAEHOSTUNREACH, EHOSTUNREACH):
            return error::host_unreachable;

        case NATIVE(WSAECONNREFUSED, ECONNREFUSED):
            return error::connection_refused;

        case NATIVE(WSAEWOULDBLOCK, EWOULDBLOCK):
#if !defined(HAVE_WINDOWS) && (EAGAIN != EWOULDBLOCK)
        case EAGAIN:
#endif
            return error::would_block;

        case NATIVE(WSAESHUTDOWN, ESHUTDOWN):
        case NATIVE(WSAENETRESET, ENETRESET):
        case NATIVE(WSAENOPROTOOPT, ENOPROTOOPT):
        case NATIVE(WSAENOTCONN, ENOTCONN):
        case NATIVE(WSAENOTSOCK, ENOTSOCK):
        case NATIVE(WSAEPROTONOSUPPORT, EPROTONOSUPPORT):
        case NATIVE(WSAEPROTOTYPE, EPROTOTYPE):
#if !defined(HAVE_WINDOWS)
        case EPIPE:
#endif
            return error::connect_failed;

        case NATIVE(WSAECONNRESET, ECONNRESET):
            return error::peer_disconnect;

        case NATIVE(WSAEBADF, EBADF):
        case NATIVE(WSAEMSGSIZE, EMSGSIZE):
            return error::bad_stream;

        case NATIVE(WSAEFAULT, EFAULT):
        case NATIVE(WSAEINTR, EINTR):
        case NATIVE(WSAEINVAL, EINVAL):
        case NATIVE(WSAENAMETOOLONG, ENAMETOOLONG):
        case NATIVE(WSAEMFILE, EMFILE):
#if !defined(HAVE_WINDOWS)
        case ENOMEM:
#endif
            return error::invalid_configuration;

        case NATIVE(WSAENOBUFS, ENOBUFS):
            return error::insufficient_buffer;

        case NATIVE(WSAEACCES, EACCES):
        case NATIVE(WSAEOPNOTSUPP, EOPNOTSUPP):
#if !defined(HAVE_WINDOWS)
        case EPERM:
#endif
            return error::not_allowed;

        case NATIVE(WSAETIMEDOUT, ETIMEDOUT):
            return error::channel_timeout;

        default:
            return error::unknown;
    }
}

#undef NATIVE

// Socket addresses.
// ----------------------------------------------------------------------------

sockaddr* socket_address::data() NOEXCEPT
{
    return pointer_cast<sockaddr>(&storage);
}

const sockaddr* socket_address::data() const NOEXCEPT
{
    return pointer_cast<const sockaddr>(&storage);
}

socket_address to_socket_address(const ip::endpoint& point) NOEXCEPT
{
    socket_address out{};
    const auto& address = point.address();
    if (address.is_v6())
    {
        auto& value = *pointer_cast<sockaddr_in6>(&out.storage);
        value.sin6_family = AF_INET6;
        value.sin6_port = htons(point.port());
        const auto& bytes = address.to_v6().to_bytes();
        std::copy(bytes.begin(), bytes.end(),
            pointer_cast<uint8_t>(&value.sin6_addr));

        out.size = sizeof(sockaddr_in6);
    }
    else
    {
        auto& value = *pointer_cast<sockaddr_in>(&out.storage);
        value.sin_family = AF_INET;
        value.sin_port = htons(point.port());
        const auto& bytes = address.to_v4().to_bytes();
        std::copy(bytes.begin(), bytes.end(),
            pointer_cast<uint8_t>(&value.sin_addr));

        out.size = sizeof(sockaddr_in);
    }

    return out;
}

ip::endpoint to_endpoint(const sockaddr* address) NOEXCEPT
{
    if (address->sa_family == AF_INET6)
    {
        const auto& value = *pointer_cast<const sockaddr_in6>(address);
        ip::ipv6::bytes_type bytes{};
        const auto start = pointer_cast<const uint8_t>(&value.sin6_addr);
        std::copy_n(start, bytes.size(), bytes.begin());
        return { ip::ipv6{ bytes }, ntohs(value.sin6_port) };
    }

    if (address->sa_family == AF_INET)
    {
        const auto& value = *pointer_cast<const sockaddr_in>(address);
        ip::ipv4::bytes_type bytes{};
        const auto start = pointer_cast<const uint8_t>(&value.sin_addr);
        std::copy_n(start, bytes.size(), bytes.begin());
        return { ip::ipv4{ bytes }, ntohs(value.sin_port) };
    }

    return {};
}

// Sockets.
// ----------------------------------------------------------------------------

void close_socket(native_socket socket) NOEXCEPT
{
    if (socket == invalid_socket)
        return;

#if defined(HAVE_WINDOWS)
    ::closesocket(socket);
#else
    ::close(socket);
#endif
}

native_socket create_socket(bool v6, int& native) NOEXCEPT
{
    const auto family = v6 ? AF_INET6 : AF_INET;

#if defined(HAVE_WINDOWS)
    const auto socket = ::WSASocketW(family, SOCK_STREAM, IPPROTO_TCP,
        nullptr, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);
    native = socket == invalid_socket ? last_error() : 0;
    return socket;
#else
    const auto socket = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (socket == invalid_socket)
    {
        native = last_error();
        return socket;
    }

    // Close on exec, nonblocking, and (on apple) no SIGPIPE.
    ::fcntl(socket, F_SETFD, FD_CLOEXEC);
#if defined(SO_NOSIGPIPE)
    const int enable{ 1 };
    ::setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &enable, sizeof(enable));
#endif
    if (!set_blocking(socket, false, native))
    {
        close_socket(socket);
        return invalid_socket;
    }

    native = 0;
    return socket;
#endif
}

bool set_blocking(native_socket socket, bool blocking, int& native) NOEXCEPT
{
#if defined(HAVE_WINDOWS)
    u_long value = blocking ? 0 : 1;
    if (::ioctlsocket(socket, FIONBIO, &value) == SOCKET_ERROR)
    {
        native = last_error();
        return false;
    }
#else
    const auto flags = ::fcntl(socket, F_GETFL, 0);
    const auto value = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    if (flags < 0 || ::fcntl(socket, F_SETFL, value) < 0)
    {
        native = last_error();
        return false;
    }
#endif

    native = 0;
    return true;
}

bool wait_ready(native_socket socket, bool write, int& native) NOEXCEPT
{
#if defined(HAVE_WINDOWS)
    WSAPOLLFD descriptor{ socket, static_cast<SHORT>(write ? POLLOUT :
        POLLIN), 0 };

    if (::WSAPoll(&descriptor, 1, -1) == SOCKET_ERROR)
#else
    pollfd descriptor{ socket, static_cast<short>(write ? POLLOUT : POLLIN),
        0 };

    if (::poll(&descriptor, 1, -1) < 0)
#endif
    {
        native = last_error();
        return false;
    }

    native = 0;
    return true;
}

} // namespace detail
} // namespace io
} // namespace network
} // namespace libbitcoin
