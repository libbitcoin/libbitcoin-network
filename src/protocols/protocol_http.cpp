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
#include <bitcoin/network/protocols/protocol_http.hpp>

#include <memory>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/settings.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/http/http.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_http
    
using namespace http;
using namespace system;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

// [field] returns "" if not found but .at(field) throws.

protocol_http::protocol_http(const session::ptr& session,
    const channel::ptr& channel, const options_t& options) NOEXCEPT
  : protocol(session, channel),
    channel_(std::dynamic_pointer_cast<channel_http>(channel)),
    session_(std::dynamic_pointer_cast<session_server>(session)),
    default_port_(options.secure ? default_tls : default_http),
    options_(options)
{
}

// Start.
// ----------------------------------------------------------------------------

void protocol_http::start() NOEXCEPT
{
    BC_ASSERT(stranded());

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

void protocol_http::handle_receive_get(const code& ec,
    const method::get::cptr& get) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (stopped(ec)) return;
    send_method_not_allowed(*get);
}

void protocol_http::handle_receive_post(const code& ec,
    const method::post::cptr& post) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (stopped(ec)) return;
    send_method_not_allowed(*post);
}

void protocol_http::handle_receive_put(const code& ec,
    const method::put::cptr& put) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (stopped(ec)) return;
    send_method_not_allowed(*put);
}

void protocol_http::handle_receive_head(const code& ec,
    const method::head::cptr& head) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (stopped(ec)) return;
    send_method_not_allowed(*head);
}

void protocol_http::handle_receive_delete(const code& ec,
    const method::delete_::cptr& delete_) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (stopped(ec)) return;
    send_method_not_allowed(*delete_);
}

void protocol_http::handle_receive_trace(const code& ec,
    const method::trace::cptr& trace) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (stopped(ec)) return;
    send_method_not_allowed(*trace);
}

void protocol_http::handle_receive_options(const code& ec,
    const method::options::cptr& options) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (stopped(ec)) return;
    send_method_not_allowed(*options);
}

void protocol_http::handle_receive_connect(const code& ec,
    const method::connect::cptr& connect) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (stopped(ec)) return;
    send_method_not_allowed(*connect);
}

void protocol_http::handle_receive_unknown(const code& ec,
    const method::unknown::cptr& unknown) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (stopped(ec)) return;
    send_method_not_allowed(*unknown);
}

// Senders.
// ----------------------------------------------------------------------------

// Closes channel.
void protocol_http::send_internal_server_error(const code& reason,
    const request& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    LOGA("Internal server error ["
        << (request.target().empty() ? "default" : request.target()) << "] "
        << reason.message());

    std::string details{ "error=" };
    details += reason.message();
    const auto code = status::internal_server_error;
    const auto media = to_media_type(request[field::accept]);
    response out{ code, request.version() };
    add_common_headers(out, request, true);
    out.body() = string_status(code, out.reason(), media, details);
    out.prepare_payload();
    SEND(std::move(out), handle_complete, _1, error::internal_server_error);
}

void protocol_http::send_bad_target(const code& reason,
    const request& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    std::string details{ "target=" };
    details += request.target();
    if (reason) details += "\nreason=" + reason.message();
    const auto code = status::bad_request;
    const auto media = to_media_type(request[field::accept]);
    response out{ code, request.version() };
    add_common_headers(out, request);
    out.body() = string_status(code, out.reason(), media, details);
    out.prepare_payload();
    SEND(std::move(out), handle_complete, _1, error::success);
}

void protocol_http::send_not_found(const request& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    std::string details{ "path:" };
    details += request.target();
    const auto code = status::not_found;
    const auto media = to_media_type(request[field::accept]);
    response out{ code, request.version() };
    add_common_headers(out, request);
    out.body() = string_status(code, out.reason(), media, details);
    out.prepare_payload();
    SEND(std::move(out), handle_complete, _1, error::success);
}

void protocol_http::send_not_acceptable(const request& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    const auto code = status::not_acceptable;
    const auto media = to_media_type(request[field::accept]);
    response out{ code, request.version() };
    add_common_headers(out, request);
    out.body() = string_status(code, out.reason(), media);
    out.prepare_payload();
    SEND(std::move(out), handle_complete, _1, error::success);
}

// Closes channel.
void protocol_http::send_not_implemented(const request& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    const auto code = status::not_implemented;
    const auto media = to_media_type(request[field::accept]);
    response out{ code, request.version() };
    add_common_headers(out, request, true);
    out.body() = string_status(code, out.reason(), media);
    out.prepare_payload();
    SEND(std::move(out), handle_complete, _1, error::not_implemented);
}

// Closes channel.
void protocol_http::send_forbidden(const request& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    std::string details{ "origin:" };
    details += request[field::origin];
    const auto code = status::forbidden;
    const auto media = to_media_type(request[field::accept]);
    response out{ code, request.version() };
    add_common_headers(out, request, true);
    out.body() = string_status(code, out.reason(), media, details);
    out.prepare_payload();
    SEND(std::move(out), handle_complete, _1, error::forbidden);
}

// Closes channel.
void protocol_http::send_bad_host(const request& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    std::string details{ "host=" };
    details += request[field::host];
    const auto code = status::bad_request;
    const auto media = to_media_type(request[field::accept]);
    response out{ code, request.version() };
    add_common_headers(out, request, true);
    out.body() = string_status(code, out.reason(), media, details);
    out.prepare_payload();
    SEND(std::move(out), handle_complete, _1, error::bad_request);
}

// Closes channel.
void protocol_http::send_method_not_allowed(const request& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    std::string details{ "method=" };
    details += request.method_string();
    const auto code = status::method_not_allowed;
    const auto media = to_media_type(request[field::accept]);
    response out{ code, request.version() };
    add_common_headers(out, request, true);
    out.body() = string_status(code, out.reason(), media, details);
    out.prepare_payload();
    SEND(std::move(out), handle_complete, _1, error::method_not_allowed);
}

// Sets access control headers.
void protocol_http::send_ok(const request& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    const auto code = status::ok;
    const auto media = to_media_type(request[field::accept]);
    response out{ code, request.version() };
    add_common_headers(out, request);
    add_access_control_headers(out, request);
    out.body() = string_status(code, out.reason(), media);
    out.prepare_payload();
    SEND(std::move(out), handle_complete, _1, error::success);
}

// Handle sends.
// ----------------------------------------------------------------------------

void protocol_http::handle_complete(const code& ec,
    const code& reason) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped(ec))
        return;

    if (reason)
    {
        stop(reason);
        return;
    }

    // Continue read loop.
    channel_->receive();
}

// Utilities.
// ----------------------------------------------------------------------------

bool protocol_http::is_allowed_host(const fields& fields,
    size_t version) const NOEXCEPT
{
    // Disallow unspecified host.
    // Host header field is mandatory at http 1.1.
    const auto host = fields[field::host];
    if (host.empty() && version >= version_1_1)
        return false;

    return options_.hosts.empty() || contains(options_.hosts,
        config::to_normal_host(host, default_port()));
}

bool protocol_http::is_allowed_origin(const fields& fields,
    size_t version) const NOEXCEPT
{
    // Allow same-origin and no-origin requests.
    // Origin header field is not available until http 1.1.
    const auto origin = fields[field::origin];
    if (origin.empty() || version < version_1_1)
        return true;

    // Opaque origin is sent as "null", often representing a file: URL.
    if (origin == "null")
        return options_.allow_opaque_origin;

    return options_.origins.empty() || contains(options_.origins,
        config::to_normal_host(origin, default_port()));
}

void protocol_http::add_common_headers(fields& fields,
    const request& request, bool closing) const NOEXCEPT
{
    BC_ASSERT(stranded());

    fields.set(field::date, format_http_time(zulu_time()));
    if (!options_.server.empty())
        fields.set(field::server, options_.server);

    // http 1.1 assumes keep-alive if not specified, http 1.0 does not.
    // Beast parser defaults keep-alive to true in http 1.1 requests.
    if (closing || !request.keep_alive())
    {
        fields.set(field::connection, "close");
    }
    else
    {
        if (request.version() < version_1_1)
            fields.set(field::connection, "keep-alive");

        // Remaining zero if inactivity timer has expired (or not configured).
        if (const auto secs = remaining())
            fields.set(field::keep_alive, "timeout=" + serialize(secs));
    }
}

void protocol_http::add_access_control_headers(fields& fields,
    const request& request) const NOEXCEPT
{
    const auto origin = to_bool(request.count(field::origin));
    const auto headers = to_bool(request.count(
        field::access_control_request_headers));
    const auto method = to_bool(request.count(
        field::access_control_request_method));

    if (origin)
        fields.set(field::access_control_allow_origin, request[field::origin]);
    if (headers)
        fields.set(field::access_control_allow_headers, "*");
    if (method)
        fields.set(field::access_control_allow_methods, "*");

    if (origin || headers || method)
    {
        fields.set(field::vary, "origin");
        fields.set(field::access_control_max_age, "86400");
    }
}

// Status message generation.
// ----------------------------------------------------------------------------
// status.reason text is only available on non-polymorphic response types, so
// so it's dereferenced before calling and passed along with status enum value.

std::string protocol_http::string_status(const http::status /*status*/,
    const std::string& reason, const http::media_type& /*type*/,
    const std::string& details) const NOEXCEPT
{
    // TODO: format proper status response bodies for status and media type.
    return reason + (details.empty() ? "" : " [" + details + "]");
}

// Properties.
// ----------------------------------------------------------------------------

uint16_t protocol_http::default_port() const NOEXCEPT
{
    return default_port_;
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
