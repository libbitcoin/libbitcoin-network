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

#include <utility>
#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace messages::peer;
using namespace std::placeholders;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// The peer_ methods are a specialization of the body_ methods, passing the
// messages::peer::body type (see socket_body.cpp for the taxonomy). Reads
// surface one message per completion, writes emit the frame serialized by
// peer::serialize (typed serialization requires the caller's static type).

void socket::peer_read(http::flat_buffer& buffer,
    frame& message, count_handler&& handler) NOEXCEPT
{
    // Create variant http request to capture read.
    const auto in = to_shared<http::request>();

    // Preselect peer frame body value type, propagating parse context.
    in->body() = frame{ message };

    // Capture body and move it back into message reference.
    body_read(buffer, *in,
        std::bind(&socket::handle_peer_read,
            shared_from_this(), _1, _2, std::ref(message), in,
            std::move(handler)));
}

// private
void socket::handle_peer_read(const code& ec, size_t bytes,
    const ref<frame>& out, const http::request_ptr& in,
    const count_handler& handler) NOEXCEPT
{
    // Move peer frame from http body value to caller out param.
    // Moved on failure as well, as the frame carries parse fault detail.
    out.get() = std::move(std::get<frame>(in->body().value()));
    handler(ec, bytes);
}

void socket::peer_write(frame&& message,
    count_handler&& handler) NOEXCEPT
{
    http::response out{};
    out.body() = std::move(message);
    body_write(std::move(out), std::move(handler));
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
