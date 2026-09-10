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
#include <bitcoin/network/asio.hpp>

#include <bitcoin/network/define.hpp>

#if defined(HAVE_MSC)
    #include <winsock2.h>
    #include <mstcpip.h>
#elif defined(HAVE_APPLE)
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <netinet/tcp_fsm.h>
#elif defined(HAVE_LINUX)
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
#endif

namespace libbitcoin {
namespace network {
namespace asio {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

bool half_closed(socket& sock) NOEXCEPT
{
    using namespace system;
    if (!sock.is_open())
        return false;

#if defined(HAVE_MSC)
    TCP_INFO_v0 info{};
    DWORD version{};
    DWORD size{};
    if (is_zero(::WSAIoctl(sock.native_handle(), SIO_TCP_INFO, &version,
        sizeof(version), &info, sizeof(info), &size, nullptr, nullptr)))
        return info.State == TCPSTATE_CLOSE_WAIT;

#elif defined(HAVE_APPLE)
    struct tcp_connection_info info{};
    socklen_t size = sizeof(info);
    if (is_zero(::getsockopt(sock.native_handle(), IPPROTO_TCP,
        TCP_CONNECTION_INFO, &info, &size)))
        return info.tcpi_state == TCPS_CLOSE_WAIT;

#elif defined(HAVE_LINUX)
    struct tcp_info info{};
    socklen_t size = sizeof(info);
    if (is_zero(::getsockopt(sock.native_handle(), IPPROTO_TCP, TCP_INFO,
        &info, &size)))
        return info.tcpi_state == TCP_CLOSE_WAIT;
#endif
    return false;
}

BC_POP_WARNING()

} // namespace asio
} // namespace network
} // namespace libbitcoin
