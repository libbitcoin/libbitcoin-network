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
  : session(network, true),
    batch_size_(std::max(settings_.connect_batch_size, 1u))
{
}

// Connect sequence.
// ----------------------------------------------------------------------------

// protected:
void session_batch::connect(channel_handler handler)
{
    // TODO: just use a state member variable, this will be stranded.

    ////const auto join_handler = synchronize(handler, batch_size_, NAME "_join",
    ////    synchronizer_terminate::on_success);

    for (size_t host = 0; host < batch_size_; ++host)
        new_connect(handler);
}

void session_batch::new_connect(channel_handler handler)
{
    if (stopped())
    {
        handler(error::channel_stopped, nullptr);
        return;
    }

    network_.fetch_address(BIND3(start_connect, _1, _2, handler));
}

void session_batch::start_connect(const code& ec, const authority& host,
    channel_handler handler)
{
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

    const auto connector = create_connector();

    ////pend(connector);

    // CONNECT
    connector->connect(host, BIND4(handle_connect, _1, _2, connector, handler));
}

void session_batch::handle_connect(const code& ec,
    channel::ptr channel, connector::ptr connector, channel_handler handler)
{
    ////unpend(connector);

    if (ec)
    {
        handler(ec, nullptr);
        return;
    }

    // This is the end of the connect sequence.
    handler(error::success, channel);
}

} // namespace network
} // namespace libbitcoin
