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
#include <bitcoin/network/protocols/protocol_version_70002.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_version_70001.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_version_70002

using namespace bc::system;
using namespace bc::network::messages;
using namespace std::placeholders;

static const std::string protocol_name = "version";
static const std::string insufficient_version = "insufficient-version";
static const std::string insufficient_services = "insufficient-services";

protocol_version_70002::protocol_version_70002(const session& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol_version_70002(session, channel,
        session.settings().services_minimum,
        session.settings().services_maximum,
        session.settings().relay_transactions)
{
}

protocol_version_70002::protocol_version_70002(const session& session,
    const channel::ptr& channel, uint64_t minimum_services,
    uint64_t maximum_services, bool relay) NOEXCEPT
  : protocol_version_70001(session, channel, minimum_services,
      maximum_services, relay),
    tracker<protocol_version_70002>(session.log())
{
}

const std::string& protocol_version_70002::name() const NOEXCEPT
{
    return protocol_name;
}

// Start.
// ----------------------------------------------------------------------------

void protocol_version_70002::shake(result_handler&& handle_event) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_70002");

    if (started())
        return;

    SUBSCRIBE2(reject, handle_receive_reject, _1, _2);

    protocol_version_70001::shake(std::move(handle_event));
}

// Outgoing [(in)sufficient_peer => send_reject].
// ----------------------------------------------------------------------------

void protocol_version_70002::rejection(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_70002");

    // Handshake completion may result before completion of this send (okay).
    if (ec == error::insufficient_peer)
    {
        SEND1((reject{ version::command, reject::reason_code::obsolete }),
            handle_send, _1);
    }
    else if (ec == error::protocol_violation)
    {
        SEND1((reject{ version::command, reject::reason_code::duplicate }),
            handle_send, _1);
    }

    return protocol_version_70001::rejection(ec);
}

// Incoming [receive_reject => log].
// ----------------------------------------------------------------------------

void protocol_version_70002::handle_receive_reject(const code& ec,
    const reject::ptr& LOG_ONLY(reject)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_70002");

    if (stopped(ec))
        return;

    LOG("Reject message '" << reject->message << "' ("
        << static_cast<uint16_t>(reject->code) << ") from [" << authority()
        << "] with reason: " << reject->reason);
}

} // namespace network
} // namespace libbitcoin
