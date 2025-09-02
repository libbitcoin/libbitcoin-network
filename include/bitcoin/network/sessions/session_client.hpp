/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_SESSION_CLIENT_HPP
#define LIBBITCOIN_NETWORK_SESSION_CLIENT_HPP

#include <memory>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/sessions/session.hpp>

namespace libbitcoin {
namespace network {

class p2p;

class BCT_API session_client
  : public session
{
public:
    typedef std::shared_ptr<session_client> ptr;

protected:
    /// Construct an instance (network should be started).
    session_client(p2p& network, uint64_t identifier) NOEXCEPT;

    /// Channel sequence.
    /// -----------------------------------------------------------------------

    /// Override to change version protocol (base calls from channel strand).
    void attach_handshake(const channel::ptr& channel,
        result_handler&& handler) NOEXCEPT override;

    /// Override to change channel protocols (base calls from channel strand).
    void attach_protocols(const channel::ptr& channel) NOEXCEPT override;

    /// Factories.
    /// -----------------------------------------------------------------------

    /// Call to create channel acceptor, owned by caller.
    acceptor::ptr create_acceptor() NOEXCEPT override;

    /// Call to create channel connector, owned by caller.
    connector::ptr create_connector() NOEXCEPT override;

    /// Call to create a set of channel connectors, owned by caller.
    connectors_ptr create_connectors(size_t count) NOEXCEPT override;

    /// Create a channel from the started socket.
    channel::ptr create_channel(const socket::ptr& socket) NOEXCEPT override;

private:
    // This is thread safe (mostly).
    p2p& network_;
};

} // namespace network
} // namespace libbitcoin

#endif
