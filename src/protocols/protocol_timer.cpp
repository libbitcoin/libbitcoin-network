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
#include <bitcoin/network/protocols/protocol_timer.hpp>

#include <functional>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_events.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_timer
using namespace bc::system;
using namespace std::placeholders;

protocol_timer::protocol_timer(channel::ptr channel, const duration& timeout,
    bool perpetual)
  : protocol_events(channel),
    timeout_(timeout),
    perpetual_(perpetual),
    timer_(std::make_shared<deadline<asio::strand>>(channel->strand()))
{
}

// Start sequence.
// ----------------------------------------------------------------------------

// protected:
void protocol_timer::start(event_handler handle_event)
{
    // EVENTS START COMPLETES WITHOUT INVOKING THE HANDLER.
    // protocol_events retains this handler to be invoked multiple times.
    // handle_event is invoked on the channel thread.
    protocol_events::start(BIND2(handle_notify, _1, handle_event));
    reset_timer();
}

void protocol_timer::handle_notify(const code& ec,
    event_handler handler)
{
    if (ec == error::channel_stopped)
        timer_->stop();

    handler(ec);
}

// Timer.
// ----------------------------------------------------------------------------

// protected:
void protocol_timer::reset_timer()
{
    if (stopped())
        return;

    timer_->start(BIND1(handle_timer, _1), timeout_);
}

void protocol_timer::handle_timer(const code& ec)
{
    if (stopped())
        return;

    LOG_VERBOSE(LOG_NETWORK)
        << "Fired protocol_" << name() << " timer on [" << authority() << "] "
        << ec.message();

    set_event(error::channel_timeout);

    // Reset timer until the channel is stopped.
    if (perpetual_)
        reset_timer();
}

} // namespace network
} // namespace libbitcoin
