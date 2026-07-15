/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/channels/channel_http.hpp>

#include <utility>
#include <variant>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

#define CASE_REQUEST_TO_MODEL(verb_, request_, model_) \
case verb::verb_: \
    model_.method = #verb_; \
    model_.params = { rpc::array_t{ rpc::any_t{ \
        http::method::tag_request<verb::verb_>(request_) } } }; \
    break

using namespace system;
using namespace network::http;
using namespace std::placeholders;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Start/stop/resume (started upon create).
// ----------------------------------------------------------------------------

// This should not be called internally, as derived rely on stop() override.
void channel_http::stopping(const code& ec) NOEXCEPT
{
    BC_ASSERT(stranded());
    channel::stopping(ec);
    dispatcher_.stop(ec);
}

void channel_http::resume() NOEXCEPT
{
    BC_ASSERT(stranded());
    channel::resume();
    receive();
}

// Read cycle.
// ----------------------------------------------------------------------------
// Failure to call receive() after successful message handling stalls channel.

void channel_http::receive() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT_MSG(!reading_, "already reading");

    if (stopped() || paused() || reading_)
        return;

    reading_ = true;
    const auto in = create_request();

    read(request_buffer(), *in,
        std::bind(&channel_http::handle_receive,
            shared_from_base<channel_http>(), _1, _2, in));
}

void channel_http::handle_receive(const code& ec, size_t bytes,
    const http::request_cptr& request) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped())
    {
        LOGQ("Http read abort [" << endpoint() << "]");
        return;
    }

    if (ec == error::upgraded)
    {
        // Don't dispatch the upgrade request, just restart the reader.
        reading_ = false;
        receive();
        return;
    }

    if (ec)
    {
        // Don't log common conditions.
        if (ec != error::end_of_stream && ec != error::operation_canceled)
        {
            LOGF("Http read failure [" << endpoint() << "] "
                << ec.message());
        }

        stop(ec);
        return;
    }

    LOGV(log_message(*request, bytes));

    reading_ = false;
    dispatch(request);
}

// Wrap the http request as a tagged verb request and dispatch by type.
void channel_http::dispatch(const request_cptr& request) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (unauthorized(*request))
    {
        send({ status::unauthorized, request->version() },
            std::bind(&channel_http::handle_unauthorized,
                shared_from_base<channel_http>(), _1));
        return;
    }

    rpc::request_t model{};
    switch (request.get()->method())
    {
        CASE_REQUEST_TO_MODEL(get, request, model);
        CASE_REQUEST_TO_MODEL(head, request, model);
        CASE_REQUEST_TO_MODEL(post, request, model);
        CASE_REQUEST_TO_MODEL(put, request, model);
        CASE_REQUEST_TO_MODEL(delete_, request, model);
        CASE_REQUEST_TO_MODEL(trace, request, model);
        CASE_REQUEST_TO_MODEL(options, request, model);
        CASE_REQUEST_TO_MODEL(connect, request, model);

        default:
        CASE_REQUEST_TO_MODEL(unknown, request, model);
    }

    if (const auto code = dispatcher_.notify(model))
        stop(code);
}

// request helpers
// ----------------------------------------------------------------------------

flat_buffer& channel_http::request_buffer() NOEXCEPT
{
    BC_ASSERT(stranded());
    return request_buffer_;
}

body::value_type channel_http::websocket_body() const NOEXCEPT
{
    // There is no forwarding constructor so assign and move.
    body::value_type value{};
    value = json_value{};
    return value;
}

// private
request_ptr channel_http::create_request() const NOEXCEPT
{
    BC_ASSERT(stranded());

    const auto out = to_shared<request>();
    if (websocket())
    {
        // out->method() will return verb::unknown (mapped in dispatch).
        // socket will not produce verb::unknown for http requests (blocked). 
        // plain_json value is not necessary since reader is explicitly set.
        out->method_string("websocket");
        out->body() = websocket_body();
    }

    return out;
}

// Send/notify.
// ----------------------------------------------------------------------------

void channel_http::send(response&& response, result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    assign_json_buffer(response);

    std::string message{ LOG_ONLY(log_message(response)) };

    write(std::move(response),
        std::bind(&channel_http::handle_send,
            shared_from_base<channel_http>(), _1, _2, false, std::move(message),
                std::move(handler)));
}

void channel_http::notify(response&& notification,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(websocket());

    std::string message{ LOG_ONLY(log_message(notification)) };

    write(std::move(notification),
        std::bind(&channel_http::handle_send,
            shared_from_base<channel_http>(), _1, _2, true, std::move(message),
                std::move(handler)));
}

void channel_http::handle_send(const code& ec, size_t bytes, bool notification,
    const std::string& message, const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec) stop(ec);

    // Do not log websocket sends as it creates log subscription feedback loop.
    if (!websocket()) { LOGV(boost_format(message) % bytes); }
    handler(ec);

    // Restart the listener (only in response to requests).
    if (!notification)
        receive();
}

// private
void channel_http::assign_json_buffer(response& response) NOEXCEPT
{
    BC_ASSERT(stranded());

    // websocket is full duplex, so cannot use shared json repsonse buffer.
    if (!websocket())
    {
        const auto& body = response.body();
        if (body.contains<json_body::value_type>())
            body.get<json_body::value_type>().buffer = response_buffer_;
        else if (body.contains<rpc::request>())
            body.get<rpc::request>().buffer = response_buffer_;
    }
}

// unauthorized helpers
// ----------------------------------------------------------------------------

bool channel_http::unauthorized(const request& request) NOEXCEPT
{
    return options_.authorize() &&
        (options_.credential() != request[field::authorization]);
}

void channel_http::handle_unauthorized(const code& ec) NOEXCEPT
{
    BC_ASSERT(stranded());
    stop(ec ? ec : error::unauthorized);
}

// log helpers
// ----------------------------------------------------------------------------

std::string channel_http::log_message(const request& request,
    size_t bytes) const NOEXCEPT
{
    if (websocket())
    {
        const std::string scheme = secure() ? "wss" : "ws";
        const std::string size = serialize(bytes);
        return scheme + " (" + size + ") [" + endpoint().to_string() + "] " +
            std::string(request.target());
    }

    const std::string scheme = secure() ? "https" : "http";
    const std::string method = request.method_string();
    const std::string keep = request.keep_alive() ? "keep" : "drop";
    const std::string size = request.chunked() ? "c" : serialize(bytes);
    const std::string accept = split(request[field::accept], ",").front();
    const std::string version = "http/" + serialize(request.version() / 10) +
        "." + serialize(request.version() % 10);

    return scheme + " [" + method + "] " + version + " (" + size + ") " +
        keep + " [" + endpoint().to_string() + "] " +
        "{" + accept + "...} " + std::string(request.target());
}

std::string channel_http::log_message(const response& response) const NOEXCEPT
{
    if (websocket())
    {
        const std::string scheme = secure() ? "wss" : "ws";
        return scheme + " [" + endpoint().to_string() + "]";
    }

    const std::string scheme = secure() ? "https" : "http";
    const std::string status = status_string(response.result());
    const std::string keep = response.keep_alive() ? "keep" : "drop";
    const std::string version = "http/" + serialize(response.version() / 10) +
        "." + serialize(response.version() % 10);

    // %1% is required placeholder for response size (filled by caller).
    return scheme + " [" + status + "] " + version + " (%1%) " +
        keep + " [" + endpoint().to_string() + "] " +
        "{" + std::string(response[field::content_type]) + "}";
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
