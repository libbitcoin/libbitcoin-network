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
#ifndef LIBBITCOIN_NETWORK_SESSION_OUTBOUND_HPP
#define LIBBITCOIN_NETWORK_SESSION_OUTBOUND_HPP

#include <cstddef>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/session.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class p2p;

/// Outbound connections session, thread safe.
class BCT_API session_outbound
  : public session, track<session_outbound>
{
public:
    typedef std::shared_ptr<session_outbound> ptr;

    /// Construct an instance.
    session_outbound(p2p& network);

    /// Start/stop the session.
    virtual void start(result_handler handler) override;
    virtual void stop() override;

protected:
    /// Overridden to attach minimum service level for witness support.
    virtual void attach_handshake(channel::ptr channel,
        result_handler handle_started) const override;

    /// Override to attach specialized protocols upon channel start.
    virtual void attach_protocols(channel::ptr channel,
        result_handler handler={}) const override;

private:
    void new_connection(const code&);

    void handle_started(const code& ec, result_handler handler);
    void handle_connect(const code& ec, channel::ptr channel);

    void handle_channel_stop(const code& ec, channel::ptr channel);
    void handle_channel_start(const code& ec, channel::ptr channel);

    void batch(channel_handler handler);
    void start_batch(const code& ec, const config::authority& host,
        connector::ptr connector, channel_handler handler);
    void handle_batch(const code& ec, channel::ptr channel,
        connectors_ptr connectors, channel_handler complete);

    const size_t batch_;
    size_t count_;
};

} // namespace network
} // namespace libbitcoin

#endif
