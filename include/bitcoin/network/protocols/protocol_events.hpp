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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_EVENTS_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_EVENTS_HPP

#include <atomic>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>

namespace libbitcoin {
namespace network {

/// Base class for stateful protocol implementation, thread and lock safe.
class BCT_API protocol_events
  : public protocol
{
protected:
    protocol_events(channel::ptr channel);

    virtual void start();

    /// The handler is invoked at each completion event.
    virtual void start(result_handler handle_event);

    /// Invoke the event handler.
    virtual void set_event(const code& ec);

    /// Determine if the event handler has been cleared.
    virtual bool stopped() const;

    /// Determine if the code is a stop code or the handler has been cleared.
    virtual bool stopped(const code& ec) const;

private:
    void handle_stopped(const code& ec);

    result_handler handler_;
    std::atomic<bool> stopped_;
};

} // namespace network
} // namespace libbitcoin

#endif
