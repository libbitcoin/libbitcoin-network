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
#ifndef LIBBITCOIN_NETWORK_SESSION_PEER_HPP
#define LIBBITCOIN_NETWORK_SESSION_PEER_HPP

#include <memory>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/channels/channels.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/session.hpp>

namespace libbitcoin {
namespace network {

class net;

/// Abstract base class for maintaining a channel set, thread safe.
class BCT_API session_peer
  : public session
{
public:
    typedef std::shared_ptr<session_peer> ptr;
    using options_t = network::settings::tcp_server;

    /// Utilities.
    /// -----------------------------------------------------------------------

    /// Take an entry from address pool.
    virtual void take(address_item_handler&& handler) const NOEXCEPT;

    /// Fetch a subset of entries (count based on config) from address pool.
    virtual void fetch(address_handler&& handler) const NOEXCEPT;

    /// Restore an address to the address pool.
    virtual void restore(const address_item_cptr& address,
        result_handler&& handler) const NOEXCEPT;

    /// Save a subset of entries (count based on config) from address pool.
    virtual void save(const address_cptr& message,
        count_handler&& handler) const NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// Number of entries in the address pool.
    virtual size_t address_count() const NOEXCEPT;

protected:
    /// Constructors.
    /// -----------------------------------------------------------------------

    /// Construct an instance (network should be started).
    session_peer(net& network, uint64_t identifier,
        const options_t& options) NOEXCEPT;

    /// Channel sequence.
    /// -----------------------------------------------------------------------

    /// Perform handshake and attach protocols (call from network strand).
    void start_channel(const channel::ptr& channel, result_handler&& starter,
        result_handler&& stopper) NOEXCEPT override;

    /// Override to change version protocol (base calls from channel strand).
    void attach_handshake(const channel::ptr& channel,
        result_handler&& handler) NOEXCEPT override;

    /// Override to change channel protocols (base calls from channel strand).
    void attach_protocols(const channel::ptr& channel) NOEXCEPT override;

    /// Factories.
    /// -----------------------------------------------------------------------

    /// Create a channel from the started socket.
    channel::ptr create_channel(const socket::ptr& socket) NOEXCEPT override;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// Number of all connected channels.
    virtual size_t channel_count() const NOEXCEPT;

    /// Number of inbound connected channels.
    virtual size_t inbound_channel_count() const NOEXCEPT;

    /// Number of outbound connected channels (including manual).
    virtual size_t outbound_channel_count() const NOEXCEPT;

    /// Message level is supported by confired protocol level.
    virtual bool is_configured(messages::peer::level level) const NOEXCEPT;

    /// The configured options for this peer session.
    virtual const options_t& options() const NOEXCEPT;

private:
    void do_handle_handshake(const code& ec, const channel::ptr& channel,
        const result_handler& start) NOEXCEPT override;
    void do_attach_protocols(const channel::ptr& channel,
        const result_handler& started) NOEXCEPT override;
    void do_handle_channel_stopped(const code& ec, const channel::ptr& channel,
        const result_handler& stopped) NOEXCEPT override;

    // This is thread safe (mostly).
    net& network_;

    // This is thread safe.
    const options_t& options_;
};

} // namespace network
} // namespace libbitcoin

#endif
