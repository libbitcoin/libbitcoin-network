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

#include <functional>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/protocols/protocol_events.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_address_31402
static const std::string protocol_name = "address";

using namespace bc;
using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

protocol_address_31402::protocol_address_31402(const session& session,
    channel::ptr channel)
  : protocol_events(session, channel)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

void protocol_address_31402::start()
{
    BC_ASSERT_MSG(stranded(), "stranded");

    // Events start completes without invoking the handler.
    // Must have a handler to capture a shared self pointer in stop subscriber.
    protocol_events::start(BIND1(handle_stop, _1));

    if (!is_zero(settings().self.port()))
    {
        SEND2(address{ { settings().self.to_address_item() } }, handle_send,
            _1, address::command);
    }

    // If we can't store addresses we don't ask for or handle them.
    if (is_zero(settings().host_pool_capacity))
        return;

    SUBSCRIBE2(address, handle_receive_address, _1, _2);
    SUBSCRIBE2(get_address, handle_receive_get_address, _1, _2);
    SEND2(get_address{}, handle_send, _1, get_address::command);
}

// Protocol.
// ----------------------------------------------------------------------------

void protocol_address_31402::handle_receive_address(const code& ec,
    address::ptr message)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (stopped(ec))
        return;

    LOG_VERBOSE(LOG_NETWORK)
        << "Storing addresses from [" << authority() << "] ("
        << message->addresses.size() << ")" << std::endl;

    // TODO: manage timestamps (active channels are connected < 3 hours ago).
    saves(message->addresses);
}

void protocol_address_31402::handle_receive_get_address(const code& ec,
    get_address::ptr)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (stopped(ec))
        return;

    fetches(BIND2(handle_fetch_addresses, _1, _2));
}

void protocol_address_31402::handle_fetch_addresses(const code& ec,
    const messages::address_items& hosts)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (!ec)
    {
        SEND2(address{ hosts }, handle_send, _1, address::command);

        LOG_DEBUG(LOG_NETWORK)
            << "Sending addresses to [" << authority() << "] ("
            << hosts.size() << ")" << std::endl;
    }
}

void protocol_address_31402::handle_stop(const code&)
{
    BC_ASSERT_MSG(stranded(), "stranded");
}

const std::string& protocol_address_31402::name() const
{
    return protocol_name;
}

} // namespace network
} // namespace libbitcoin
