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

#include <filesystem>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
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
    protocol_client(const session::ptr& session, const channel::ptr& channel,
        const http_server& settings) NOEXCEPT;

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

    virtual void send_file(const http_string_request& request,
        http_file&& file, const std::string& mime_type) NOEXCEPT;
    virtual void send_bad_host(const http_string_request& request) NOEXCEPT;
    virtual void send_not_found(const http_string_request& request) NOEXCEPT;
    virtual void send_forbidden(const http_string_request& request) NOEXCEPT;
    virtual void send_bad_target(const http_string_request& request) NOEXCEPT;
    virtual void send_method_not_allowed(const http_string_request& request,
        const code& ec) NOEXCEPT;

    /// Request handler MUST invoke one of these unless stopped(ec).
    virtual void handle_complete(const code& ec,
        const code& reason) NOEXCEPT;

private:
    std::filesystem::path to_local_path(
        const std::string& target) const NOEXCEPT;
    void add_common_headers(http_fields& fields,
        const http_string_request& request,
        bool closing=false) const NOEXCEPT;
    bool is_allowed_origin(const std::string& origin,
        size_t version) const NOEXCEPT;
    bool is_allowed_host(const std::string& host,
        size_t version) const NOEXCEPT;

    // This is mostly thread safe, and used in a thread safe manner.
    // pause/resume/paused/attach not invoked, setters limited to handshake.
    const channel_client::ptr channel_;

    // These are thread safe.
    const session_client::ptr session_;
    const system::string_list origins_;
    const system::string_list hosts_;
    const std::filesystem::path& root_;
    const std::string& default_;
    const std::string& server_;
    const uint16_t port_;
};

} // namespace network
} // namespace libbitcoin

#endif
