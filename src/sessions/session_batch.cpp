/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/sessions/session_batch.hpp>

#include <cstddef>
#include <bitcoin/system.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/sessions/session.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_batch
#define NAME "session_batch"

using namespace bc::system;
using namespace config;
using namespace messages;
using namespace std::placeholders;

session_batch::session_batch(p2p& network)
  : session(network),
    batch_size_(std::max(network.network_settings().connect_batch_size, 1u)),
    count_(zero)
{
}

void session_batch::connect(channel_handler handler)
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    const auto connects = create_connectors(batch_size_);

    channel_handler start = 
        BIND4(handle_connect, _1, _2, connects, std::move(handler));

    // Initialize batch of connectors.
    for (auto it = connects->begin(); it != connects->end() && !stopped(); ++it)
        network_.fetch_address(BIND4(start_connect, _1, _2, *it, start));
}

void session_batch::start_connect(const code& ec, const authority& host,
    connector::ptr connector, channel_handler handler)
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    if (stopped(ec))
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    // This termination prevents a tight loop in the empty address pool case.
    if (ec)
    {
        handler(ec, nullptr);
        return;
    }

    // This creates a tight loop in the case of a small address pool.
    if (blacklisted(host))
    {
        handler(error::address_blocked, nullptr);
        return;
    }

    // CONNECT (wait)
    connector->connect(host, std::move(handler));
}

// Called once for each call to start_connect.
void session_batch::handle_connect(const code& ec, channel::ptr channel,
    connectors_ptr connectors, channel_handler complete)
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    auto finished = (++count_ == batch_size_);

    if (!ec)
    {
        // Clear unfinished connectors.
        if (!finished)
            for (auto it = connectors->begin(); it != connectors->end(); ++it)
                (*it)->stop();

        // Got a connection.
        complete(error::success, channel);
        return;
    }

    // Got no successful connection.
    if (finished)
        complete(error::connect_failed, nullptr);
}

} // namespace network
} // namespace libbitcoin
