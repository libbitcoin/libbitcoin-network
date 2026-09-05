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

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

namespace libbitcoin {
namespace network {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace std::placeholders;

// ZMTP (read).
// ----------------------------------------------------------------------------
// The stream frames the wire only. Control traffic (subscriptions, PING/PONG)
// is intercepted by the tier above on read and answered through its own write
// queue, so this surfaces one frame per read.

void socket::zmtp_read(zmtp::stream::frame& out,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_zmtp_read,
            shared_from_this(), std::ref(out), std::move(handler)));
}

// private
void socket::do_zmtp_read(ref<zmtp::stream::frame> out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (!publisher())
    {
        handler(error::bad_stream, zero);
        return;
    }

    get_zmtp().async_read_frame(out.get(),
        std::bind(&socket::handle_async,
            shared_from_this(), _1, _2, handler, "async_read_frame"));
}

// ZMTP (write).
// ----------------------------------------------------------------------------
// Buffer is fully framed and allocated before write, identical to tcp_write.

void socket::zmtp_write(const asio::const_buffer& in,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_zmtp_write,
            shared_from_this(), in, std::move(handler)));
}

// private
void socket::do_zmtp_write(const asio::const_buffer& in,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (!publisher())
    {
        handler(error::bad_stream, zero);
        return;
    }

    get_zmtp().async_write(in,
        std::bind(&socket::handle_async,
            shared_from_this(), _1, _2, handler, "async_write"));
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
