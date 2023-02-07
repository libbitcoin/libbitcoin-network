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

using namespace system;
using namespace messages;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

protocol_address_31402::protocol_address_31402(const session& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    inbound_(session.inbound()),
    request_(!inbound_ && settings().outbound_enabled()),
    received_(false),
    sent_(false),
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
// TODO: as peers connect inbound, broadcast new address.

messages::address_item protocol_address_31402::self() const NOEXCEPT
{
    return settings().self.to_address_item(unix_time(),
        settings().services_maximum);
}

void protocol_address_31402::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_31402");

    if (started())
        return;

    // Advertise self if configured for inbound and valid self address.
    if (settings().advertise_enabled())
    {
        SEND1(messages::address{ { self() } }, handle_send, _1);
    }

    // Always capture address and get_address (so can accept and/or reject).
    SUBSCRIBE2(messages::address, handle_receive_address, _1, _2);
    SUBSCRIBE2(get_address, handle_receive_get_address, _1, _2);

    // Do not accept addresses from inbound channels (too injectable).
    // Do not request addresses if not configured for outbound connections.
    if (request_)
    {
        SEND1(get_address{}, handle_send, _1);
    }

    protocol::start();
}

// Inbound (store addresses).
// ----------------------------------------------------------------------------

void protocol_address_31402::handle_receive_address(const code& ec,
    const address::cptr& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_31402");

    if (stopped(ec))
        return;

    const auto& items = message->addresses;
    const auto singleton = is_one(items.size());

    // If not requested, disallow multiple values and multiple messages.
    if (!request_ && (!singleton || received_))
    {
        // /bitnodes.earn.com:0.1/ sends unsolicited addresses.
        LOG("Unsolicited addresses from [" << authority() << "]");
        stop(error::protocol_violation);
        return;
    }

    received_ = true;

    // Do not store redundant adresses, address() is own checked out address.
    if (singleton && (items.front() == address()))
    {
        ////LOG("Dropping redundant address from [" << authority() << "]");
        return;
    }

    // This will accept previously rejected addresses (state not retained).
    save(message, BIND3(handle_save_addresses, _1, _2, items.size()));
}

void protocol_address_31402::handle_save_addresses(const code& ec,
    size_t LOG_ONLY(accepted), size_t LOG_ONLY(count)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_31402");

    if (stopped(ec))
        return;

    if (count > 10u)
    {
        LOG("Accepted (" << accepted << " of " << count << ") "
            "addresses from [" << authority() << "].");
    }
}

// Outbound (fetch and send addresses).
// ----------------------------------------------------------------------------

void protocol_address_31402::handle_receive_get_address(const code& ec,
    const get_address::cptr&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_31402");

    if (stopped(ec))
        return;

    // Limit get_address requests to one per session.
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
    const address::cptr& message) NOEXCEPT
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
