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
#include <bitcoin/network/net/socket.hpp>

#include <variant>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace std::placeholders;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

void socket::rpc_read(http::flat_buffer& buffer, rpc::request& request,
    count_handler&& handler) NOEXCEPT
{
    // Create variant http request to capture read.
    const auto in = to_shared<http::request>();

    // Preselect rpc::request body value type, propagating caller options.
    in->body() = rpc::request{ .strict = request.strict };

    // Capture body and move it back into request reference.
    body_read(buffer, *in,
        std::bind(&socket::handle_rpc_read,
            shared_from_this(), _1, _2, std::ref(request), in,
            std::move(handler)));
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
    http::response out{};
    out.body() = std::move(response);
    body_write(std::move(out), std::move(handler));
}

void socket::rpc_notify(rpc::request&& notification,
    count_handler&& handler) NOEXCEPT
{
    http::request out{};
    out.body() = std::move(notification);
    body_notify(std::move(out), std::move(handler));
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
