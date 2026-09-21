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
#include <bitcoin/network/protocols/protocol_address_out_70016.hpp>

#include <bitcoin/network/channels/channels.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_address_out_209.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_address_out_70016

using namespace system;
using namespace messages::peer;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

protocol_address_out_70016::protocol_address_out_70016(
    const session::ptr& session, const channel::ptr& channel) NOEXCEPT
  : protocol_address_out_209(session, channel),
    tracker<protocol_address_out_70016>(session->log)
{
}

// Outbound (fetch and send addresses).
// ----------------------------------------------------------------------------

// The v2 protocol represents all address networks.
void protocol_address_out_70016::send_addresses(
    const address& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_out_70016");
    SEND((address_v2{ message.addresses }), handle_send, _1);
}

void protocol_address_out_70016::notify_addresses(
    const address& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_out_70016");
    NOTIFY((address_v2{ message.addresses }), handle_send, _1);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
