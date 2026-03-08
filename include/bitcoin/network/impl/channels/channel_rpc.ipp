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

using namespace std::placeholders;

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

    // Release the batch shared_ptr on stop to avoid holding the allocation
    // longer than necessary and to make the stopped state unambiguous.
    batch_source_.reset();
    batch_cursor_ = 0;
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
            // These are most likely to be json-rpc parse errors (remote).
            LOGR("Rpc read failure [" << endpoint() << "] " << ec.message());
        }

        stop(ec);
        return;
    }

    // RESOLVED: Extend support to batch (array of rpc).
    // RESOLVED: This would consist of asynchronous recursion here, with iteration
    // RESOLVED: over the message array. The response is accumulated, but there is
    // RESOLVED: no way we would buffer it at the server until complete, which is a
    // RESOLVED: clear DoS vector. We would instead track the iteration and send
    // RESOLVED: each response with the necessary delimiters. This allows a request
    // RESOLVED: to safely be of any configured byte size or request element count.
    // Implemented via dispatch_batch() + dispatch_next() + batch_source_/batch_cursor_.
    // One response is in flight at a time; handle_send() advances the cursor.

    reading_ = false;

    if (request->is_batch())
    {
        LOGA("Rpc batch   : (" << bytes << ") bytes [" << endpoint() << "] "
            << request->batch.size() << " item(s).");
        dispatch_batch(request);
    }
    else
    {
        // Save response identity for this request (single path, unchanged).
        identity_ = request->message.id;
        version_  = request->message.jsonrpc;

        LOGA("Rpc request : (" << bytes << ") bytes [" << endpoint() << "] "
            << request->message.method << "(...).");
        dispatch(request);
    }
}

TEMPLATE
inline void CLASS::dispatch(const rpc::request_cptr& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    if (const auto code = dispatcher_.notify(request->message))
        stop(code);
}

TEMPLATE
inline void CLASS::dispatch_batch(const rpc::request_cptr& request) NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(request->is_batch());
    BC_ASSERT_MSG(!batch_source_, "nested batch");

    // Hold the parsed request alive by shared_ptr; no copy of batch items.
    batch_source_ = request;
    batch_cursor_ = 0;
    dispatch_next();
}

TEMPLATE
inline void CLASS::dispatch_next() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(batch_source_);

    const auto& items = batch_source_->batch;

    // Skip notification items (no id): dispatch for side-effects only.
    // Per JSON-RPC 2.0 notifications must receive no response.
    // Unknown method on a notification is silently ignored (not a channel error).
    while (batch_cursor_ < items.size()
        && !items[batch_cursor_].id.has_value())
    {
        dispatcher_.notify(items[batch_cursor_++]);

        // A notification handler could call stop(). Bail if so.
        if (stopped()) return;
    }

    // All remaining items consumed (may have been all notifications).
    if (batch_cursor_ >= items.size())
    {
        batch_source_.reset();
        batch_cursor_ = 0;
        receive();
        return;
    }

    // Dispatch the next request item (has id, must receive a response).
    const auto& req = items[batch_cursor_++];
    identity_ = req.id;
    version_  = req.jsonrpc;

    if (const auto ec = dispatcher_.notify(req))
    {
        // Unknown method: send a per-item error response.
        // handle_send() will call dispatch_next() for us when the write
        // completes, advancing to the next batch item.
        send_error({ .code = ec.value(), .message = ec.message() },
            [](const code&) NOEXCEPT {});
        return;
    }

    // Success: the matched subscriber handler will eventually call
    // send_result() or send_code(), which routes through handle_send(),
    // which calls dispatch_next() to advance the cursor.
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
    send_error({ .code = ec.value(), .message = ec.message() },
        std::move(handler));
}

TEMPLATE
void CLASS::send_error(rpc::result_t&& error,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    send({ .jsonrpc = version_, .id = identity_, .error = std::move(error) },
        two * error.message.size(), std::move(handler));
}

TEMPLATE
void CLASS::send_result(rpc::value_t&& result, size_t size_hint,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    send({ .jsonrpc = version_, .id = identity_, .result = std::move(result) },
        size_hint, std::move(handler));
}

// protected
TEMPLATE
inline void CLASS::send(rpc::response_t&& model, size_t size_hint,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
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

    // Typically a noop, but handshake may pause channel here.
    handler(ec);

    LOGA("Rpc response: (" << bytes << ") bytes [" << endpoint() << "] "
        << response->message.error.value_or(rpc::result_t{}).message);

    // Continue: advance batch or read the next top-level message.
    if (batch_source_)
        dispatch_next();
    else
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

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin

#endif
