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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_HTTP_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_HTTP_HPP

#include <memory>
#include <bitcoin/network/channels/channels.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// This abstract base protocol subscribes to http request messages (by verb)
/// and dispatches the message obect to virtual methods by verb. Standard
/// responses are sent for disalloved verbs and other invalidated requests.
/// Utilities are provided to simplify common header response validation and
/// behavior based configured options. Derive from this class to implement an
/// http service that does NOT process a document directory (see
/// node::protocol_html for that).
class BCT_API protocol_http
  : public protocol
{
public:
    typedef std::shared_ptr<protocol_http> ptr;
    using channel_t = channel_http;
    using options_t = channel_t::options_t;

    void start() NOEXCEPT override;

protected:
    protocol_http(const session::ptr& session, const channel::ptr& channel,
        const options_t& options) NOEXCEPT;

    DECLARE_SEND()
    DECLARE_SUBSCRIBE_CHANNEL()

    /// Message handlers by http method.
    virtual void handle_receive_get(const code& ec,
        const http::method::get::cptr& get) NOEXCEPT;
    virtual void handle_receive_head(const code& ec,
        const http::method::head::cptr& head) NOEXCEPT;
    virtual void handle_receive_post(const code& ec,
        const http::method::post::cptr& post) NOEXCEPT;
    virtual void handle_receive_put(const code& ec,
        const http::method::put::cptr& put) NOEXCEPT;
    virtual void handle_receive_delete(const code& ec,
        const http::method::delete_::cptr& delete_) NOEXCEPT;
    virtual void handle_receive_trace(const code& ec,
        const http::method::trace::cptr& trace) NOEXCEPT;
    virtual void handle_receive_options(const code& ec,
        const http::method::options::cptr& options) NOEXCEPT;
    virtual void handle_receive_connect(const code& ec,
        const http::method::connect::cptr& connect) NOEXCEPT;
    virtual void handle_receive_unknown(const code& ec,
        const http::method::unknown::cptr& unknown) NOEXCEPT;

    /// Senders.
    virtual void send_ok(const http::request& request={}) NOEXCEPT;
    virtual void send_bad_host(const http::request& request={}) NOEXCEPT;
    virtual void send_bad_request(const http::request& request={}) NOEXCEPT;
    virtual void send_not_found(const http::request& request={}) NOEXCEPT;
    virtual void send_not_acceptable(const http::request& request={}) NOEXCEPT;
    virtual void send_forbidden(const http::request& request={}) NOEXCEPT;
    virtual void send_not_implemented(const http::request& request={}) NOEXCEPT;
    virtual void send_method_not_allowed(const http::request& request={}) NOEXCEPT;
    virtual void send_internal_server_error(const code& reason,
        const http::request& request={}) NOEXCEPT;
    virtual void send_bad_target(const code& reason={},
        const http::request& request={}) NOEXCEPT;

    /// Request handler MUST invoke this once unless stopped.
    virtual void handle_complete(const code& ec,
        const code& reason) NOEXCEPT;

    /// Sets date, server, keep-alive - does NOT set access control.  
    virtual void add_common_headers(http::fields& fields,
        const http::request& request, bool closing=false) const NOEXCEPT;

    /// Set only on success (200/204), assumes origin has been verified.
    virtual void add_access_control_headers(http::fields& fields,
        const http::request& request) const NOEXCEPT;

    /// Override to replace string status response message.
    virtual std::string string_status(const http::status status,
        const std::string& reason, const http::media_type& type,
        const std::string& details={}) const NOEXCEPT;

    /// Utilities.
    virtual bool is_allowed_host(const http::fields& fields,
        size_t version) const NOEXCEPT;
    virtual bool is_allowed_origin(const http::fields& fields,
        size_t version) const NOEXCEPT;

    /// Properties.
    uint16_t default_port() const NOEXCEPT;

private:
    // This is mostly thread safe, and used in a thread safe manner.
    // pause/resume/paused/attach not invoked, setters limited to handshake.
    const channel_http::ptr channel_;

    // These are thread safe.
    const session_server::ptr session_;
    const uint16_t default_port_;
    const options_t& options_;
};

} // namespace network
} // namespace libbitcoin

#endif
