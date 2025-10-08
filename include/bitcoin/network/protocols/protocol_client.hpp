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
#include <bitcoin/network/messages/rpc/messages.hpp>
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

    /// Message handlers by http method.
    virtual void handle_receive_get(const code& ec,
        const messages::rpc::method::get& request) NOEXCEPT;
    virtual void handle_receive_head(const code& ec,
        const messages::rpc::method::head& request) NOEXCEPT;
    virtual void handle_receive_post(const code& ec,
        const messages::rpc::method::post& request) NOEXCEPT;
    virtual void handle_receive_put(const code& ec,
        const messages::rpc::method::put& request) NOEXCEPT;
    virtual void handle_receive_delete(const code& ec,
        const messages::rpc::method::delete_& request) NOEXCEPT;
    virtual void handle_receive_trace(const code& ec,
        const messages::rpc::method::trace& request) NOEXCEPT;
    virtual void handle_receive_options(const code& ec,
        const messages::rpc::method::options& request) NOEXCEPT;
    virtual void handle_receive_connect(const code& ec,
        const messages::rpc::method::connect& request) NOEXCEPT;
    virtual void handle_receive_unknown(const code& ec,
        const messages::rpc::method::unknown& request) NOEXCEPT;

    /// Send a common not_allowed response.
    virtual void send_not_allowed(const code& ec,
        const http_string_request_cptr& request) NOEXCEPT;

    /// Request handler must invoke one of these.
    virtual void handle_complete(const code& ec,
        const code& reason) NOEXCEPT;

private:
    // This is mostly thread safe, and used in a thread safe manner.
    // pause/resume/paused/attach not invoked, setters limited to handshake.
    const channel_client::ptr channel_;

    // These are thread safe.
    const session_client::ptr session_;
    const std::filesystem::path& root_;
    const std::string& default_;
};

} // namespace network
} // namespace libbitcoin

#endif
