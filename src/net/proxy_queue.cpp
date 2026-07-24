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
#include <bitcoin/network/net/proxy.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

// stackoverflow.com/questions/7754695/boost-asio-async-write-how-to-not-
// interleaving-async-write-calls

namespace libbitcoin {
namespace network {

// Send cycle (send continues until queue is empty).
// ----------------------------------------------------------------------------
// private

void proxy::do_write(const writer& call) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped())
    {
        // Does not queue new work or invoke handler after stop.
        LOGQ("Payload write abort [" << endpoint() << "]");
        return;
    }

    const auto started = !queue_.empty();
    queue_.push_back(call);

    // Start the asynchronous loop if it wasn't already started.
    if (!started)
        write();
}

void proxy::write() NOEXCEPT
{
    BC_ASSERT(stranded());
    if (queue_.empty())
        return;

    // Invokes oldest writer on the queue, completion invokes handle_write.
    queue_.front()();
}

void proxy::handle_write(const code& ec, size_t bytes,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (queue_.empty())
        return;

    handler(ec, bytes);
    queue_.pop_front();
    total_ = system::ceilinged_add(total_.load(), bytes);

    // All handlers must be invoked unless stopped, so continue despite code.
    write();
}

} // namespace network
} // namespace libbitcoin
