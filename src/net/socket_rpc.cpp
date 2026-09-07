/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#include <bitcoin/network/net/socket.hpp>

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace std::placeholders;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// The transport is selected on the strand: a zmtp socket reads and writes
// the rpc message by its role (see socket_zmtp.cpp), otherwise the message
// is a json-rpc body over tcp/ws.

void socket::rpc_read(http::flat_buffer& buffer, rpc::request& request,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_rpc_read,
            shared_from_this(), std::ref(buffer), std::ref(request),
            std::move(handler)));
}

// private
void socket::do_rpc_read(const ref<http::flat_buffer>& buffer,
    const ref<rpc::request>& request, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (zeromq())
    {
        do_zmtp_read(emplace_shared<zmtp_read_state>(request.get(),
            buffer.get()), handler);
        return;
    }

    // Create variant http request to capture read.
    const auto in = to_shared<http::request>();

    // Preselect rpc::request body value type, propagating caller state.
    in->body() = rpc::request{ .batch = request.get().batch };

    // Capture body and move it back into request reference.
    body_read(buffer.get(), *in,
        std::bind(&socket::handle_rpc_read,
            shared_from_this(), _1, _2, request, in, handler));
}

// private
void socket::handle_rpc_read(const code& ec, size_t bytes,
    const ref<rpc::request>& out, const http::request_ptr& in,
    const count_handler& handler) NOEXCEPT
{
    if (!ec)
    {
        // Move rpc::request from http body value to caller out param.
        out.get() = std::move(std::get<rpc::request>(in->body().value()));
    }

    handler(ec, bytes);
}

void socket::rpc_write(rpc::response&& response,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_rpc_write,
            shared_from_this(), move_shared(std::move(response)),
            std::move(handler)));
}

// private
void socket::do_rpc_write(const rpc::response_ptr& response,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (zeromq())
    {
        do_zmtp_response(response, handler);
        return;
    }

    // Stream (tcp/ws) messages are newline terminated (http chunks are not).
    response->terminate = true;

    http::response out{};
    out.body() = std::move(*response);
    body_write(std::move(out), count_handler{ handler });
}

void socket::rpc_notify(rpc::request&& notification,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_rpc_notify,
            shared_from_this(), move_shared(std::move(notification)),
            std::move(handler)));
}

// private
void socket::do_rpc_notify(const rpc::request_ptr& notification,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (zeromq())
    {
        do_zmtp_notify(notification, handler);
        return;
    }

    // Stream (tcp/ws) messages are newline terminated (http chunks are not).
    notification->terminate = true;

    http::request out{};
    out.body() = std::move(*notification);
    body_notify(std::move(out), count_handler{ handler });
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
