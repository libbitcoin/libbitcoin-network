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
#include <bitcoin/network/protocols/protocol_client_http.hpp>

#include <memory>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/settings.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/http/messages.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_client_http

using namespace http;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// [field] returns "" if not found but .at(field) throws.

protocol_client_http::protocol_client_http(const session::ptr& session,
    const channel::ptr& channel, const settings::http_server& options) NOEXCEPT
  : protocol_client(session, channel),
    channel_(std::dynamic_pointer_cast<channel_client>(channel)),
    session_(std::dynamic_pointer_cast<session_client>(session)),
    origins_(channel->settings().admin.origin_names()),
    hosts_(options.host_names()),
    server_(options.server),
    port_(options.secure ? default_tls : default_http),
    tracker<protocol_client_http>(session->log)
{
}

// Start.
// ----------------------------------------------------------------------------

void protocol_client_http::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (started())
        return;

    SUBSCRIBE_CHANNEL(method::get,     handle_receive_get,     _1, _2);
    SUBSCRIBE_CHANNEL(method::head,    handle_receive_head,    _1, _2);
    SUBSCRIBE_CHANNEL(method::post,    handle_receive_post,    _1, _2);
    SUBSCRIBE_CHANNEL(method::put,     handle_receive_put,     _1, _2);
    SUBSCRIBE_CHANNEL(method::delete_, handle_receive_delete,  _1, _2);
    SUBSCRIBE_CHANNEL(method::trace,   handle_receive_trace,   _1, _2);
    SUBSCRIBE_CHANNEL(method::options, handle_receive_options, _1, _2);
    SUBSCRIBE_CHANNEL(method::connect, handle_receive_connect, _1, _2);
    SUBSCRIBE_CHANNEL(method::unknown, handle_receive_unknown, _1, _2);
    protocol::start();
}

// Handle disallowed-by-default methods (override to implement).
// ----------------------------------------------------------------------------

void protocol_client_http::handle_receive_get(const code& ec,
    const method::get& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

void protocol_client_http::handle_receive_post(const code& ec,
    const method::post& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

void protocol_client_http::handle_receive_put(const code& ec,
    const method::put& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

void protocol_client_http::handle_receive_head(const code& ec,
    const method::head& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

void protocol_client_http::handle_receive_delete(const code& ec,
    const method::delete_& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

void protocol_client_http::handle_receive_trace(const code& ec,
    const method::trace& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

void protocol_client_http::handle_receive_options(const code& ec,
    const method::options& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

void protocol_client_http::handle_receive_connect(const code& ec,
    const method::connect& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

void protocol_client_http::handle_receive_unknown(const code& ec,
    const method::unknown& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

// Senders.
// ----------------------------------------------------------------------------

// Closes channel.
void protocol_client_http::send_method_not_allowed(
    const string_request& request, const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    if (stopped(ec))
        return;

    std::string details{ "method=" };
    details += request.method_string();
    const auto code = status::method_not_allowed;
    const auto mime = to_mime_type(request[field::accept]);
    string_response response{ status::bad_request, request.version() };
    add_common_headers(response, request, true);
    response.body() = format_status(code, response.reason(), mime, details);
    response.prepare_payload();
    SEND(std::move(response), handle_complete, _1, error::method_not_allowed);
}

void protocol_client_http::send_not_found(
    const string_request& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    std::string details{ "path:" };
    details += request.target();
    const auto code = status::not_found;
    const auto mime = to_mime_type(request[field::accept]);
    string_response response{ code, request.version() };
    add_common_headers(response, request);
    response.body() = format_status(code, response.reason(), mime, details);
    response.prepare_payload();
    SEND(std::move(response), handle_complete, _1, error::success);
}

// Closes channel.
void protocol_client_http::send_forbidden(
    const string_request& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    std::string details{ "origin:" };
    details += request[field::origin];
    const auto code = status::forbidden;
    const auto mime = to_mime_type(request[field::accept]);
    string_response response{ code, request.version() };
    add_common_headers(response, request, true);
    response.body() = format_status(code, response.reason(), mime, details);
    response.prepare_payload();
    SEND(std::move(response), handle_complete, _1, error::forbidden);
}

// Closes channel.
void protocol_client_http::send_bad_host(
    const string_request& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    std::string details{ "host=" };
    details += request[field::host];
    const auto code = status::bad_request;
    const auto mime = to_mime_type(request[field::accept]);
    string_response response{ status::bad_request, request.version() };
    add_common_headers(response, request, true);
    response.body() = format_status(code, response.reason(), mime, details);
    response.prepare_payload();
    SEND(std::move(response), handle_complete, _1, error::bad_request);
}

// Closes channel.
void protocol_client_http::send_bad_target(
    const string_request& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    std::string details{ "target=" };
    details += request.target();
    const auto code = status::bad_request;
    const auto mime = to_mime_type(request[field::accept]);
    string_response response{ status::bad_request, request.version() };
    add_common_headers(response, request, true);
    response.body() = format_status(code, response.reason(), mime, details);
    response.prepare_payload();
    SEND(std::move(response), handle_complete, _1, error::bad_request);
}

// Handle sends.
// ----------------------------------------------------------------------------

void protocol_client_http::handle_complete(const code& ec,
    const code& reason) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped(ec))
        return;

    if (reason)
    {
        stop(reason);
        return;
    }

    // Continue half duplex.
    channel_->read_request();
}

// Utilities.
// ----------------------------------------------------------------------------

bool protocol_client_http::is_allowed_origin(const std::string& origin,
    size_t version) const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Allow same-origin and no-origin requests.
    // Origin header field is not available until http 1.1.
    if (origin.empty() || version < version_1_1)
        return true;

    return origins_.empty() ||
        system::contains(origins_, config::to_normal_host(origin, port_));
}

bool protocol_client_http::is_allowed_host(const std::string& host,
    size_t version) const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Disallow unspecified host.
    // Host header field is mandatory at http 1.1.
    if (host.empty() && version >= version_1_1)
        return false;

    return hosts_.empty() ||
        system::contains(hosts_, config::to_normal_host(host, port_));
}

// TODO: pass and set response mime_type.
void protocol_client_http::add_common_headers(fields& fields,
    const string_request& request, bool closing) const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // date (current)
    // ------------------------------------------------------------------------
    fields.set(field::date, format_http_time(zulu_time()));

    // server (configured)
    // ------------------------------------------------------------------------
    if (!server_.empty())
        fields.set(field::server, server_);

    // origin (allow)
    // ------------------------------------------------------------------------
    if (to_bool(request.count(field::origin)))
        fields.set(field::access_control_allow_origin, request[field::origin]);

    // connection (close or keep-alive)
    // ------------------------------------------------------------------------
    // http 1.1 assumes keep-alive if not specified, http 1.0 does not.
    // Beast parser defaults keep-alive to true in http 1.1 requests.

    if (closing || !request.keep_alive())
    {
        fields.set(field::connection, "close");
        return;
    }

    if (request.version() < version_1_1)
        fields.set(field::connection, "keep-alive");

    // keep_alive (configured timeout)
    // ------------------------------------------------------------------------
    // The keep_alive.timeout field is encoded as seconds.

    // Remaining is zero if inactivity timer is expired (or not configured).
    if (const auto secs = remaining())
        fields.set(field::keep_alive, "timeout=" + system::serialize(secs));
}

// Status message generation.
// ----------------------------------------------------------------------------
// status.reason text is only available on non-polymorphic response types, so
// so it's dereferenced before calling and passed along with status enum value.

std::string protocol_client_http::format_status(const http::status /*status*/,
    const std::string& reason, const http::mime_type& /*type*/,
    const std::string& details) const NOEXCEPT
{
    // TODO: format proper status response bodies for status and mime type.
    return reason + (details.empty() ? "" : " [" + details + "]");
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
