/**
 * Copyright (c) 2011-2016 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/network/sessions/session_batch.hpp>

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/utility/connector.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_batch
#define NAME "session_batch"

using namespace bc::config;
using namespace std::placeholders;

session_batch::session_batch(p2p& network, bool notify_on_connect)
  : session(network, notify_on_connect),
    batch_size_(std::max(settings_.connect_batch_size, 1u))
{
}

// Connect sequence.
// ----------------------------------------------------------------------------

// protected:
void session_batch::connect(connector::ptr connect, channel_handler handler)
{
    static const auto mode = synchronizer_terminate::on_success;

    const channel_handler complete_handler =
        BIND3(handle_connect, _1, _2, handler);

    const channel_handler join_handler =
        synchronize(complete_handler, batch_size_, NAME "_join", mode);

    for (uint32_t host = 0; host < batch_size_; ++host)
        new_connect(connect, join_handler);
}

void session_batch::new_connect(connector::ptr connect,
    channel_handler handler)
{
    if (stopped())
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Suspended batch connection.";
        handler(error::channel_stopped, nullptr);
        return;
    }

    fetch_address(BIND4(start_connect, _1, _2, connect, handler));
}

void session_batch::start_connect(const code& ec, const authority& host,
    connector::ptr connect, channel_handler handler)
{
    if (stopped() || ec == error::service_stopped)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Batch session stopped while starting.";
        handler(error::service_stopped, nullptr);
        return;
    }

    // This termination prevents a tight loop in the empty address pool case.
    if (ec)
    {
        LOG_WARNING(LOG_NETWORK)
            << "Failure fetching new address: " << ec.message();
        handler(ec, nullptr);
        return;
    }

    // This creates a tight loop in the case of a small address pool.
    if (blacklisted(host))
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Fetched blacklisted address [" << host << "] ";
        handler(error::address_blocked, nullptr);
        return;
    }

    LOG_DEBUG(LOG_NETWORK)
        << "Connecting to [" << host << "]";

    // CONNECT
    connect->connect(host, handler);
}

// It is common for no connections, one connection or multiple connections to
// succeed, but only zero or one will reach this point. Other connections that
// succeed fall out of scope causing the socket to close on destruct.
void session_batch::handle_connect(const code& ec, channel::ptr channel,
    channel_handler handler)
{
    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failed to connect after (" << batch_size_
            << ") concurrent attempts: " << ec.message();
        handler(ec, nullptr);
        return;
    }

    LOG_DEBUG(LOG_NETWORK)
        << "Connected to [" << channel->authority() << "]";

    // This is the end of the connect sequence.
    handler(error::success, channel);
}

} // namespace network
} // namespace libbitcoin
