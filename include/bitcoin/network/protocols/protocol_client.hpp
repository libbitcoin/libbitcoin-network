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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_CLIENT_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_CLIENT_HPP

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

class BCT_API protocol_client
  : public protocol, protected tracker<protocol_client>
{
public:
    typedef std::shared_ptr<protocol_client> ptr;

    /// Construct an instance.
    protocol_client(const session::ptr& session,
        const channel::ptr& channel) NOEXCEPT;

    /// Start protocol (strand required).
    void start() NOEXCEPT override;

protected:
    DECLARE_SEND();
    DECLARE_SUBSCRIBE_CHANNEL();

    virtual void handle_receive_request(const code& ec,
        const asio::http_request& request) NOEXCEPT;

private:
    void handle_unalive_request(const code& ec, const code& reason) NOEXCEPT;

    // This is mostly thread safe, and used in a thread safe manner.
    // pause/resume/paused/attach not invoked, setters limited to handshake.
    const channel_client::ptr channel_;

    // This is thread safe.
    const session_client::ptr session_;
};

} // namespace network
} // namespace libbitcoin

#endif
