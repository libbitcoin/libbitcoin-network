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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_TIMER_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_TIMER_HPP

#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_events.hpp>

namespace libbitcoin {
namespace network {

/// Base class for timed protocol implementation.
class BCT_API protocol_timer
  : public protocol_events
{
protected:

    protocol_timer(channel::ptr channel, const duration& timeout,
        bool perpetual=true);

    virtual void start(result_handler handle_event);

    // Expose polymorphic start method from base.
    using protocol_events::start;

protected:
    void reset_timer();

private:
    void handle_timer(const code& ec);
    void handle_notify(const code& ec, result_handler handler);

    const duration timeout_;
    const bool perpetual_;
    deadline::ptr timer_;
};

} // namespace network
} // namespace libbitcoin

#endif
