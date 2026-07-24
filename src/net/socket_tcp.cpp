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
#include <bitcoin/network/net/socket.hpp>

#include <utility>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

namespace libbitcoin {
namespace network {

using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// TCP (read).
// ----------------------------------------------------------------------------
// Fixed-size read that completes only when the mutable_buffer is filled.

void socket::tcp_read(const asio::mutable_buffer& out,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_tcp_read,
            shared_from_this(), out, std::move(handler)));
}

// private
void socket::do_tcp_read(const asio::mutable_buffer& out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    async_read(out, handler);
}

// TCP (write).
// ----------------------------------------------------------------------------
// Buffer is fully allocated before write, identical to ws_write.

void socket::tcp_write(const asio::const_buffer& in,
    count_handler&& handler) NOEXCEPT
{
    ws_write(in, true, std::move(handler));
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
