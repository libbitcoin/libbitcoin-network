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
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/session.hpp>

namespace libbitcoin {
namespace network {

class net;

class BCT_API session_client
  : public session
{
public:
    typedef std::shared_ptr<session_client> ptr;

    /// Start accepting connections as configured (call from network strand).
    void start(result_handler&& handler) NOEXCEPT override;

protected:
    /// Construct an instance (network should be started).
    session_client(net& network, uint64_t identifier,
        const config::endpoints& bindings, size_t connections,
        const std::string& name) NOEXCEPT;

    /// Accept cycle.
    /// -----------------------------------------------------------------------

    /// Start accepting based on constructed configuration (called from start).
    virtual void start_accept(const code& ec,
        const acceptor::ptr& acceptor) NOEXCEPT;

    /// Channel sequence.
    /// -----------------------------------------------------------------------

    /// Default no-op implementation of client-server handshake protocol.
    void attach_handshake(const channel::ptr& channel,
        result_handler&& handler) NOEXCEPT override;

    /// Default no-op implementation of client-server handshake protocol.
    void do_attach_handshake(const channel::ptr& channel,
        const result_handler& handshake) NOEXCEPT override;

private:
    void handle_started(const code& ec,
        const result_handler& handler) NOEXCEPT;
    void handle_accepted(const code& ec, const socket::ptr& socket,
        const acceptor::ptr& acceptor) NOEXCEPT;

    // Completion sequence.
    void handle_channel_start(const code& ec,
        const channel::ptr& channel) NOEXCEPT;
    void handle_channel_stop(const code& ec,
        const channel::ptr& channel) NOEXCEPT;

    // This is thread safe (mostly).
    ////net& network_;

    // These are thread safe.
    const config::endpoints& bindings_;
    const size_t connections_;
    const std::string name_;

    // This is protected by strand.
    size_t channel_count_{};
};

} // namespace network
} // namespace libbitcoin

#endif
