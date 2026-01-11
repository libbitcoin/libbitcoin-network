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

// Failure to call after successful message handling causes stalled channel.
TEMPLATE
inline void CLASS::receive() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT_MSG(!reading_, "already reading");

    if (stopped() || paused() || reading_)
        return;

    reading_ = true;
    const auto in = system::to_shared<rpc::request>();
    using namespace std::placeholders;

    // Post handle_read to strand upon stop, error, or buffer full.
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
            LOGF("Rpc read failure [" << endpoint() << "] "
                << ec.message());
        }

        stop(ec);
        return;
    }

    // Save response state.
    identity_ = request->message.id;
    version_ = request->message.jsonrpc;

    reading_ = false;
    log_message(*request, bytes);
    dispatch(request);
}

TEMPLATE
inline void CLASS::dispatch(const rpc::request_cptr& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (const auto code = dispatcher_.notify(request->message))
        stop(code);
}

TEMPLATE
inline http::flat_buffer& CLASS::request_buffer() NOEXCEPT
{
    BC_ASSERT(stranded());
    return request_buffer_;
}

// Send.
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::send_code(const code& ec, result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    send_error({ .code = ec.value(), .message = ec.message() },
        std::move(handler));
}

TEMPLATE
void CLASS::send_error(rpc::result_t&& error,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    using namespace std::placeholders;
    send({ .jsonrpc = version_, .id = identity_, .error = std::move(error) },
        two * error.message.size(), std::move(handler));
}

TEMPLATE
void CLASS::send_result(rpc::value_t&& result, size_t size_hint,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    using namespace std::placeholders;
    send({ .jsonrpc = version_, .id = identity_, .result = std::move(result) },
        size_hint, std::move(handler));
}

// protected
TEMPLATE
inline void CLASS::send(rpc::response_t&& model, size_t size_hint,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    using namespace std::placeholders;
    const auto out = assign_message(std::move(model), size_hint);
    count_handler complete = std::bind(&CLASS::handle_send,
        shared_from_base<CLASS>(), _1, _2, out, std::move(handler));

    if (!out)
    {
        complete(error::bad_alloc, {});
        return;
    }
    
    write(*out, std::move(complete));
}

// protected
TEMPLATE
inline void CLASS::handle_send(const code& ec, size_t bytes,
    const rpc::response_cptr& response, const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (ec) stop(ec);
    log_message(*response, bytes);

    // Typically a noop, but handshake may pause channel here.
    handler(ec);

    // Continue read loop (does not unpause or restart channel).
    receive();
}

// private
TEMPLATE
inline rpc::response_ptr CLASS::assign_message(rpc::response_t&& message,
    size_t size_hint) NOEXCEPT
{
    BC_ASSERT(stranded());
    response_buffer_->max_size(size_hint);
    const auto ptr = system::to_shared<rpc::response>();
    ptr->message = std::move(message);
    ptr->buffer = response_buffer_;
    return ptr;
}

// log helpers
// ----------------------------------------------------------------------------

TEMPLATE
inline void CLASS::log_message(const rpc::request& LOG_ONLY(request),
    size_t LOG_ONLY(bytes)) const NOEXCEPT
{
    LOGA("Rpc request: [" << bytes << "] "
        << request.message.method << "(...).");
}

TEMPLATE
inline void CLASS::log_message(const rpc::response& LOG_ONLY(response),
    size_t LOG_ONLY(bytes)) const NOEXCEPT
{
    LOG_ONLY(const auto message = response.message.error.has_value() ?
        response.message.error.value().message : "success");

    LOGA("Rpc response: [" << bytes << "], " << message << ".");
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin

#endif
