/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/session.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class p2p;

/// Outbound connections session, thread safe.
class BCT_API session_outbound
  : public session, protected tracker<session_outbound>
{
public:
    typedef std::shared_ptr<session_outbound> ptr;

    /// Construct an instance (network should be started).
    session_outbound(p2p& network, uint64_t identifier) NOEXCEPT;

    /// Start configured number of connections (call from network strand).
    void start(result_handler&& handler) NOEXCEPT override;

protected:
    /// Overridden to change version protocol (base calls from channel strand).
    void attach_handshake(const channel::ptr& channel,
        result_handler&& handle_started) NOEXCEPT override;

    /// Overridden to change channel protocols (base calls from channel strand).
    void attach_protocols(const channel::ptr& channel) NOEXCEPT override;

    /// Start outbound connection loop.
    virtual void start_connect(const code& ec) NOEXCEPT;

private:
    typedef race_quality<const code&, const socket::ptr&> race;

    void handle_started(const code& ec,
        const result_handler& handler) NOEXCEPT;
    void do_one(const code& ec, const config::address& peer, object_key key,
        const race::ptr& racer, const connector::ptr& connector) NOEXCEPT;
    void handle_one(const code& ec, const socket::ptr& socket,
        object_key key, const config::address& peer,
        const race::ptr& racer) NOEXCEPT;
    void handle_connect(const code& ec, const socket::ptr& socket,
        object_key key) NOEXCEPT;

    void handle_channel_start(const code& ec,
        const channel::ptr& channel) NOEXCEPT;
    void handle_channel_stop(const code& ec,
        const channel::ptr& channel) NOEXCEPT;

    /// Restore an address to the address pool.
    inline bool maybe_reclaim(const code& ec) const NOEXCEPT;
    inline bool always_reclaim(const code& ec) const NOEXCEPT;
    void reclaim(const code& ec, const socket::ptr& socket) NOEXCEPT;
    void reclaim(const code& ec, const channel::ptr& channel) NOEXCEPT;
    void handle_reclaim(const code& ec) const NOEXCEPT;
};

} // namespace network
} // namespace libbitcoin

#endif
