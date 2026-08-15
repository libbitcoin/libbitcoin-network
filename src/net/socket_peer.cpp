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

// The message is framed, so reads are exact (see socket_body.cpp for the
// taxonomy), obtaining the heading and then the payload it indicates. Writes
// pass the messages::peer::body type to the body_ methods.

void socket::peer_read(data_chunk& buffer, frame& message,
    count_handler&& handler) NOEXCEPT
{
    boost_code ec{};
    const auto in = emplace_shared<peer_state>(message, buffer);
    in->reader.init({}, ec);

    boost::asio::dispatch(strand_,
        std::bind(&socket::do_peer_read,
            shared_from_this(), zero, in, std::move(handler)));
}

// private
void socket::do_peer_read(size_t total, const peer_state::ptr& in,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // The heading is read into fixed storage, the payload into the caller
    // buffer, sized by the heading.
    const auto need = in->reader.need();

    if (in->headed)
        in->payload.resize(need);

    const auto data = in->headed ? in->payload.data() : in->head.data();

    async_read(asio::mutable_buffer{ data, need },
        std::bind(&socket::handle_peer_read,
            shared_from_this(), _1, _2, total, in, handler));
}

// private
void socket::handle_peer_read(const code& ec, size_t size, size_t total,
    const peer_state::ptr& in, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    total = ceilinged_add(total, size);

    if (ec)
    {
        handler(ec, total);
        return;
    }

    boost_code code{};
    const auto data = in->headed ? in->payload.data() : in->head.data();
    in->reader.put(asio::const_buffer{ data, size }, code);

    if (code)
    {
        handler(error::http_to_error_code(code), total);
        return;
    }

    if (in->reader.done())
    {
        in->reader.finish(code);
        handler(error::http_to_error_code(code), total);
        return;
    }

    in->headed = true;
    do_peer_read(total, in, handler);
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
