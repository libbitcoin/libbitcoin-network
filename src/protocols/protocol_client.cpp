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

protocol_client::protocol_client(const session::ptr& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    channel_(std::dynamic_pointer_cast<channel_client>(channel)),
    session_(std::dynamic_pointer_cast<session_client>(session)),
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

    SUBSCRIBE_CHANNEL(http_string_request, handle_receive_request, _1, _2);
    protocol::start();
}

// Inbound/outbound.
// ----------------------------------------------------------------------------

static const std::string form =
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
</html>)";

std::string get_mime_type(const std::string& path) NOEXCEPT
{
    if (path.ends_with(".jpg") || path.ends_with(".jpeg"))
        return "image/jpeg";

    if (path.ends_with(".png"))
        return "image/png";

    if (path.ends_with(".gif"))
        return "image/gif";

    if (path.ends_with(".ico"))
        return "image/x-icon";

    return "application/octet-stream";
}

void protocol_client::handle_receive_request(const code& ec,
    const http_string_request& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    using namespace boost::beast;
    using namespace system;

    if (stopped(ec))
        return;

    if (request.version() != 11u)
    {
        http_string_response response{ http::status::http_version_not_supported, request.version() };
        response.set(http::field::server, BC_USER_AGENT);
        SEND(std::move(response), handle_failed_request, _1, error::http_version_not_supported);
        return;
    }

    // Default path GET returns HTML form.
    if (request.method() == http::verb::get &&
        request.target() == "/")
    {
        http_string_response response{ http::status::ok, request.version() };
        response.set(http::field::content_type, "text/html; charset=UTF-8");
        response.body() = form;
        response.prepare_payload();
        SEND(std::move(response), handle_successful_request, _1);
        return;
    }

    // favicon.ico path GET returns image.
    if (request.method() == http::verb::get &&
        get_mime_type(request.target()).starts_with("image/"))
    {
        auto path = "m:/server/images" + std::string{ request.target() };
        error_code code{};
        http::file_body::value_type file{};

        try
        {
            // file.open does not reset code for success.
            file.open(path.c_str(), file_mode::read, code);
        }
        catch (...)
        {
            code = error::not_found;
        }

        if (code)
        {
            http_string_response response{ http::status::not_found, request.version() };
            response.set(http::field::server, BC_USER_AGENT);
            response.body() += BC_USER_AGENT;
            response.body() += "\r\nfile   : ";
            response.body() += request.target();
            response.body() += "\r\nerror  : ";
            response.body() += code.message();
            response.prepare_payload();
            SEND(std::move(response), handle_failed_request, _1, error::not_found);
            return;
        }

        http_file_response response{ http::status::ok, request.version() };
        response.set(http::field::content_type, get_mime_type(path));
        response.body() = std::move(file);
        response.prepare_payload();
        SEND(std::move(response), handle_failed_request, _1, error::im_a_teapot);
        return;
    }

    // Other GETs just echo.
    if (request.method() == http::verb::get ||
        request.method() == http::verb::post)
    {
        http_string_response response{ http::status::ok, request.version() };
        response.set(http::field::content_type, "text/plain");
        response.body() += BC_USER_AGENT;
        response.body() += "\r\nmethod : ";
        response.body() += request.method_string();
        response.body() += "\r\ntarget : ";
        response.body() += request.target();
        response.body() += "\r\nversion: ";
        response.body() += system::serialize(request.version());
        response.body() += "\r\npayload: ";
        response.body() += system::serialize(request.body().size());
        response.body() += "\r\n";
        response.body() += request.body();
        response.prepare_payload();
        SEND(std::move(response), handle_successful_request, _1);
        return;
    }

    http_string_response response{ http::status::method_not_allowed, request.version() };
    response.set(http::field::server, BC_USER_AGENT);
    SEND(std::move(response), handle_failed_request, _1, error::method_not_allowed);
}

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

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
