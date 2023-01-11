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
    DEFAULT4(session_outbound);

    typedef std::shared_ptr<session_outbound> ptr;

    /// Construct an instance (network should be started).
    session_outbound(p2p& network) NOEXCEPT;

    /// Start configured number of connections (call from network strand).
    void start(result_handler&& handler) NOEXCEPT override;

protected:
    /// The channel is outbound (do not pend the nonce).
    bool inbound() const NOEXCEPT override;

    /// Notify subscribers on channel start.
    bool notify() const NOEXCEPT override;

    /// Overridden to change version protocol (base calls from channel strand).
    void attach_handshake(const channel::ptr& channel,
        result_handler&& handle_started) const NOEXCEPT override;

    /// Overridden to change channel protocols (base calls from channel strand).
    void attach_protocols(const channel::ptr& channel) const NOEXCEPT override;

    /// Start outbound connections based on config (called from start).
    virtual void start_connect(const connectors_ptr& connectors) NOEXCEPT;

private:
    typedef std::shared_ptr<size_t> count_ptr;

    void handle_started(const code& ec, const result_handler& handler) NOEXCEPT;
    void handle_connect(const code& ec, const channel::ptr& channel,
        const connectors_ptr& connectors) NOEXCEPT;

    void handle_channel_start(const code& ec, const channel::ptr& channel) NOEXCEPT;
    void handle_channel_stop(const code& ec, const connectors_ptr& connectors) NOEXCEPT;

    void do_one(const code& ec, const config::authority& host,
        const connector::ptr& connector, const channel_handler& handler) NOEXCEPT;
    void handle_one(const code& ec, const channel::ptr& channel,
        const count_ptr& count, const connectors_ptr& connectors,
        const channel_handler& handler) NOEXCEPT;
};

} // namespace network
} // namespace libbitcoin

#endif
