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

#include <string>
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

protocol_address_31402::protocol_address_31402(const session& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    sent_(false),
    track<protocol_address_31402>(session.log())
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

    // Own address message is derived from config, if port is non-zero.
    if (!is_zero(settings().self.port()))
    {
        static const auto self = settings().self.to_address_item();
        SEND1(address{ { self } }, handle_send, _1);
    }

    // If addresses can't be stored don't ask for them.
    if (!is_zero(settings().host_pool_capacity))
    {
        SUBSCRIBE2(address, handle_receive_address, _1, _2);
        SUBSCRIBE2(get_address, handle_receive_get_address, _1, _2);
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

    // TODO: manage timestamps (active channels are connected < 3 hours ago).

    // Protocol handles and logs code.
    saves(message->addresses);
}

// Outbound (fetch and send addresses).
// ----------------------------------------------------------------------------

void protocol_address_31402::handle_receive_get_address(const code& ec,
    const get_address::ptr&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_31402");

    if (stopped(ec))
        return;

    // TODO: log duplicate request (or drop channel).
    if (sent_)
        return;

    fetches(BIND2(handle_fetch_addresses, _1, _2));
}

void protocol_address_31402::handle_fetch_addresses(const code& ec,
    const messages::address_items& addresses) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_address_31402");

    if (stopped(ec))
        return;

    SEND1(address{ addresses }, handle_send, _1);

    // Precludes multiple address requests.
    sent_ = true;
}

} // namespace network
} // namespace libbitcoin
