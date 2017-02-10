/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/protocols/protocol_reject_70002.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocol_events.hpp>

namespace libbitcoin {
namespace network {

#define NAME "reject"
#define CLASS protocol_reject_70002

using namespace bc::message;
using namespace std::placeholders;

protocol_reject_70002::protocol_reject_70002(p2p& network,
    channel::ptr channel)
  : protocol_events(network, channel, NAME),
    CONSTRUCT_TRACK(protocol_reject_70002)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

void protocol_reject_70002::start()
{
    protocol_events::start();

    SUBSCRIBE2(reject, handle_receive_reject, _1, _2);
}

// Protocol.
// ----------------------------------------------------------------------------

// TODO: mitigate log fill DOS.
bool protocol_reject_70002::handle_receive_reject(const code& ec,
    reject_const_ptr reject)
{
    if (stopped(ec))
        return false;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure receiving reject from [" << authority() << "] "
            << ec.message();
        stop(error::channel_stopped);
        return false;
    }

    const auto& message = reject->message();

    // Handle these in the version protocol.
    if (message == version::command)
        return true;

    std::string hash;
    if (message == block::command || message == transaction::command)
        hash = " [" + encode_hash(reject->data()) + "].";

    const auto code = reject->code();
    LOG_DEBUG(LOG_NETWORK)
        << "Received " << message << " reject (" << static_cast<uint16_t>(code)
        << ") from [" << authority() << "] '" << reject->reason()
        << "'" << hash;
    return true;
}

} // namespace network
} // namespace libbitcoin
