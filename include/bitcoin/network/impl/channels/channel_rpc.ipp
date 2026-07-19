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
#ifndef LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_RPC_IPP
#define LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_RPC_IPP

#include <memory>
#include <utility>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Start/stop/resume (started upon create).
// ----------------------------------------------------------------------------

// This should not be called internally, as derived rely on stop() override.
TEMPLATE
inline void CLASS::stopping(const code& ec) NOEXCEPT
{
    BC_ASSERT(stranded());
    channel::stopping(ec);
    dispatcher_.stop(ec);
}

TEMPLATE
inline void CLASS::resume() NOEXCEPT
{
    BC_ASSERT(stranded());
    channel::resume();
    receive();
}

// Read cycle.
// ----------------------------------------------------------------------------
// Failure to call receive() after successful message handling stalls channel.

TEMPLATE
inline void CLASS::receive() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT_MSG(!reading_, "already reading");
    using namespace std::placeholders;
    using namespace system;

    if (stopped() || paused() || reading_)
        return;

    reading_ = true;
    const auto in = create_request();
    
    read(request_buffer(), *in,
        std::bind(&channel_rpc::handle_receive,
            shared_from_base<channel_rpc>(), _1, _2, in));
}

TEMPLATE
inline void CLASS::handle_receive(const code& ec, size_t bytes,
    const rpc::request_cptr& request) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped())
    {
        LOGQ("Rpc read abort [" << endpoint() << "]");
        return;
    }

    if (ec)
    {
        // Don't log common conditions.
        if (ec != error::end_of_stream && ec != error::operation_canceled)
        {
            // These are most likely to be json-rpc parse errors (remote).
            LOGR("Rpc read failure [" << endpoint() << "] " << ec.message());
        }

        stop(ec);
        return;
    }

    // TODO: Extend support to batch (array of rpc).
    // TODO: This would consist of asynchronous recursion here, with iteration
    // TODO: over the message array. The response is accumulated, but there is
    // TODO: no way we would buffer it at the server until complete, which is a
    // TODO: clear DoS vector. We would instead track the iteration and send
    // TODO: each response with the necessary delimiters. This allows a request
    // TODO: to safely be of any configured byte size or request element count.

    // Save response state.
    identity_ = request->message.id;
    version_ = request->message.jsonrpc;

    LOGA("Rpc request : (" << bytes << ") bytes [" << endpoint() << "] "
        << request->message.method << "(...).");

    reading_ = false;
    dispatch(request);
}

TEMPLATE
inline void CLASS::dispatch(const rpc::request_cptr& request) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Electrum laxness (single value params) is allowed, btcd laxness
    // (batched v1) is not (the rpc channel is jrpc over tcp).
    if (request->lax_batch)
    {
        stop(error::jsonrpc_batch_requires_v2);
        return;
    }

    if (const auto code = dispatcher_.notify(request->message))
        stop(code);
}

// request helpers
// ----------------------------------------------------------------------------

TEMPLATE
inline http::flat_buffer& CLASS::request_buffer() NOEXCEPT
{
    BC_ASSERT(stranded());
    return request_buffer_;
}

TEMPLATE
inline rpc::request_ptr CLASS::create_request() const NOEXCEPT
{
    // The parse is always lax, with tolerated jrpc violations and batch
    // state reflected on the value (validated by channel dispatch).
    return system::to_shared<rpc::request>();
}

// Send.
// ----------------------------------------------------------------------------

template <typename Message>
std::string extract_method(const Message& message) NOEXCEPT
{
    if constexpr (is_same_type<Message, rpc::response_t>)
        return "response: " + message.error.value_or(rpc::result_t{}).message;
    else
        return "request: " + message.method;
}

// protected
TEMPLATE
template <typename Message>
inline void CLASS::send(Message&& message, size_t size_hint,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    using namespace std::placeholders;

    auto complete = std::bind(&CLASS::handle_send<Message>,
        shared_from_base<CLASS>(), _1, _2, extract_method(message),
        std::move(handler));

    // Write message (response or notification) to socket.
    write(
    {
        json::json_value{ .size_hint = size_hint },
        std::forward<Message>(message)
    }, std::move(complete));
}

// protected
TEMPLATE
template <typename Message>
inline void CLASS::handle_send(const code& ec, size_t bytes,
    const std::string& method, const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (ec)
        stop(ec);

    LOGA("Rpc " << method << " (" << bytes << ") bytes [" << endpoint() << "] ");

    // Typically a noop, but handshake may pause channel here.
    handler(ec);

    // Restart the listener (only in response to requests).
    if constexpr (is_same_type<Message, rpc::response_t>)
    {
        receive();
    }
}

TEMPLATE
inline void CLASS::send_code(const code& ec, result_handler&& handler) NOEXCEPT
{
    send_error(
    {
        .code = ec.value(),
        .message = ec.message()
    }, std::move(handler));
}

TEMPLATE
inline void CLASS::send_error(rpc::result_t&& error,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    const auto hint = two * error.message.size();
    send(rpc::response_t
    {
        .jsonrpc = version_,
        .id = identity_,
        .error = std::move(error)
    }, hint, std::move(handler));
}

TEMPLATE
inline void CLASS::send_result(rpc::value_t&& result, size_t size_hint,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    send(rpc::response_t
    {
        .jsonrpc = version_,
        .id = identity_,
        .result = std::move(result)
    }, size_hint, std::move(handler));
}

TEMPLATE
inline void CLASS::send_notification(rpc::string_t&& method,
    rpc::params_t&& notification, size_t size_hint,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    send(rpc::request_t
    {
        .jsonrpc = version_,
        .method = std::move(method),
        .params = std::move(notification)
    }, size_hint, std::move(handler));
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin

#endif
