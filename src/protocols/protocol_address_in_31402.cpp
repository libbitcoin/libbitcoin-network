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
#include <bitcoin/network/log/log.hpp>
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

protocol_address_in_31402::protocol_address_in_31402(
    const session::ptr& session, const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    outbound_(!channel->inbound()),
    tracker<protocol_address_in_31402>(session->log)
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
    SUBSCRIBE_CHANNEL(address, handle_receive_address, _1, _2);

    // Do not request addresses from inbound channels.
    if (outbound_)
    {
        SEND(get_address{}, handle_send, _1);
    }

    protocol::start();
}

// Inbound (store addresses).
// ----------------------------------------------------------------------------

address::cptr protocol_address_in_31402::filter(
    const address_items& items) const NOEXCEPT
{
    const size_t cap = settings().host_pool_capacity;
    const size_t gap = cap - address_count();

    // Take at least the gap or what we can get.
    const size_t minimum = std::min(gap, items.size());

    // Take up to the cap but no more.
    const size_t maximum = std::min(cap, items.size());

    // Returns zero if minimum > maximum.
    const size_t select = pseudo_random::next(minimum, maximum);

    if (is_zero(select))
        return to_shared<address>();

    // CLang doesn't like emplacement with default constructors, so use new.
    BC_PUSH_WARNING(NO_NEW_OR_DELETE)
    const auto message = std::shared_ptr<address>(new address{ items });
    BC_POP_WARNING()

    // Shuffle, reduce, and filter to the target amount.
    pseudo_random::shuffle(message->addresses);
    message->addresses.resize(select);
    std::erase_if(message->addresses, [&](const auto& address) NOEXCEPT
    {
        return settings().excluded(address);
    });

    return message;
}

bool protocol_address_in_31402::handle_receive_address(const code& ec,
    const address::cptr& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_in_31402");

    if (stopped(ec))
        return false;

    // Accept only one advertisement message unless messages requested.
    const auto start_size = message->addresses.size();
    const auto advertisement = first_ && (start_size < maximum_advertisement);
    first_ = false;

    if (!outbound_ && !advertisement)
    {
        LOGP("Ignoring (" << start_size << ") unsolicited addresses from ["
            << authority() << "].");
        ////stop(error::protocol_violation);
        return true;
    }

    // Relay only advertisement of a new peer (trickle creates cycle).
    if (advertisement)
    {
        BROADCAST(address, message);
        LOGP("Relay (" << start_size << ") addresses by ["
            << authority() << "].");
    }

    if (is_one(start_size) && message->addresses.front() == outbound())
    {
        ////LOGP("Dropping redundant address from [" << authority() << "].");
        return true;
    }

    const auto filtered = filter(message->addresses);
    const auto end_size = filtered->addresses.size();

    // This allows previously-rejected addresses.
    save(filtered,
        BIND(handle_save_address, _1, _2, end_size, start_size));

    return true;
}

void protocol_address_in_31402::handle_save_address(const code& ec,
    size_t LOG_ONLY(accepted), size_t LOG_ONLY(filtered),
    size_t LOG_ONLY(start_size)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_in_31402");

    if (stopped(ec))
        return;

    LOGP("Accepted (" << start_size << ">" << filtered << ">" << accepted << ") "
        "addresses from [" << authority() << "].");
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
