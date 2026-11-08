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
#include <bitcoin/network/log/log.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace network::rpc;
using namespace std::placeholders;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// BODY (read).
// ----------------------------------------------------------------------------
// The body_ methods accept any http body (reader/writer) to read/write a tcp
// socket. For http read/write use the http_ methods. The rpc_ methods are just
// a specialization of these methods, passing the rpc::body<> types. For simple
// fixed-size tcp use the tcp_ methods, and for simple framed ws use the ws_
// methods. The body methods require fixed size or framed read/write. The json
// and json-rpc bodies are internally framed by json, so can read/write over a
// raw tcp socket (electrum) or a framed websocket (btcd).

void socket::body_read(http::flat_buffer& buffer, http::request& request,
    count_handler&& handler) NOEXCEPT
{
    const auto parser = emplace_shared<body_parser>(request, !websocket());
    parser->body_limit(maximum_);

    count_handler complete = std::bind(&socket::handle_body_read,
        shared_from_this(), _1, _2, std::move(handler));

    io::dispatch(strand_,
        std::bind(&socket::do_parse<body_parser>,
            shared_from_this(), std::ref(buffer), parser, parse_mode::all,
            zero, std::move(complete)));
}

// private
// Batchable bodies deliver each message by pausing the parse, with residue
// carried in the buffer to the next logical read.
void socket::handle_body_read(const code& ec, size_t size,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    handler(ec == error::need_buffer ? error::success : ec, size);
}

// Body (write).
// ----------------------------------------------------------------------------

// A json body serializes through the retained buffer, which chunks the write
// at its bound. Stream and peer bodies serialize into it whole, within its
// bound. Other bodies write from caller memory.
// The buffer is returned to the minimum once the write has completed.
void socket::assign_buffer(http::body::value_type& body) NOEXCEPT
{
    const auto assign = [this](json::json_value& value) NOEXCEPT
    {
        value.buffer = &response_;
    };

    if (body.contains<http::json_value>())
        assign(body.get<http::json_value>());
    else if (body.contains<rpc::request>())
        assign(body.get<rpc::request>());
    else if (body.contains<rpc::response>())
        assign(body.get<rpc::response>());
    else if (body.contains<http::stream_value>())
        body.get<http::stream_value>().buffer = &response_;
    else if (body.contains<http::peer_value>())
        body.get<http::peer_value>().buffer = &response_;
}

// The buffer is idle here, so its retention is reduced to the minimum.
void socket::reclaim_buffer() NOEXCEPT
{
    if (response_.capacity() > minimum_buffer_)
    {
        response_.shrink_to_fit();
        response_.reserve(minimum_buffer_);
    }
}

void socket::body_write(http::response&& response,
    count_handler&& handler) NOEXCEPT
{
    body_send<body_serializer>(std::move(response), std::move(handler));
}

void socket::body_notify(http::request&& notification,
    count_handler&& handler) NOEXCEPT
{
    body_send<notify_serializer>(std::move(notification), std::move(handler));
}

// private
template <class Serializer>
void socket::body_send(typename Serializer::message_type&& message,
    count_handler&& handler) NOEXCEPT
{
    assign_buffer(message.body());
    const auto out = emplace_shared<serialize_state<Serializer>>(
        std::move(message));

    count_handler complete = std::bind(&socket::handle_http_write,
        shared_from_this(), _1, _2, std::move(handler));

    io::dispatch(strand_,
        std::bind(&socket::do_serialize<Serializer>,
            shared_from_this(), out, zero, std::move(complete)));
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
