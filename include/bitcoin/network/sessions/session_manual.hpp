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
#ifndef LIBBITCOIN_NETWORK_SESSION_MANUAL_HPP
#define LIBBITCOIN_NETWORK_SESSION_MANUAL_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/session.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class p2p;

/// Manual connections session, thread safe.
class BCT_API session_manual
  : public session, track<session_manual>
{
public:
    typedef std::shared_ptr<session_manual> ptr;
    typedef std::function<void(const code&, channel::ptr)>
        channel_handler;

    /// Construct an instance.
    session_manual(p2p& network);

    /// Start/stop the manual session.
    void start(result_handler handler) override;

    /// Maintain connection to a node.
    virtual void connect(const std::string& hostname, uint16_t port);

    /// Maintain connection to a node with callback on first connect.
    virtual void connect(const std::string& hostname, uint16_t port,
        channel_handler handler);

    /// Maintain connection to a node.
    virtual void connect(const config::authority& host,
        channel_handler handler);

protected:

    /// Override to attach specialized protocols upon channel start.
    void attach_protocols(const channel::ptr& channel) const override;

    /// Override in test.
    virtual void start_connect(const config::authority& host,
        connector::ptr connector, channel_handler handler);

private:
    void handle_started(const code& ec, result_handler handler);
    void handle_connect(const code& ec, channel::ptr channel,
        const config::authority& host, connector::ptr connector,
        channel_handler handler);

    void handle_channel_start(const code& ec, const config::authority& host,
        channel::ptr channel, channel_handler handler);
    void handle_channel_stop(const code& ec, const config::authority& host,
        connector::ptr connector, channel_handler handler);
};

} // namespace network
} // namespace libbitcoin

#endif
