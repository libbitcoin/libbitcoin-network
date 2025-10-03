/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/protocols/protocol_version_70002.hpp>

#include <utility>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/p2p/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_version_70001.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_version_70002

using namespace system;
using namespace network::messages::p2p;
using namespace std::placeholders;

protocol_version_70002::protocol_version_70002(const session::ptr& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol_version_70002(session, channel,
        session->settings().services_minimum,
        session->settings().services_maximum,
        session->settings().enable_relay)
{
}

protocol_version_70002::protocol_version_70002(const session::ptr& session,
    const channel::ptr& channel,
    uint64_t minimum_services,
    uint64_t maximum_services,
    bool relay) NOEXCEPT
  : protocol_version_70001(session, channel, minimum_services, maximum_services,
      relay),
    tracker<protocol_version_70002>(session->log)
{
}

// Start.
// ----------------------------------------------------------------------------

void protocol_version_70002::shake(result_handler&& handle_event) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_70002");

    if (started())
        return;

    SUBSCRIBE_CHANNEL(reject, handle_receive_reject, _1, _2);

    protocol_version_70001::shake(std::move(handle_event));
}

// Outgoing [(in)sufficient_peer => send_reject].
// ----------------------------------------------------------------------------
// Reject is the only difference at protocol level 70002.

void protocol_version_70002::rejection(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_70002");

    // Handshake completion may result before completion of this send (okay).
    if (ec == error::peer_insufficient)
    {
        SEND((reject{ version::command, reject::reason_code::obsolete }),
            handle_send, _1);
    }
    else if (ec == error::peer_unsupported)
    {
        SEND((reject{ version::command, reject::reason_code::undefined,
            "services" }),
            handle_send, _1);
    }
    else if (ec == error::protocol_violation)
    {
        SEND((reject{ version::command, reject::reason_code::duplicate }),
            handle_send, _1);
    }
    else if (ec == error::peer_timestamp)
    {
        SEND((reject{ version::command, reject::reason_code::undefined,
            "timestamp" }), handle_send, _1);
    }

    protocol_version_70001::rejection(ec);
}

// Incoming [receive_reject => log].
// ----------------------------------------------------------------------------

bool protocol_version_70002::handle_receive_reject(const code& ec,
    const reject::cptr& LOG_ONLY(message)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_70002");

    if (stopped(ec))
        return false;

    LOGP("Reject message '" << message->message << "' ("
        << static_cast<uint16_t>(message->code) << ") from [" << authority()
        << "] with reason: " << message->reason);

    return true;
}

} // namespace network
} // namespace libbitcoin
