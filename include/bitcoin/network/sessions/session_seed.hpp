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
#ifndef LIBBITCOIN_NETWORK_SESSION_SEED_HPP
#define LIBBITCOIN_NETWORK_SESSION_SEED_HPP

#include <cstddef>
#include <memory>
#include <unordered_set>
#include <bitcoin/system.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
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
    session_seed(p2p& network) NOEXCEPT;

    /// Perform seeding as configured (call from network strand).
    /// Seeding is complete invocation of the handler.
    void start(result_handler&& handler) NOEXCEPT override;

    /// Stop the session timer and subscriber (call from network strand).
    void stop() NOEXCEPT override;

protected:
    typedef std::shared_ptr<size_t> count_ptr;

    /// The channel is outbound (do not pend the nonce).
    bool inbound() const NOEXCEPT override;

    /// Do not notify subscribers on channel start.
    bool notify() const NOEXCEPT override;

    /// Overridden to set service and version minimums upon session start.
    void attach_handshake(const channel::ptr& channel,
        result_handler&& handler) const NOEXCEPT override;

    /// Overridden to attach only seeding protocols upon channel start.
    void attach_protocols(const channel::ptr& channel) const NOEXCEPT override;

    /// Start a seed connection (called from start).
    virtual void start_seed(const config::endpoint& seed,
        const connector::ptr& connector,
        const channel_handler& handler) NOEXCEPT;

    /// Accumulate the result of the seed connection.
    virtual void stop_seed(const count_ptr& counter,
        const result_handler& handler) NOEXCEPT;

private:
    void handle_started(const code& ec, const result_handler& handler) NOEXCEPT;
    void handle_connect(const code& ec, const channel::ptr& channel,
        const config::endpoint& seed, const count_ptr& counter,
        const result_handler& handler) NOEXCEPT;

    void handle_channel_start(const code& ec, const channel::ptr& channel) NOEXCEPT;
    void handle_channel_stop(const code& ec, const count_ptr& counter,
        const channel::ptr& channel, const result_handler& handler) NOEXCEPT;

    std::unordered_set<channel::ptr> seeding_{};
};

} // namespace network
} // namespace libbitcoin

#endif

