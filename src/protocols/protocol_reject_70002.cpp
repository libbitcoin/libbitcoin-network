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
#include <bitcoin/network/protocols/protocol_reject_70002.hpp>

#include <cstdint>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_reject_70002
static const std::string protocol_name = "reject";

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

// This protocol creates log overflow DOS vector, and is not in widespread use.
protocol_reject_70002::protocol_reject_70002(const session& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel)
{
}

const std::string& protocol_reject_70002::name() const NOEXCEPT
{
    return protocol_name;
}

// Start.
// ----------------------------------------------------------------------------

void protocol_reject_70002::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_reject_70002");

    if (started())
        return;

    SUBSCRIBE2(reject, handle_receive_reject, _1, _2);

    protocol::start();
}

// Inbound (log).
// ----------------------------------------------------------------------------

void protocol_reject_70002::handle_receive_reject(const code& ec,
    const reject::ptr& reject) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_reject_70002");

    if (stopped(ec))
        return;

    const auto& message = reject->message;

    // vesion message rejection is handled in protocol_version_70002, however
    // if received here (outside of handshake), a protocol error is implied.
    if (reject->message == version::command)
    {
        // TODO: log protocol violation.
        stop(error::protocol_violation);
        return;
    }

    std::string hash;
    if (message == block::command || message == transaction::command)
        hash = " [" + encode_hash(reject->hash) + "].";

    ////LOG_DEBUG(LOG_NETWORK)
    ////    << "Received " << message << " reject ("
    ////    << static_cast<uint16_t>(reject->code) << ") from ["
    ////    << authority() << "] '" << reject->reason << "'" << hash << std::endl;
}

} // namespace network
} // namespace libbitcoin
