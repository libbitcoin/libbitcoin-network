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
#include <bitcoin/network/protocols/protocol_address_in_31402.hpp>

#include <algorithm>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_address_in_31402

using namespace system;
using namespace messages;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

protocol_address_in_31402::protocol_address_in_31402(const session& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    inbound_(session.inbound()),
    tracker<protocol_address_in_31402>(session.log())
{
}

// Start.
// ----------------------------------------------------------------------------

void protocol_address_in_31402::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_in_31402");

    if (started())
        return;

    // Always allow a singleton unrequested address (advertisement).
    SUBSCRIBE2(address, handle_receive_address, _1, _2);

    // Do not request addresses from inbound channels.
    if (!inbound_)
    {
        SEND1(get_address{}, handle_send, _1);
    }

    protocol::start();
}

// Inbound (store addresses).
// ----------------------------------------------------------------------------

address::cptr protocol_address_in_31402::filter(
    const address_items& items) const NOEXCEPT
{
    // CLang doesn't like emplacement with default constructors, so use new.
    BC_PUSH_WARNING(NO_NEW_OR_DELETE)
    const auto message = std::shared_ptr<address>(new address{ items });
    BC_POP_WARNING()

    std::erase_if(message->addresses, [&](const auto& address) NOEXCEPT
    {
        return !is_specified(address)
            || settings().disabled(address)
            || settings().insufficient(address)
            || settings().unsupported(address)
            || settings().peered(address)
            || settings().blacklisted(address)
            || !settings().whitelisted(address);
    });

    return message;
}

void protocol_address_in_31402::handle_receive_address(const code& ec,
    const address::cptr& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_in_31402");

    if (stopped(ec))
        return;

    // Do not accept multiple addresses from inbound channels.
    const auto size = message->addresses.size();
    if (inbound_ && (received_ || !is_one(size)))
    {
        LOG("Ignoring unsolicited addresses from [" << authority() << "]");
        ////stop(error::protocol_violation);
        return;
    }

    received_ = true;
    if (message->addresses.front() == outbound())
    {
        // This is very common.
        ////LOG("Dropping redundant address from [" << authority() << "]");
        return;
    }

    const auto filtered = filter(message->addresses);

    // This allows previously-rejected addresses.
    save(filtered,
        BIND4(handle_save_address, _1, _2, filtered->addresses.size(), size));
}

void protocol_address_in_31402::handle_save_address(const code& ec,
    size_t LOG_ONLY(accepted), size_t LOG_ONLY(filtered),
    size_t LOG_ONLY(start)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_in_31402");

    if (stopped(ec))
        return;

    LOG("Accepted (" << start << ">" << filtered << ">" << accepted << ") "
        "addresses from [" << authority() << "].");
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
