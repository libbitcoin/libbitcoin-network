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
#ifndef LIBBITCOIN_NETWORK_SESSION_SEED_HPP
#define LIBBITCOIN_NETWORK_SESSION_SEED_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/session.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class p2p;

/// Seed connections session, thread safe.
class BCT_API session_seed
  : public session, protected tracker<session_seed>
{
public:
    typedef std::shared_ptr<session_seed> ptr;

    /// Construct an instance.
    session_seed(p2p& network, uint64_t identifier) NOEXCEPT;

    /// Perform seeding as configured (call from network strand).
    /// Seeding is complete invocation of the handler.
    void start(result_handler&& handler) NOEXCEPT override;

protected:
    /// Overridden to set service and version minimums upon session start.
    void attach_handshake(const channel::ptr& channel,
        result_handler&& handler) NOEXCEPT override;

    /// Overridden to attach only seeding protocols upon channel start.
    void attach_protocols(const channel::ptr& channel) NOEXCEPT override;

    /// Start a seed connection (called from start).
    virtual void start_seed(const code& ec, const config::endpoint& seed,
        const connector::ptr& connector, const socket_handler& handler) NOEXCEPT;

    /// All seed connections are stopped.
    virtual void stop_seed(const code& ec) NOEXCEPT;

private:
    typedef race_volume<error::success, error::seeding_unsuccessful> race;

    void handle_started(const code& ec,
        const result_handler& handler) NOEXCEPT;
    void handle_connect(const code& ec, const socket::ptr& socket,
        const config::endpoint& seed, const race::ptr& racer) NOEXCEPT;
    void handle_channel_start(const code& ec,
        const channel::ptr& channel) NOEXCEPT;
    void handle_channel_stop(const code& ec,
        const channel::ptr& channel, const race::ptr& racer) NOEXCEPT;
};

} // namespace network
} // namespace libbitcoin

#endif

