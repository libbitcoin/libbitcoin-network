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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_PEER_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_PEER_HPP

#include <memory>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/p2p/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class BCT_API protocol_peer
  : public protocol
{
protected:
    typedef std::shared_ptr<protocol_peer> ptr;

    DECLARE_SEND();
    DECLARE_SUBSCRIBE_CHANNEL();

    /// Construct an instance.
    /// -----------------------------------------------------------------------
    protocol_peer(const session::ptr& session,
        const channel::ptr& channel) NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// The start height (for version message).
    virtual size_t start_height() const NOEXCEPT;

    /// The protocol version of the peer.
    virtual messages::p2p::version::cptr peer_version() const NOEXCEPT;

    /// Set protocol version of the peer (set only during handshake).
    virtual void set_peer_version(const messages::p2p::version::cptr& value) NOEXCEPT;

    /// The negotiated protocol version.
    virtual uint32_t negotiated_version() const NOEXCEPT;

    /// Set negotiated protocol version (set only during handshake).
    virtual void set_negotiated_version(uint32_t value) NOEXCEPT;

    /// Advertised addresses with own services and current timestamp.
    virtual messages::p2p::address selfs() const NOEXCEPT;

    /// Addresses.
    /// -----------------------------------------------------------------------

    /// Number of entries in the address pool.
    virtual size_t address_count() const NOEXCEPT;

    /// Fetch a set of peer addresses from the address pool.
    virtual void fetch(address_handler&& handler) NOEXCEPT;

    /// Save a set of peer addresses to the address pool.
    virtual void save(const address_cptr& message,
        count_handler&& handler) NOEXCEPT;

private:
    void handle_fetch(const code& ec, const address_cptr& message,
        const address_handler& handler) NOEXCEPT;
    void handle_save(const code& ec, size_t accepted,
        const count_handler& handler) NOEXCEPT;

    // This is mostly thread safe, and used in a thread safe manner.
    // pause/resume/paused/attach not invoked, setters limited to handshake.
    const channel_peer::ptr channel_;

    // This is thread safe.
    const session_peer::ptr session_;
};

} // namespace network
} // namespace libbitcoin

#endif
