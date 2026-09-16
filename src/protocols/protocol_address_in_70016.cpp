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
#include <bitcoin/network/protocols/protocol_address_in_70016.hpp>

#include <bitcoin/network/channels/channels.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_address_in_209.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_address_in_70016

using namespace system;
using namespace messages::peer;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

protocol_address_in_70016::protocol_address_in_70016(
    const session::ptr& session, const channel::ptr& channel) NOEXCEPT
  : protocol_address_in_209(session, channel),
    tracker<protocol_address_in_70016>(session->log)
{
}

// Start.
// ----------------------------------------------------------------------------

// A peer that has not been sent send_address_v2 may still send address.
void protocol_address_in_70016::subscribe_address() NOEXCEPT
{
    SUBSCRIBE_CHANNEL(address_v2, handle_receive_address_v2, _1, _2);
    protocol_address_in_209::subscribe_address();
}

// Inbound (store addresses).
// ----------------------------------------------------------------------------

bool protocol_address_in_70016::handle_receive_address_v2(const code& ec,
    const address_v2::cptr& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_in_70016");

    if (stopped(ec))
        return false;

    // The encodings differ but the messages are otherwise identical.
    return handle_receive_address(ec, to_shared<address>(message->addresses));
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
