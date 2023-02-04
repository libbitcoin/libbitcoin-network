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
#include <bitcoin/network/protocols/protocol_address_31402.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_address_31402

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

protocol_address_31402::protocol_address_31402(const session& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    sent_(false),
    received_(false),
    tracker<protocol_address_31402>(session.log())
{
}

const std::string& protocol_address_31402::name() const NOEXCEPT
{
    static const std::string protocol_name = "address";
    return protocol_name;
}

// Start.
// ----------------------------------------------------------------------------

void protocol_address_31402::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_31402");

    if (started())
        return;

    // Own address (from config) is sent unsolicited if inbound enabled.
    if (!is_zero(settings().self.port()) &&
        !is_zero(settings().inbound_connections))
    {
        SEND1(address{ { settings().self.to_address_item(unix_time(),
            settings().services_maximum) } }, handle_send, _1);
    }

    // If no address pool, do not ask for them or capture requests for them.
    if (!is_zero(settings().host_pool_capacity))
    {
        SUBSCRIBE2(address, handle_receive_address, _1, _2);
        SUBSCRIBE2(get_address, handle_receive_get_address, _1, _2);

        // TODO: this could be gated on state of the address pool.
        // Satoshi peers send addr anyway, despite getaddr not being sent.
        SEND1(get_address{}, handle_send, _1);
    }

    protocol::start();
}

// Inbound (store addresses).
// ----------------------------------------------------------------------------

void protocol_address_31402::handle_receive_address(const code& ec,
    const address::ptr& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_31402");

    if (stopped(ec))
        return;

    // Allow only one singleton address message if addresses not requested.
    // Allows peer to make one unsolicited broadcast of desire for incoming.
    if (is_zero(settings().host_pool_capacity) &&
        (received_ || message->addresses.size() > one))
    {
        LOG("Unsolicited address batch received from [" << authority() << "]");
        stop(error::protocol_violation);
        return;
    }

    save(message, BIND2(handle_save_addresses, _1, message->addresses.size()));
    received_ = true;
}

void protocol_address_31402::handle_save_addresses(const code& ec,
    size_t LOG_ONLY(count)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_31402");

    if (stopped(ec))
        return;

    LOG("Received (" << count << ") addresses from [" << authority() << "]");
}

// Outbound (fetch and send addresses).
// ----------------------------------------------------------------------------

void protocol_address_31402::handle_receive_get_address(const code& ec,
    const get_address::ptr&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_31402");

    if (stopped(ec))
        return;

    // Limit requests to one per session (fingerprinting).
    if (sent_)
    {
        LOG("Ignoring duplicate address request from [" << authority() << "]");
        return;
    }

    fetch(BIND2(handle_fetch_addresses, _1, _2));
    sent_ = true;
}

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

void protocol_address_31402::handle_fetch_addresses(const code& ec,
    const address::ptr& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_31402");

    if (stopped(ec))
        return;

    LOG("Sending (" << message->addresses.size() << ") addresses to "
        "[" << authority() << "]");

    SEND1(*message, handle_send, _1);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
