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
#include <bitcoin/network/protocols/protocol_version_70014.hpp>

#include <bitcoin/network/channels/channels.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_version_70002.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

// sendcmpct (bip152) is sent after verack, so it cannot inform protocol
// attachment. The peer's sendcmpct is handled by the compact protocols.

namespace libbitcoin {
namespace network {

#define CLASS protocol_version_70014

using namespace system;
using namespace network::messages::peer;
using namespace std::placeholders;

protocol_version_70014::protocol_version_70014(const session::ptr& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol_version_70014(session, channel,
        session->network_settings().enable_relay)
{
}

protocol_version_70014::protocol_version_70014(const session::ptr& session,
    const channel::ptr& channel, bool relay) NOEXCEPT
  : protocol_version_70002(session, channel, relay),
    tracker<protocol_version_70014>(session->log)
{
}

// Outgoing [signal compact blocks version 2, low bandwidth (bip152)].
// ----------------------------------------------------------------------------

bool protocol_version_70014::handle_receive_acknowledge(const code& ec,
    const messages::peer::version_acknowledge::cptr& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_70014");

    if (!protocol_version_70002::handle_receive_acknowledge(ec, message))
        return false;

    const auto compact = network_settings().enable_compact;
    if (compact && negotiated_version() >= level::bip152)
    {
        constexpr auto version = send_compact::compact_version_2;
        SEND((send_compact{ false, version }), handle_send, _1);
    }

    return true;
}

} // namespace network
} // namespace libbitcoin
