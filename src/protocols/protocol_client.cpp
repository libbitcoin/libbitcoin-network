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
#include <bitcoin/network/protocols/protocol_client.hpp>

#include <algorithm>
#include <ranges>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/rpc/messages.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_client

using namespace asio;
using namespace http;
using namespace system;
using namespace messages::rpc;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

protocol_client::protocol_client(const session::ptr& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    channel_(std::dynamic_pointer_cast<channel_client>(channel)),
    session_(std::dynamic_pointer_cast<session_client>(session)),
    host_names_(channel->settings().admin.host_names()),
    root_(channel->settings().admin.path),
    default_(channel->settings().admin.default_),
    server_(channel->settings().admin.server),
    timeout_(channel->settings().admin.timeout_seconds),
    tracker<protocol_client>(session->log)
{
}

// Start.
// ----------------------------------------------------------------------------

void protocol_client::start() NOEXCEPT
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

// Do sends.
// ----------------------------------------------------------------------------

void protocol_client::send_file(const http_string_request& request,
    http_file&& file, const std::string& mime_type) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(file.is_open(), "sending closed file handle");

    http_file_response response{ status::ok, request.version() };
    add_common_headers(response.base(), request);
    response.set(field::content_type, mime_type);

    response.body() = std::move(file);
    response.prepare_payload();

    SEND(std::move(response), handle_complete, _1, error::success);
}

void protocol_client::send_not_found(
    const http_string_request& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    http_string_response response{ status::not_found, request.version() };
    add_common_headers(response.base(), request);

    const code reason{ error::not_found };
    response.body() += reason.message() + " : target : ";
    response.body() += request.target();
    response.prepare_payload();

    SEND(std::move(response), handle_complete, _1, error::success);
}

// Closes channel.
void protocol_client::send_bad_host(
    const http_string_request& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    http_string_response response{ status::bad_request, request.version() };
    add_common_headers(response.base(), request, true);

    const code reason{ error::bad_request };
    response.body() += reason.message() + " : host : ";
    response.body() += request.at(field::host);
    response.prepare_payload();

    SEND(std::move(response), handle_complete, _1, reason);
}

// Closes channel.
void protocol_client::send_bad_target(
    const http_string_request& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    http_string_response response{ status::bad_request, request.version() };
    add_common_headers(response.base(), request, true);

    const code reason{ error::bad_request };
    response.body() += reason.message() + " : target : ";
    response.body() += request.target();
    response.prepare_payload();

    SEND(std::move(response), handle_complete, _1, reason);
}

// Closes channel.
void protocol_client::send_method_not_allowed(
    const http_string_request& request, const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped(ec))
        return;

    const auto version = request.version();
    http_string_response response{ status::method_not_allowed, version };
    add_common_headers(response.base(), request, true);

    const code reason{ error::method_not_allowed };
    response.body() += reason.message() + " : ";
    response.body() += request.method_string();
    response.prepare_payload();

    SEND(std::move(response), handle_complete, _1, reason);
}

// Handle get method.
// ----------------------------------------------------------------------------

void protocol_client::handle_receive_get(const code& ec,
    const method::get& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped(ec))
        return;

    // Enforce http host header if any hosts are configured.
    if (!is_allowed_host(request->at(field::host)))
    {
        send_bad_host(*request);
        return;
    }

    // Empty path implies malformed target (terminal).
    const auto path = to_local_path(request->target());
    if (path.empty())
    {
        send_bad_target(*request);
        return;
    }

    // Not open implies file not found (non-terminal).
    auto file = get_file_body(path);
    if (!file.is_open())
    {
        send_not_found(*request);
        return;
    }

    send_file(*request, std::move(file), get_mime_type(path));
}

// Handle disallowed-by-default methods (override to implement).
// ----------------------------------------------------------------------------

void protocol_client::handle_receive_post(const code& ec,
    const method::post& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

void protocol_client::handle_receive_put(const code& ec,
    const method::put& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

void protocol_client::handle_receive_head(const code& ec,
    const method::head& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

void protocol_client::handle_receive_delete(const code& ec,
    const method::delete_& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

void protocol_client::handle_receive_trace(const code& ec,
    const method::trace& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

void protocol_client::handle_receive_options(const code& ec,
    const method::options& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

void protocol_client::handle_receive_connect(const code& ec,
    const method::connect& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

void protocol_client::handle_receive_unknown(const code& ec,
    const method::unknown& request) NOEXCEPT
{
    send_method_not_allowed(*request, ec);
}

// Handle sends.
// ----------------------------------------------------------------------------

void protocol_client::handle_complete(const code& ec,
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
// private

// TODO: sanitize_origin() must be enhanced for security purposes.
const std::filesystem::path protocol_client::to_local_path(
    const std::string& target) const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return sanitize_origin(root_, target == "/" ? target + default_ : target);
}

bool protocol_client::is_allowed_host(const std::string& host) const NOEXCEPT
{
    if (host_names_.empty())
        return true;

    using namespace system;
    return contains(host_names_, ascii_to_lower(host));
}

void protocol_client::add_common_headers(http_fields& fields,
    const http_string_request& request, bool closing) const NOEXCEPT
{
    // date
    // ------------------------------------------------------------------------
    fields.set(field::date, format_http_time(zulu_time()));

    // server
    // ------------------------------------------------------------------------
    if (!server_.empty())
        fields.set(field::server, server_);

    // connection
    // ------------------------------------------------------------------------
    // http 1.1 assumes keep-alive if not specified, http 1.0 does not.
    // Beast parser defaults keep-alive to true in http 1.1 requests.

    if (closing || !request.keep_alive())
    {
        fields.set(field::connection, "close");
        return;
    }

    if (request.version() < 11u)
        fields.set(field::connection, "keep-alive");

    // keep_alive
    // ------------------------------------------------------------------------
    // The keep_alive.timeout field is encoded as seconds.

    if (!is_zero(timeout_))
        fields.set(field::keep_alive, "timeout=" + serialize(timeout_));
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
