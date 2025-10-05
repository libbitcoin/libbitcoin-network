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

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/rpc/messages.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_client

using namespace asio;
using namespace messages::rpc;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// TODO: get from config (need access to 'server' config).
protocol_client::protocol_client(const session::ptr& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    channel_(std::dynamic_pointer_cast<channel_client>(channel)),
    session_(std::dynamic_pointer_cast<session_client>(session)),
    root_(system::to_path("m:/server")),
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

// Inbound/outbound.
// ----------------------------------------------------------------------------

// Handle get method.

void protocol_client::handle_receive_get(const code& ec,
    const method::get& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped(ec))
        return;

    const auto version = request->version();
    if (request->target() == "/")
    {
        // Default path GET returns HTML form.
        http_string_response response{ http::status::ok, version };
        response.set(http::field::content_type, "text/html; charset=UTF-8");
        response.body() = get_form();
        response.prepare_payload();
        SEND(std::move(response), handle_successful_request, _1);
        return;
    }

    // Empty path implies invalid.
    const auto path = sanitize_origin(root_, request->target());

    // favicon.ico path GET returns image.
    if (get_mime_type(path).starts_with("image/"))
    {
        auto file = get_file_body(path);
        if (file.is_open())
        {
            http_file_response response{ http::status::ok, version };
            response.set(http::field::content_type, get_mime_type(path));
            response.body() = std::move(file);
            response.prepare_payload();
            SEND(std::move(response), handle_failed_request, _1, error::im_a_teapot);
            return;
        }

        // Return 404 Not Found.
        http_string_response response{ http::status::not_found, version };
        response.set(http::field::server, BC_USER_AGENT);
        response.body() += BC_USER_AGENT;
        response.body() += "\r\nfile   : ";
        response.body() += request->target();
        response.prepare_payload();
        SEND(std::move(response), handle_failed_request, _1, error::not_found);
        return;
    }

    // Other GETs just echo.
    http_string_response response{ http::status::ok, version };
    response.set(http::field::content_type, "text/plain");
    response.body() += BC_USER_AGENT;
    response.body() += "\r\nmethod : ";
    response.body() += request->method_string();
    response.body() += "\r\ntarget : ";
    response.body() += request->target();
    response.body() += "\r\nversion: ";
    response.body() += system::serialize(version);
    response.body() += "\r\npayload: ";
    response.body() += system::serialize(request->body().size());
    response.body() += "\r\n";
    response.body() += request->body();
    response.prepare_payload();
    SEND(std::move(response), handle_successful_request, _1);
}

// Handle post method.
// ----------------------------------------------------------------------------

void protocol_client::handle_receive_post(const code& ec,
    const method::post& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped(ec))
        return;

    // Echo.
    const auto version = request->version();
    http_string_response response{ http::status::ok, version };
    response.set(http::field::content_type, "text/plain");
    response.body() += BC_USER_AGENT;
    response.body() += "\r\nmethod : ";
    response.body() += request->method_string();
    response.body() += "\r\ntarget : ";
    response.body() += request->target();
    response.body() += "\r\nversion: ";
    response.body() += system::serialize(version);
    response.body() += "\r\npayload: ";
    response.body() += system::serialize(request->body().size());
    response.body() += "\r\n";
    response.body() += request->body();
    response.prepare_payload();
    SEND(std::move(response), handle_successful_request, _1);
}

// Handle disallosed methods.
// ----------------------------------------------------------------------------

void protocol_client::handle_receive_put(const code& ec,
    const method::put& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    send_not_allowed(ec, request.ptr);
}

void protocol_client::handle_receive_head(const code& ec,
    const method::head& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    send_not_allowed(ec, request.ptr);
}

void protocol_client::handle_receive_delete(const code& ec,
    const method::delete_& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    send_not_allowed(ec, request.ptr);
}

void protocol_client::handle_receive_trace(const code& ec,
    const method::trace& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    send_not_allowed(ec, request.ptr);
}

void protocol_client::handle_receive_options(const code& ec,
    const method::options& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    send_not_allowed(ec, request.ptr);
}

void protocol_client::handle_receive_connect(const code& ec,
    const method::connect& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    send_not_allowed(ec, request.ptr);
}

void protocol_client::handle_receive_unknown(const code& ec,
    const method::unknown& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    send_not_allowed(ec, request.ptr);
}

void protocol_client::send_not_allowed(const code& ec,
    const http_string_request_cptr& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(ec || request, "success with null request");

    if (stopped(ec))
        return;

    const auto version = request->version();
    http_string_response response{ http::status::method_not_allowed, version };
    SEND(std::move(response), handle_failed_request, _1, error::method_not_allowed);
}

// Handle sends.
// ----------------------------------------------------------------------------

// Invoked to continue as half-duplex, required if not stopping.
void protocol_client::handle_successful_request(const code&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    channel_->read_request();
}

// Invoked to terminate connection after error response (as applicable).
void protocol_client::handle_failed_request(const code&,
    const code& reason) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop(reason);
}

// Utility.
// ----------------------------------------------------------------------------

// static/private
const std::string& protocol_client::get_form() NOEXCEPT
{
    static const std::string form
    {
R"(<!DOCTYPE html>
<html>
<head>
    <title>Login Form</title>
    <link rel="icon" type="image/x-icon" href="favicon.ico">
</head>
<body>
    <h1>All your nodes are us!</h1>
    <form action="/submit" method="POST">
        <label for="username">Username:</label>
        <input type="text" id="username" name="username" placeholder="Enter username"><br>
        <label for="password">Password:</label>
        <input type="password" id="password" name="password" placeholder="Enter password"><br>
        <input type="submit" value="Submit">
    </form>
</body>
</html>)" 
    };

    return form;
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
