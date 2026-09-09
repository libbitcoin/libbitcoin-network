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
    
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Wait.
// ----------------------------------------------------------------------------

// The peer close is a connection state, not a readability event. A socket
// carrying a request that arrived during a long-running query is readable
// whether or not the peer has closed, so the state is read directly. That
// leaves the request in the receive buffer, so a caller may pipeline up to
// that limit without the monitor mistaking the request for a drop.
bool socket::half_closed() NOEXCEPT
{
    BC_ASSERT(stranded());
    return asio::half_closed(get_base());
}

void socket::cancel(result_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_cancel,
            shared_from_this(), std::move(handler)));
}

// private
void socket::do_cancel(const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped())
    {
        handler(error::success);
        return;
    }

    try
    {
        // Causes connect, send, and receive calls to quit with
        // asio::error::operation_aborted passed to handlers.
        get_base().cancel();
        handler(error::success);
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ do_cancel: " << e.what());
        handler(error::service_stopped);
    }
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
