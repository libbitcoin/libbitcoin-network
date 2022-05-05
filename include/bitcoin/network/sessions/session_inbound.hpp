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
#ifndef LIBBITCOIN_NETWORK_SESSION_INBOUND_HPP
#define LIBBITCOIN_NETWORK_SESSION_INBOUND_HPP

#include <cstddef>
#include <memory>
#include <vector>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/session.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class p2p;

/// Inbound connections session, thread safe.
class BCT_API session_inbound
  : public session, track<session_inbound>
{
public:
    typedef std::shared_ptr<session_inbound> ptr;

    /// Construct an instance (network should be started).
    session_inbound(p2p& network) noexcept;

    /// Start accepting inbound connections as configured (call from network strand).
    void start(result_handler handler) noexcept override;

protected:
    /// The channel is inbound (pend the nonce).
    bool inbound() const noexcept override;

    /// Notify subscribers on channel start.
    bool notify() const noexcept override;

    /// Overridden to change version protocol (base calls from channel strand).
    void attach_handshake(const channel::ptr& channel,
        result_handler handler) const noexcept override;

    /// Overridden to change channel protocols (base calls from channel strand).
    void attach_protocols(const channel::ptr& channel) const noexcept override;

    /// Start accepting based on configuration (call from network strand).
    virtual void start_accept(const code& ec, acceptor::ptr acceptor) noexcept;

private:
    void handle_started(const code& ec, result_handler handler) noexcept;
    void handle_accept(const code& ec, channel::ptr channel,
        acceptor::ptr acceptor) noexcept;

    void handle_channel_start(const code& ec, channel::ptr channel) noexcept;
    void handle_channel_stop(const code& ec, channel::ptr channel) noexcept;

    // This is thread safe.
    const size_t connection_limit_;
};

} // namespace network
} // namespace libbitcoin

#endif
