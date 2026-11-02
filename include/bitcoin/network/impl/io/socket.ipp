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
#ifndef LIBBITCOIN_NETWORK_IMPL_IO_SOCKET_IPP
#define LIBBITCOIN_NETWORK_IMPL_IO_SOCKET_IPP

#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {
namespace io {

template <typename Buffers>
void socket::async_read_some(const Buffers& buffers,
    io_handler&& handler) NOEXCEPT
{
    do_read(to_buffers<mutable_buffer>(buffers), std::move(handler));
}

template <typename Buffers>
void socket::async_write_some(const Buffers& buffers,
    io_handler&& handler) NOEXCEPT
{
    do_write(to_buffers<const_buffer>(buffers), std::move(handler));
}

template <typename Buffers>
size_t socket::read_some(const Buffers& buffers, code& ec) NOEXCEPT
{
    return do_read_some(to_buffers<mutable_buffer>(buffers), ec);
}

template <typename Buffers>
size_t socket::write_some(const Buffers& buffers, code& ec) NOEXCEPT
{
    return do_write_some(to_buffers<const_buffer>(buffers), ec);
}

} // namespace io
} // namespace network
} // namespace libbitcoin

#endif
