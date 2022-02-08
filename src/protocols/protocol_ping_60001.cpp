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
#include <bitcoin/network/protocols/protocol_ping_60001.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_ping_31402.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_ping_60001
static const std::string protocol_name = "ping";

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

protocol_ping_60001::protocol_ping_60001(const session& session,
    channel::ptr channel, const duration& heartbeat)
  : protocol_ping_31402(session, channel, heartbeat),
    pending_(false)
{
}

// This is fired by the callback (i.e. base timer and stop handler).
void protocol_ping_60001::send_ping(const code& ec)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (stopped(ec))
        return;

    if (ec && ec != error::channel_timeout)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure in ping timer for [" << authority() << "] "
            << ec.message() << std::endl;
        stop(ec);
        return;
    }

    if (pending_)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Ping latency limit exceeded [" << authority() << "]"
            << std::endl;
        stop(error::channel_timeout);
        return;
    }

    pending_ = true;
    const auto nonce = pseudo_random::next<uint64_t>();
    SUBSCRIBE3(pong, handle_receive_pong, _1, _2, nonce);
    SEND2(ping{ nonce }, handle_send_ping, _1, ping::command);
}

void protocol_ping_60001::handle_send_ping(const code& ec,
    const std::string&)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (stopped(ec))
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure sending ping to [" << authority() << "] "
            << ec.message() << std::endl;
        stop(ec);
        return;
    }
}

void protocol_ping_60001::handle_receive_ping(const code& ec,
    ping::ptr message)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (stopped(ec))
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure getting ping from [" << authority() << "] "
            << ec.message() << std::endl;
        stop(ec);
        return;
    }

    SEND2(pong{ message->nonce }, handle_send, _1, pong::command);
}

void protocol_ping_60001::handle_receive_pong(const code& ec,
    pong::ptr message, uint64_t nonce)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (stopped(ec))
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure getting pong from [" << authority() << "] "
            << ec.message() << std::endl;
        stop(ec);
    }

    pending_ = false;

    if (message->nonce != nonce)
    {
        LOG_WARNING(LOG_NETWORK)
            << "Invalid pong nonce from [" << authority() << "]" << std::endl;
        stop(error::bad_stream);
    }
}

const std::string& protocol_ping_60001::name() const
{
    return protocol_name;
}

} // namespace network
} // namespace libbitcoin
