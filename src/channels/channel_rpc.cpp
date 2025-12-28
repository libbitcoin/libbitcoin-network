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
#include <bitcoin/network/channels/channel_rpc.hpp>

#include <memory>
#include <utility>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace network::rpc;
using namespace std::placeholders;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Start/stop/resume (started upon create).
// ----------------------------------------------------------------------------

// This should not be called internally, as derived rely on stop() override.
void channel_rpc::stopping(const code& ec) NOEXCEPT
{
    BC_ASSERT(stranded());
    channel::stopping(ec);
    dispatcher_.stop(ec);
}

void channel_rpc::resume() NOEXCEPT
{
    BC_ASSERT(stranded());
    channel::resume();
    receive();
}

// Read cycle.
// ----------------------------------------------------------------------------

// Failure to call after successful message handling causes stalled channel.
void channel_rpc::receive() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT_MSG(!reading_, "already reading");

    if (stopped() || paused() || reading_)
        return;

    reading_ = true;
    const auto in = to_shared<request>();

    // Post handle_read to strand upon stop, error, or buffer full.
    read(request_buffer(), *in,
        std::bind(&channel_rpc::handle_receive,
            shared_from_base<channel_rpc>(), _1, _2, in));
}

void channel_rpc::handle_receive(const code& ec, size_t bytes,
    const request_cptr& request) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped())
    {
        LOGQ("Rpc read abort [" << authority() << "]");
        return;
    }

    if (ec)
    {
        // Don't log common conditions.
        if (ec != error::end_of_stream && ec != error::operation_canceled)
        {
            LOGF("Rpc read failure [" << authority() << "] "
                << ec.message());
        }

        stop(ec);
        return;
    }

    reading_ = false;
    log_message(*request, bytes);
    dispatch(request);
}

void channel_rpc::dispatch(const request_cptr& request) NOEXCEPT
{
    if (const auto code = dispatcher_.notify(request->message))
        stop(code);
}

http::flat_buffer& channel_rpc::request_buffer() NOEXCEPT
{
    return request_buffer_;
}

// Send.
// ----------------------------------------------------------------------------

void channel_rpc::send(response_t&& model, size_t size_hint,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    const auto out = assign_message(std::move(model), size_hint);
    count_handler complete = std::bind(&channel_rpc::handle_send,
        shared_from_base<channel_rpc>(), _1, _2, out, std::move(handler));

    if (!out)
    {
        complete(error::bad_alloc, {});
        return;
    }
    
    write(*out, std::move(complete));
}

void channel_rpc::handle_send(const code& ec, size_t bytes,
    const response_cptr& response, const result_handler& handler) NOEXCEPT
{
    if (ec)
        stop(ec);

    log_message(*response, bytes);
    handler(ec);
}

// private
response_ptr channel_rpc::assign_message(response_t&& message,
    size_t size_hint) NOEXCEPT
{
    response_buffer_->max_size(size_hint);
    const auto ptr = to_shared<response>();
    ptr->message = std::move(message);
    ptr->buffer = response_buffer_;
    return ptr;
}

// log helpers
// ----------------------------------------------------------------------------

void channel_rpc::log_message(const request& LOG_ONLY(request),
    size_t LOG_ONLY(bytes)) const NOEXCEPT
{
    LOGA("Rpc request: [" << bytes << "] "
        << request.message.method << "(...).");
}

void channel_rpc::log_message(const response& LOG_ONLY(response),
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
