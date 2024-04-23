/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_reject_70002

using namespace system;
using namespace messages;
using namespace std::placeholders;

// This protocol creates log overflow DOS vector, and is not in widespread use.
protocol_reject_70002::protocol_reject_70002(const session::ptr& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    tracker<protocol_reject_70002>(session->log)
{
}

// Start.
// ----------------------------------------------------------------------------

void protocol_reject_70002::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_reject_70002");

    if (started())
        return;

    SUBSCRIBE_CHANNEL(reject, handle_receive_reject, _1, _2);

    protocol::start();
}

// Inbound (log).
// ----------------------------------------------------------------------------

std::string protocol_reject_70002::get_hash(const reject& message) const NOEXCEPT
{
    return message.message == block::command ||
        message.message == transaction::command ?
        system::encode_hash(message.hash) : std::string{ "n/a" };
}

bool protocol_reject_70002::handle_receive_reject(const code& ec,
    const reject::cptr& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_reject_70002");

    if (stopped(ec))
        return false;

    // vesion message rejection is handled in protocol_version_70002, however
    // if received here (outside of handshake), a protocol error is implied.
    if (message->message == version::command)
    {
        LOGR("Version reject after handshake [" << authority() << "]");
        stop(error::protocol_violation);
        return false;
    }

    // system::serialize require for uint8_t serialization.
    LOGP("Reject from [" << authority() << "]..."
        << "\ncode   : " << system::serialize(reject::reason_to_byte(
                            message->code))
        << "\nmessage: " << message->message
        << "\nreason : " << message->reason
        << "\nhash   : " << get_hash(*message));

    return true;
}

} // namespace network
} // namespace libbitcoin
