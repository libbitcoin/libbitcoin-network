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
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/channels/channels.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/messages.hpp>
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

protected:
    protocol_http(const session::ptr& session, const channel::ptr& channel,
        const options_t& options) NOEXCEPT;

    void start() NOEXCEPT override;

    DECLARE_SEND();
    DECLARE_SUBSCRIBE_CHANNEL();

    /// Message handlers by http method.
    virtual void handle_receive_get(const code& ec,
        const http::method::get& request) NOEXCEPT;
    virtual void handle_receive_head(const code& ec,
        const http::method::head& request) NOEXCEPT;
    virtual void handle_receive_post(const code& ec,
        const http::method::post& request) NOEXCEPT;
    virtual void handle_receive_put(const code& ec,
        const http::method::put& request) NOEXCEPT;
    virtual void handle_receive_delete(const code& ec,
        const http::method::delete_& request) NOEXCEPT;
    virtual void handle_receive_trace(const code& ec,
        const http::method::trace& request) NOEXCEPT;
    virtual void handle_receive_options(const code& ec,
        const http::method::options& request) NOEXCEPT;
    virtual void handle_receive_connect(const code& ec,
        const http::method::connect& request) NOEXCEPT;
    virtual void handle_receive_unknown(const code& ec,
        const http::method::unknown& request) NOEXCEPT;

    /// Senders.
    virtual void send_bad_host(const http::string_request& request) NOEXCEPT;
    virtual void send_not_found(const http::string_request& request) NOEXCEPT;
    virtual void send_forbidden(const http::string_request& request) NOEXCEPT;
    virtual void send_bad_target(const http::string_request& request) NOEXCEPT;
    virtual void send_not_implemented(const http::string_request& request) NOEXCEPT;
    virtual void send_method_not_allowed(const http::string_request& request,
        const code& ec) NOEXCEPT;

    /// Request handler MUST invoke this once unless stopped.
    virtual void handle_complete(const code& ec,
        const code& reason) NOEXCEPT;

    /// Override to replace status response headers.
    virtual void add_common_headers(http::fields& fields,
        const http::string_request& request,
        bool closing = false) const NOEXCEPT;

    /// Override to replace status response message.
    virtual std::string format_status(const http::status status,
        const std::string& reason, const http::mime_type& type,
        const std::string& details = {}) const NOEXCEPT;

    /// Utilities.
    bool is_allowed_host(const std::string& host,
        size_t version) const NOEXCEPT;

    /// Properties.
    uint16_t default_port() const NOEXCEPT;

private:
    // This is mostly thread safe, and used in a thread safe manner.
    // pause/resume/paused/attach not invoked, setters limited to handshake.
    const channel_http::ptr channel_;

    // These are thread safe.
    const session_tcp::ptr session_;
    const uint16_t default_port_;
    const options_t& options_;
};

} // namespace network
} // namespace libbitcoin

#endif
