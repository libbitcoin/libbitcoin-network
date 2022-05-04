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

    /// Construct an instance (network should be started).
    session_outbound(p2p& network) noexcept;

    /// Start configured number of connections (call from network strand).
    void start(result_handler handler) noexcept override;

protected:
    /// The channel is outbound (do not pend the nonce).
    bool inbound() const noexcept override;

    /// Notify subscribers on channel start.
    bool notify() const noexcept override;

    /// Overridden to change version protocol (base calls from channel strand).
    void attach_handshake(const channel::ptr& channel,
        result_handler handle_started) const noexcept override;

    /// Overridden to change channel protocols (base calls from channel strand).
    void attach_protocols(const channel::ptr& channel) const noexcept override;

    /// Start outbound connections based on config (call from network strand).
    virtual void start_connect(connectors_ptr connectors) noexcept;

private:
    typedef std::shared_ptr<size_t> count_ptr;

    void handle_started(const code& ec, result_handler handler) noexcept;
    void handle_connect(const code& ec, channel::ptr channel,
        connectors_ptr connectors) noexcept;

    void handle_channel_start(const code& ec, channel::ptr channel) noexcept;
    void handle_channel_stop(const code& ec, connectors_ptr connectors) noexcept;

    void do_one(const code& ec, const config::authority& host,
        connector::ptr connector, channel_handler handler) noexcept;
    void handle_one(const code& ec, channel::ptr channel,
        count_ptr count, connectors_ptr connectors,
        channel_handler handler) noexcept;

    // This is thread safe.
    const size_t batch_;
};

} // namespace network
} // namespace libbitcoin

#endif
