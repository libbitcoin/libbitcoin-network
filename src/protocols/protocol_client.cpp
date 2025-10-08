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
    root_(channel->settings().admin.path),
    default_(channel->settings().admin.default_),
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

// Handle get method.
// ----------------------------------------------------------------------------

void protocol_client::handle_receive_get(const code& ec,
    const method::get& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped(ec))
        return;

    const auto version = request->version();

    // Set default path as required.
    auto target = std::string{ request->target() };
    if (target == "/")
        target += default_;

    // Empty path implies invalid.
    const auto path = sanitize_origin(root_, target);
    if (path.empty())
    {
        // Return 404 Not Found and disconnect for bad target error.
        http_string_response response{ http::status::not_found, version };
        response.set(http::field::server, BC_USER_AGENT);
        response.body() += BC_USER_AGENT;
        response.body() += "\r\nfile   : ";
        response.body() += request->target();
        response.prepare_payload();
        SEND(std::move(response), handle_complete, _1, error::bad_target);
        return;
    }

    // Empty implies file not found.
    auto file = get_file_body(path);
    if (!file.is_open())
    {

        // Return 404 Not Found.
        http_string_response response{ http::status::not_found, version };
        response.set(http::field::server, BC_USER_AGENT);
        response.body() += BC_USER_AGENT;
        response.body() += "\r\nfile   : ";
        response.body() += request->target();
        response.prepare_payload();
        SEND(std::move(response), handle_complete, _1, error::not_found);
        return;
    }

    // TODO: if not keep-alive drop with new error::keep_alive.
    const code result = (request->keep_alive() ? error::success :
        error::bad_field);

    http_file_response response{ http::status::ok, version };
    response.set(http::field::content_type, get_mime_type(path));
    response.body() = std::move(file);
    response.prepare_payload();
    SEND(std::move(response), handle_complete, _1, result);
    return;
}

// Handle disallowed methods.
// ----------------------------------------------------------------------------

void protocol_client::handle_receive_post(const code& ec,
    const method::post& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    send_not_allowed(ec, request.ptr);
}

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

// common
void protocol_client::send_not_allowed(const code& ec,
    const http_string_request_cptr& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(ec || request, "success with null request");

    if (stopped(ec))
        return;

    const auto version = request->version();
    http_string_response response{ http::status::method_not_allowed, version };
    SEND(std::move(response), handle_complete, _1, error::method_not_allowed);
}

// Handle sends.
// ----------------------------------------------------------------------------

void protocol_client::handle_complete(const code& ec,
    const code& reason) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        stop(reason);
        return;
    }

    // Continue half duplex.
    channel_->read_request();
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
