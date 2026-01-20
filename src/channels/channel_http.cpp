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
#include <bitcoin/network/channels/channel_http.hpp>

#include <utility>
#include <variant>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

#define CLASS channel_http
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

// Failure to call after successful message handling causes stalled channel.
void channel_http::receive() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT_MSG(!reading_, "already reading");

    if (stopped() || paused() || reading_)
        return;

    reading_ = true;
    const auto in = to_shared<request>();

    // Post handle_read to strand upon stop, error, or buffer full.
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

    reading_ = false;
    log_message(*request, bytes);
    dispatch(request);
}

// Wrap the http request as a tagged verb request and dispatch by type.
void channel_http::dispatch(const request_cptr& request) NOEXCEPT
{
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

flat_buffer& channel_http::request_buffer() NOEXCEPT
{
    return request_buffer_;
}

// Send.
// ----------------------------------------------------------------------------

void channel_http::send(response&& response, result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    assign_json_buffer(response);
    const auto ptr = system::move_shared(std::move(response));
    count_handler complete = std::bind(&channel_http::handle_send,
        shared_from_base<channel_http>(), _1, _2, ptr, std::move(handler));

    if (!ptr)
    {
        complete(error::bad_alloc, {});
        return;
    }

    // response has been moved to ptr.
    write(*ptr, std::move(complete));
}

void channel_http::handle_send(const code& ec, size_t bytes,
    const response_cptr& response, const result_handler& handler) NOEXCEPT
{
    if (ec)
        stop(ec);

    log_message(*response, bytes);
    handler(ec);
}

// private
void channel_http::assign_json_buffer(response& response) NOEXCEPT
{
    if (const auto& body = response.body();
        body.contains<json_body::value_type>())
    {
        auto& value = body.get<json_body::value_type>();
        response_buffer_->max_size(value.size_hint);
        value.buffer = response_buffer_;
    }
}

// log helpers
// ----------------------------------------------------------------------------

void channel_http::log_message(const request& LOG_ONLY(request),
    size_t LOG_ONLY(bytes)) const NOEXCEPT
{
    LOG_ONLY(const auto version = "http/" + serialize(request.version() / 10) +
        "." + serialize(request.version() % 10);)
        
    LOGA("Http [" << request.method_string()
        << "] " << version
        << " (" << (request.chunked() ? "c" : serialize(bytes))
        << ") " << (request.keep_alive() ? "keep" : "drop")
        << " [" << endpoint() << "]"
        << " {" << (split(request[field::accept], ",").front()) << "...}"
        << " "  << request.target());
}

void channel_http::log_message(const response& LOG_ONLY(response),
    size_t LOG_ONLY(bytes)) const NOEXCEPT
{
    LOG_ONLY(const auto version = "http/" + serialize(response.version() / 10)
        + "." + serialize(response.version() % 10);)
        
    LOGA("Http [" << status_string(response.result())
        << "] " << version
        << " (" << (response.chunked() ? "c" : serialize(bytes))
        << ") " << (response.keep_alive() ? "keep" : "drop")
        << " [" << endpoint() << "]"
        << " {" << (response[field::content_type]) << "}");
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
