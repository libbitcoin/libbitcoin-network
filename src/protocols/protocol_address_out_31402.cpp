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
#include <bitcoin/network/protocols/protocol_address_out_31402.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_address_out_31402

using namespace system;
using namespace messages;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

protocol_address_out_31402::protocol_address_out_31402(
    const session::ptr& session, const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    tracker<protocol_address_out_31402>(session->log)
{
}

// Start.
// ----------------------------------------------------------------------------

void protocol_address_out_31402::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_out_31402");

    if (started())
        return;

    // Advertise self if configured for inbound and with self address(es).
    if (settings().advertise_enabled())
    {
        SEND(selfs(), handle_send, _1);
    }

    SUBSCRIBE_CHANNEL(get_address, handle_receive_get_address, _1, _2);
    protocol::start();
}

// Outbound (fetch and send addresses).
// ----------------------------------------------------------------------------

bool protocol_address_out_31402::handle_receive_get_address(const code& ec,
    const get_address::cptr&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_out_31402");

    if (stopped(ec))
        return false;

    // Limit get_address requests to one per session.
    if (sent_)
    {
        LOGP("Ignoring duplicate address request from [" << authority() << "]");
        ////stop(error::protocol_violation);
        return true;
    }

    fetch(BIND(handle_fetch_address, _1, _2));
    sent_ = true;

    LOGP("Relay start [" << authority() << "].");
    SUBSCRIBE_BROADCAST(address, handle_broadcast_address, _1, _2, _3);
    return true;
}

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)

void protocol_address_out_31402::handle_fetch_address(const code& ec,
    const address::cptr& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_out_31402");

    if (stopped(ec))
        return;

    LOGP("Sending (" << message->addresses.size() << ") addresses to "
        "[" << authority() << "]");

    SEND(*message, handle_send, _1);
}

// ----------------------------------------------------------------------------

bool protocol_address_out_31402::handle_broadcast_address(const code& ec,
    const address::cptr& message, uint64_t sender) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_out_31402");

    if (stopped(ec))
    {
        LOGP("Relay stop [" << authority() << "].");
        return false;
    }

    if (sender == identifier())
    {
        LOGP("Relay self [" << authority() << "].");
        return true;
    }

    LOGP("Relay (" << message->addresses.size() << ") addresses to ["
        << authority() << "].");

    SEND(*message, handle_send, _1);
    return true;
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
