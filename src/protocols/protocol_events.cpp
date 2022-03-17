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
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_events

using namespace bc::system;
using namespace std::placeholders;

protocol_events::protocol_events(const session& session, channel::ptr channel)
  : protocol(session, channel),
    handler_([](const code&) {})
{
}

// Properties.
// ----------------------------------------------------------------------------

bool protocol_events::stopping(const code& ec) const
{
    // The service stop code may also make its way into protocol handlers.
    return stopped() || ec == error::channel_stopped ||
        ec == error::service_stopped;
}

// Start.
// ----------------------------------------------------------------------------

void protocol_events::start()
{
    BC_ASSERT_MSG(stranded(), "stranded");
}

// START COMPLETES WITHOUT INVOKING THE HANDLER.
void protocol_events::start(result_handler handle_event)
{
    BC_ASSERT_MSG(stranded(), "stranded");
    handler_ = handle_event;
}

// Stop.
// ----------------------------------------------------------------------------

void protocol_events::handle_stopped(const code& ec)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (!stopping(ec))
    {
        LOG_VERBOSE(LOG_NETWORK)
            << "Stop protocol_" << name() << " on [" << authority() << "] "
            << ec.message() << std::endl;
    }

    // Event handlers can depend on this code for channel stop.
    set_event(error::channel_stopped);
}

// Set Event.
// ----------------------------------------------------------------------------

void protocol_events::set_event(const code& ec)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (stopping(ec))
    {
        stop(ec);
        return;
    }
    
    handler_(ec);
}

} // namespace network
} // namespace libbitcoin
