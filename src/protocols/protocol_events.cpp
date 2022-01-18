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
#include <bitcoin/network/protocols/protocol_events.hpp>

#include <functional>
#include <bitcoin/system.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_events

using namespace bc::system;
using namespace std::placeholders;

protocol_events::protocol_events(channel::ptr channel)
  : protocol(channel)
{
}

// Properties.
// ----------------------------------------------------------------------------

bool protocol_events::stopped() const
{
    // Used for context-free stop testing.
    return !handler_.load();
}

bool protocol_events::stopped(const code& ec) const
{
    // The service stop code may also make its way into protocol handlers.
    return stopped() || ec == error::channel_stopped ||
        ec == error::service_stopped;
}

// Start.
// ----------------------------------------------------------------------------

void protocol_events::start()
{
    const auto nop = [](const code&){};
    start(nop);
}

// START COMPLETES WITHOUT INVOKING THE HANDLER.
void protocol_events::start(event_handler handle_event)
{
    handler_.store(handle_event);
}

// Stop.
// ----------------------------------------------------------------------------

void protocol_events::handle_stopped(const code& ec)
{
    if (!stopped(ec))
    {
        LOG_VERBOSE(LOG_NETWORK)
            << "Stop protocol_" << name() << " on [" << authority() << "] "
            << ec.message();
    }

    // Event handlers can depend on this code for channel stop.
    set_event(error::channel_stopped);
}

// Set Event.
// ----------------------------------------------------------------------------

void protocol_events::set_event(const code& ec)
{
    // If already stopped.
    auto handler = handler_.load();
    if (!handler)
        return;

    // If stopping but not yet cleared, clear event handler now.
    if (stopped(ec))
        handler_.store(nullptr);

    // Invoke event handler.
    handler(ec);
}

} // namespace network
} // namespace libbitcoin
