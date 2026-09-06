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
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

// ZMTP messages are read into and written from rpc messages by socket role.
// ----------------------------------------------------------------------------
// The stream frames the wire only. A message (one or more frames chained by
// MORE) maps to an rpc request or response: the first part is the method (or
// topic) and each subsequent part is one positional param. A command maps to
// an rpc request named by the command in lower case, with the command fields
// as params. The role (zmtp::role) closes both directions: it selects which
// messages may be read and which rpc messages may be written, and it defines
// the routing envelope, where the rpc id is the peer identity (router).
//
// Param values: an inbound part is a byte chunk (system::chunk_cptr in any_t),
// as the wire carries no type. An outbound value encodes by its alternative:
// a string or chunk is its bytes, null is an empty part, a bool is one byte,
// and a fixed-width integral is little-endian of its own width. A double, an
// array, an object and json have no wire form (zmtp_unserializable).
//
// Control commands PING and PONG are read in every role as the "ping" and
// "pong" methods (ttl and context, context) and written from the same, and
// are answered by the proxy (the channel is keepalive-blind).

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)

using namespace system;
using namespace std::placeholders;
using stream = zmtp::stream;
using role = zmtp::role;

// Command name literals (self-describing, length-prefixed on the wire).
constexpr auto command_ping = "PING";
constexpr auto command_pong = "PONG";
constexpr auto command_subscribe = "SUBSCRIBE";
constexpr auto command_cancel = "CANCEL";

// Method name literals (the rpc form of commands).
constexpr auto method_ping = "ping";
constexpr auto method_pong = "pong";
constexpr auto method_subscribe = "subscribe";

// The PING TTL is a 16-bit big-endian (network order) field.
constexpr auto ttl_size = sizeof(uint16_t);

// Decoding (frames to rpc).
// ----------------------------------------------------------------------------

// An inbound part is delivered as a shared byte chunk.
static rpc::value_t to_chunk_value(const std::span<const uint8_t>& bytes) NOEXCEPT
{
    return rpc::any_t{ to_shared(data_chunk{ bytes.begin(), bytes.end() }) };
}

// The method (or topic) part is text.
static rpc::string_t to_method(const data_chunk& part) NOEXCEPT
{
    return { part.begin(), part.end() };
}

// A message is a method part followed by one part per positional param.
static code decode_request(rpc::request_t& out, const socket::parts_t& parts,
    size_t start) NOEXCEPT
{
    if (parts.size() <= start || parts.at(start).body.empty())
        return error::zmtp_unexpected_message;

    rpc::array_t params{};
    for (auto index = add1(start); index < parts.size(); ++index)
        params.push_back(to_chunk_value(parts.at(index).body));

    out.method = to_method(parts.at(start).body);
    out.params = std::move(params);
    return error::success;
}

// A subscription is a topic prefix and a stop (cancel) flag.
static void decode_subscription(rpc::request_t& out,
    const std::span<const uint8_t>& prefix, bool stop) NOEXCEPT
{
    out.method = method_subscribe;
    out.params = rpc::array_t{ to_chunk_value(prefix), rpc::value_t{ stop } };
}

// A command is a whole message, its name selecting the rpc method.
static code decode_command(rpc::request_t& out, role role,
    const stream::frame& command) NOEXCEPT
{
    std::string name{};
    std::span<const uint8_t> content{};
    const std::span<const uint8_t> body{ command.body };
    if (!stream::command_name(name, content, body))
        return error::zmtp_unexpected_command;

    if (name == command_ping)
    {
        if (content.size() < ttl_size)
            return error::zmtp_unexpected_command;

        data_array<ttl_size> ttl{};
        std::copy_n(content.begin(), ttl_size, ttl.begin());
        out.method = method_ping;
        out.params = rpc::array_t
        {
            rpc::value_t{ from_big_endian<uint16_t>(ttl) },
            to_chunk_value(content.subspan(ttl_size))
        };

        return error::success;
    }

    if (name == command_pong)
    {
        out.method = method_pong;
        out.params = rpc::array_t{ to_chunk_value(content) };
        return error::success;
    }

    if (role == role::publisher && name == command_subscribe)
    {
        decode_subscription(out, content, false);
        return error::success;
    }

    if (role == role::publisher && name == command_cancel)
    {
        decode_subscription(out, content, true);
        return error::success;
    }

    return error::zmtp_unexpected_command;
}

// A message (non-command) is read by role.
static code decode_message(rpc::request_t& out, role role,
    const socket::parts_t& parts) NOEXCEPT
{
    switch (role)
    {
        case role::publisher:
        {
            // The 3.0 dialect: a single frame message prefixed 0x01
            // (subscribe) or 0x00 (cancel), accepted from any peer (libzmq).
            const auto& body = parts.front().body;
            if (!is_one(parts.size()) || body.empty() || body.front() > 0x01)
                return error::zmtp_unexpected_message;

            const std::span<const uint8_t> prefix{ body };
            decode_subscription(out, prefix.subspan(one), is_zero(body.front()));
            return error::success;
        }
        case role::puller:
        {
            return decode_request(out, parts, zero);
        }
        case role::replier:
        {
            // REQ prefixes the request with an empty delimiter part. The
            // request expects a response, correlated by strict alternation,
            // so it carries a (null) id.
            if (!parts.front().body.empty())
                return error::zmtp_unexpected_message;

            out.id = rpc::identity_t{ rpc::null_t{} };
            return decode_request(out, parts, one);
        }
        case role::router:
        {
            // The peer identity precedes the request, then the empty
            // delimiter of a REQ peer (a DEALER peer omits it).
            const auto& identity = parts.front().body;
            if (identity.empty() || parts.size() < two)
                return error::zmtp_unexpected_message;

            const auto delimited = parts.at(one).body.empty();
            out.id = rpc::string_t{ identity.begin(), identity.end() };
            return decode_request(out, parts, delimited ? two : one);
        }
        default:
        {
            return error::bad_stream;
        }
    }
}

// Encoding (rpc to frames).
// ----------------------------------------------------------------------------

// A command frame: length-prefixed name then content.
static data_chunk make_command(const std::string& name,
    const std::span<const uint8_t>& content) NOEXCEPT
{
    data_chunk body{};
    body.push_back(possible_narrow_cast<uint8_t>(name.size()));
    body.insert(body.end(), name.begin(), name.end());
    body.insert(body.end(), content.begin(), content.end());
    return stream::frame_encode(body, true, false);
}

// One outbound param value is one part, encoded by its alternative.
static code encode_value(data_chunk& out, const rpc::value_t& value) NOEXCEPT
{
    const auto integral = [&](auto number) NOEXCEPT
    {
        const auto bytes = to_little_endian(number);
        out.assign(bytes.begin(), bytes.end());
        return error::success;
    };

    return std::visit(overload
    {
        [&](rpc::null_t) NOEXCEPT
        {
            out.clear();
            return error::success;
        },
        [&](rpc::boolean_t flag) NOEXCEPT
        {
            out.assign({ flag ? 0x01_u8 : 0x00_u8 });
            return error::success;
        },
        [&](const rpc::string_t& text) NOEXCEPT
        {
            out.assign(text.begin(), text.end());
            return error::success;
        },
        [&](const rpc::any_t& any) NOEXCEPT
        {
            const auto chunk = any.get<const data_chunk>();
            if (!chunk)
                return error::zmtp_unserializable;

            out = *chunk;
            return error::success;
        },
        [&](int8_t number) NOEXCEPT { return integral(number); },
        [&](int16_t number) NOEXCEPT { return integral(number); },
        [&](int32_t number) NOEXCEPT { return integral(number); },
        [&](int64_t number) NOEXCEPT { return integral(number); },
        [&](uint8_t number) NOEXCEPT { return integral(number); },
        [&](uint16_t number) NOEXCEPT { return integral(number); },
        [&](uint32_t number) NOEXCEPT { return integral(number); },
        [&](uint64_t number) NOEXCEPT { return integral(number); },
        [&](const auto&) NOEXCEPT
        {
            // number_t, array_t, object_t and json_t have no wire form.
            return error::zmtp_unserializable;
        }
    }, value.value());
}

// Positional params are one part each, a single value is one part.
static code encode_params(data_stack& parts,
    const rpc::params_option& params) NOEXCEPT
{
    if (!params)
        return error::success;

    return std::visit(overload
    {
        [&](const rpc::value_t& value) NOEXCEPT
        {
            return encode_value(parts.emplace_back(), value);
        },
        [&](const rpc::array_t& values) NOEXCEPT
        {
            for (const auto& value: values)
                if (const auto ec = encode_value(parts.emplace_back(), value))
                    return ec;

            return code{ error::success };
        },
        [&](const rpc::object_t&) NOEXCEPT
        {
            // Named params have no wire order.
            return code{ error::zmtp_unserializable };
        }
    }, *params);
}

// The router envelope is the peer identity (rpc id) and a delimiter.
static code encode_identity(data_stack& parts,
    const rpc::id_option& id) NOEXCEPT
{
    if (!id || !std::holds_alternative<rpc::string_t>(*id))
        return error::zmtp_unserializable;

    const auto& identity = std::get<rpc::string_t>(*id);
    parts.emplace_back(identity.begin(), identity.end());
    parts.emplace_back();
    return error::success;
}

// A notification is a message (or a control command) written by role.
static code encode_notification(data_chunk& packet, role role,
    const rpc::request_t& notification) NOEXCEPT
{
    // Control commands are written in every role.
    if (notification.method == method_pong ||
        notification.method == method_ping)
    {
        data_stack parts{};
        if (const auto ec = encode_params(parts, notification.params))
            return ec;

        if (notification.method == method_pong)
        {
            if (!is_one(parts.size()))
                return error::zmtp_unserializable;

            packet = stream::make_pong(parts.front());
            return error::success;
        }

        // PING content is the TTL (big-endian) then the context.
        if (parts.size() != two || parts.front().size() != ttl_size)
            return error::zmtp_unserializable;

        std::reverse(parts.front().begin(), parts.front().end());
        auto content = parts.front();
        content.insert(content.end(), parts.back().begin(), parts.back().end());
        packet = make_command(command_ping, content);
        return error::success;
    }

    data_stack parts{};
    switch (role)
    {
        case role::publisher:
        {
            break;
        }
        case role::router:
        {
            if (const auto ec = encode_identity(parts, notification.id))
                return ec;

            break;
        }
        default:
        {
            // A puller writes nothing and a replier writes only responses.
            return error::zmtp_unserializable;
        }
    }

    parts.push_back(to_chunk(notification.method));
    if (const auto ec = encode_params(parts, notification.params))
        return ec;

    packet = stream::frame_message(parts);
    return error::success;
}

// A response is a delimited result (one part) or error (code and message).
static code encode_response(data_chunk& packet, role role,
    const rpc::response_t& response) NOEXCEPT
{
    data_stack parts{};
    switch (role)
    {
        case role::replier:
        {
            parts.emplace_back();
            break;
        }
        case role::router:
        {
            if (const auto ec = encode_identity(parts, response.id))
                return ec;

            break;
        }
        default:
        {
            // A publisher and a puller have no reply path.
            return error::zmtp_unserializable;
        }
    }

    if (response.error)
    {
        const auto& error = *response.error;
        const auto ec = encode_value(parts.emplace_back(), error.code);
        if (ec)
            return ec;

        parts.push_back(to_chunk(error.message));
    }
    else
    {
        const auto& result = response.result.value_or(rpc::value_t{});
        if (const auto ec = encode_value(parts.emplace_back(), result))
            return ec;
    }

    packet = stream::frame_message(parts);
    return error::success;
}

// ZMTP (read).
// ----------------------------------------------------------------------------

// private
void socket::do_zmtp_read(const zmtp_read_state::ptr& in,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (!zeromq() || role_ == role::undefined)
    {
        handler(error::bad_stream, zero);
        return;
    }

    if (in->parts.size() >= stream::maximum_parts)
    {
        handler(error::zmtp_excessive_parts, in->total);
        return;
    }

    // Each part is read into the parts vector, chained by the MORE flag.
    get_zmtp().async_read_frame(in->parts.emplace_back(),
        std::bind(&socket::handle_zmtp_read,
            shared_from_this(), _1, _2, in, handler));
}

// private
void socket::handle_zmtp_read(const boost_code& ec, size_t size,
    const zmtp_read_state::ptr& in, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec)
    {
        handle_async(ec, in->total, handler, "async_read_frame");
        return;
    }

    in->total += size;
    const auto& frame = in->parts.back();

    // A command is a whole message, and cannot be within a multipart message.
    if (frame.command())
    {
        if (!is_one(in->parts.size()) || frame.more())
        {
            handler(error::zmtp_unexpected_command, in->total);
            return;
        }

        handler(decode_command(in->out.message, role_, frame), in->total);
        return;
    }

    if (frame.more())
    {
        do_zmtp_read(in, handler);
        return;
    }

    handler(decode_message(in->out.message, role_, in->parts), in->total);
}

// ZMTP (write).
// ----------------------------------------------------------------------------
// The packet is fully framed and allocated before write, as tcp_write.

// private
void socket::do_zmtp_notify(const rpc::request_ptr& out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (!zeromq())
    {
        handler(error::bad_stream, zero);
        return;
    }

    const auto packet = to_shared<data_chunk>();
    if (const auto ec = encode_notification(*packet, role_, out->message))
    {
        handler(ec, zero);
        return;
    }

    do_zmtp_write(packet, handler);
}

// private
void socket::do_zmtp_response(const rpc::response_ptr& out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (!zeromq())
    {
        handler(error::bad_stream, zero);
        return;
    }

    const auto packet = to_shared<data_chunk>();
    if (const auto ec = encode_response(*packet, role_, out->message))
    {
        handler(ec, zero);
        return;
    }

    do_zmtp_write(packet, handler);
}

// private
// The packet is bound to preserve its buffer for the write.
void socket::do_zmtp_write(const chunk_ptr& packet,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    get_zmtp().async_write({ packet->data(), packet->size() },
        [self = shared_from_this(), packet, handler](const boost_code& ec,
            size_t size) NOEXCEPT
        {
            self->handle_async(ec, size, handler, "async_write");
        });
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
