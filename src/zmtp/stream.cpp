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
#include <bitcoin/network/zmtp/stream.hpp>

#include <algorithm>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace zmtp {

using namespace system;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

constexpr auto protocol_error = boost::asio::error::no_protocol_option;

// The ZMTP NULL mechanism name occupies the 20-byte mechanism field.
constexpr auto mechanism_size = 20_size;

// Command name literals (self-describing, length-prefixed on the wire).
constexpr auto command_ready = "READY";
constexpr auto command_error = "ERROR";
constexpr auto command_pong = "PONG";

// Constructor.
// ----------------------------------------------------------------------------

// The context selects the upgrade; the NULL mechanism has no parameters.
stream::stream(asio::socket&& socket, const context&) NOEXCEPT
  : socket_(std::move(socket))
{
}

// Properties.
// ----------------------------------------------------------------------------

stream::executor_type stream::get_executor() NOEXCEPT
{
    return socket_.get_executor();
}

asio::socket& stream::next_layer() NOEXCEPT
{
    return socket_;
}

const asio::socket& stream::next_layer() const NOEXCEPT
{
    return socket_;
}

// Codec (static).
// ----------------------------------------------------------------------------

data_chunk stream::frame_message(const data_stack& parts) NOEXCEPT
{
    data_chunk packet{};
    if (parts.empty())
        return packet;

    const auto last = sub1(parts.size());
    for (size_t index{}; index < parts.size(); ++index)
    {
        const auto frame = frame_encode(parts.at(index), false, index < last);
        packet.insert(packet.end(), frame.begin(), frame.end());
    }

    return packet;
}

// The as-server greeting byte is 0 for the NULL mechanism (no role
// asymmetry); the parameter is retained for the CURVE mechanism (phase 2).
data_chunk stream::make_greeting(bool) NOEXCEPT
{
    data_chunk greeting(greeting_size, 0x00);

    // Signature: 0xff, eight pad bytes, 0x7f (low bit set gates validity).
    greeting.front() = 0xff;
    greeting.at(signature_size - 1u) = 0x7f;

    // Version: major.minor (revision 3.1).
    greeting.at(signature_size) = revision_major;
    greeting.at(add1(signature_size)) = revision_minor;

    // Mechanism: "NULL" left-justified in a 20-byte null-padded field.
    constexpr auto mechanism = "NULL";
    const auto position = std::next(greeting.begin(), signature_size + 2u);
    std::copy_n(mechanism, std::char_traits<char>::length(mechanism),
        position);

    // As-server byte and 31-byte filler remain zero.
    return greeting;
}

bool stream::parse_greeting(const std::span<const uint8_t>& greeting,
    uint8_t& minor) NOEXCEPT
{
    if (greeting.size() != greeting_size)
        return false;

    // Signature gates: first byte 0xff and low bit of the tenth byte set.
    if (greeting[0] != 0xff || is_zero(greeting[signature_size - 1u] & 0x01))
        return false;

    // Refuse revisions below 3.0 (no such peer exists in practice, as Core
    // requires libzmq >= 4.0). Higher revisions are treated as 3.1.
    const auto major = greeting[signature_size];
    if (major < revision_major)
        return false;

    minor = greeting[add1(signature_size)];

    // Mechanism must byte-match "NULL" in a 20-byte null-padded field.
    constexpr auto name = "NULL";
    const auto length = std::char_traits<char>::length(name);
    const auto position = std::next(greeting.begin(), signature_size + 2u);
    if (!std::equal(name, std::next(name, length), position))
        return false;

    // Trailing mechanism bytes must be null.
    return std::all_of(std::next(position, length),
        std::next(position, mechanism_size),
            [](uint8_t byte) NOEXCEPT { return is_zero(byte); });
}

data_chunk stream::make_ready_pub() NOEXCEPT
{
    // READY command body: length-prefixed name then metadata properties.
    data_chunk body{};
    const std::string name{ command_ready };
    body.push_back(possible_narrow_cast<uint8_t>(name.size()));
    body.insert(body.end(), name.begin(), name.end());

    // Property: "Socket-Type" => "PUB" (name u8-len, value u32be-len).
    const std::string key{ "Socket-Type" };
    const std::string value{ "PUB" };
    body.push_back(possible_narrow_cast<uint8_t>(key.size()));
    body.insert(body.end(), key.begin(), key.end());
    const auto size = to_big_endian(possible_narrow_cast<uint32_t>(
        value.size()));
    body.insert(body.end(), size.begin(), size.end());
    body.insert(body.end(), value.begin(), value.end());

    return frame_encode(body, true, false);
}

data_chunk stream::make_error(const std::string& reason) NOEXCEPT
{
    // ERROR command body: length-prefixed name then u8-length-prefixed reason.
    data_chunk body{};
    const std::string name{ command_error };
    body.push_back(possible_narrow_cast<uint8_t>(name.size()));
    body.insert(body.end(), name.begin(), name.end());
    body.push_back(possible_narrow_cast<uint8_t>(
        std::min(reason.size(), size_t{ 255 })));
    body.insert(body.end(), reason.begin(),
        std::next(reason.begin(), std::min(reason.size(), size_t{ 255 })));

    return frame_encode(body, true, false);
}

data_chunk stream::make_pong(const std::span<const uint8_t>& context) NOEXCEPT
{
    // PONG command body: length-prefixed name then echoed context (<=16), no
    // TTL (unlike PING).
    data_chunk body{};
    const std::string name{ command_pong };
    body.push_back(possible_narrow_cast<uint8_t>(name.size()));
    body.insert(body.end(), name.begin(), name.end());
    const auto echo = std::min(context.size(), maximum_ping_context);
    body.insert(body.end(), context.begin(),
        std::next(context.begin(), echo));

    return frame_encode(body, true, false);
}

data_chunk stream::frame_encode(const std::span<const uint8_t>& body,
    bool command, bool more) NOEXCEPT
{
    data_chunk frame{};
    uint8_t flags{};
    if (command) flags |= flag_command;
    if (more) flags |= flag_more;

    const auto size = body.size();
    if (size > 0xff)
    {
        // Long frame: LONG flag and an 8-byte big-endian length.
        flags |= flag_long;
        frame.push_back(flags);
        const auto length = to_big_endian(possible_wide_cast<uint64_t>(size));
        frame.insert(frame.end(), length.begin(), length.end());
    }
    else
    {
        // Short frame: a single length byte.
        frame.push_back(flags);
        frame.push_back(possible_narrow_cast<uint8_t>(size));
    }

    frame.insert(frame.end(), body.begin(), body.end());
    return frame;
}

bool stream::command_name(std::string& name, std::span<const uint8_t>& body,
    const std::span<const uint8_t>& frame) NOEXCEPT
{
    if (frame.empty())
        return false;

    const size_t length = frame.front();
    if (frame.size() < add1(length))
        return false;

    const auto begin = std::next(frame.begin());
    name.assign(begin, std::next(begin, length));
    body = frame.subspan(add1(length));
    return true;
}

bool stream::ready_socket_type(std::string& type,
    const std::span<const uint8_t>& body) NOEXCEPT
{
    // Scan metadata properties: name(u8-len) then value(u32be-len).
    auto data = body;
    while (!data.empty())
    {
        const size_t name_size = data.front();
        if (data.size() < add1(name_size))
            return false;

        const auto name_begin = std::next(data.begin());
        const std::string name{ name_begin, std::next(name_begin, name_size) };
        data = data.subspan(add1(name_size));

        constexpr auto length_size = sizeof(uint32_t);
        if (data.size() < length_size)
            return false;

        data_array<length_size> length_bytes{};
        std::copy_n(data.begin(), length_size, length_bytes.begin());
        const size_t value_size = from_big_endian<uint32_t>(length_bytes);
        data = data.subspan(length_size);
        if (data.size() < value_size)
            return false;

        // Case-sensitive property name match per ZMTP metadata convention.
        if (name == "Socket-Type")
        {
            type.assign(data.begin(), std::next(data.begin(), value_size));
            return true;
        }

        data = data.subspan(value_size);
    }

    return false;
}

// Handshake.
// ----------------------------------------------------------------------------

void stream::async_handshake(bool as_server,
    handshake_handler&& handler) NOEXCEPT
{
    write_greeting(as_server, handler);
}

void stream::write_greeting(bool as_server,
    const handshake_handler& handler) NOEXCEPT
{
    // Send our complete greeting eagerly (waiting to read first deadlocks).
    const auto greeting = to_shared(make_greeting(as_server));
    const boost::asio::const_buffer out{ greeting->data(), greeting->size() };
    boost::asio::async_write(socket_, out,
        std::bind(&stream::handle_greeting_sent,
            this, _1, as_server, greeting, handler));
}

void stream::handle_greeting_sent(const boost_code& ec, bool,
    const chunk_cptr&, const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    read_greeting(handler);
}

void stream::read_greeting(const handshake_handler& handler) NOEXCEPT
{
    // Read the peer greeting progressively: signature and major first.
    greeting_.resize(greeting_size);
    const boost::asio::mutable_buffer in{ greeting_.data(), greeting_stage1 };
    boost::asio::async_read(socket_, in,
        std::bind(&stream::handle_greeting_stage1,
            this, _1, handler));
}

void stream::handle_greeting_stage1(const boost_code& ec,
    const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    // Gate on signature and major before reading the balance.
    if (greeting_.front() != 0xff ||
        is_zero(greeting_.at(signature_size - 1u) & 0x01) ||
        greeting_.at(signature_size) < revision_major)
    {
        handler(protocol_error);
        return;
    }

    const auto begin = std::next(greeting_.data(), greeting_stage1);
    const boost::asio::mutable_buffer in{ begin, greeting_stage2 };
    boost::asio::async_read(socket_, in,
        std::bind(&stream::handle_greeting_stage2,
            this, _1, handler));
}

void stream::handle_greeting_stage2(const boost_code& ec,
    const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    uint8_t minor{};
    const std::span<const uint8_t> greeting{ greeting_ };
    if (!parse_greeting(greeting, minor))
    {
        handler(protocol_error);
        return;
    }

    greeting_.clear();
    greeting_.shrink_to_fit();
    write_ready(handler);
}

void stream::write_ready(const handshake_handler& handler) NOEXCEPT
{
    // Only the NULL mechanism is implemented (CURVE is phase 2).
    const auto ready = to_shared(make_ready_pub());
    const boost::asio::const_buffer out{ ready->data(), ready->size() };
    boost::asio::async_write(socket_, out,
        std::bind(&stream::handle_ready_sent,
            this, _1, ready, handler));
}

void stream::handle_ready_sent(const boost_code& ec, const chunk_cptr&,
    const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    read_ready(handler);
}

void stream::read_ready(const handshake_handler& handler) NOEXCEPT
{
    // The frame is bound to outlive the read.
    const auto ready = to_shared<frame>();
    async_read_frame(*ready,
        std::bind(&stream::handle_ready,
            this, _1, _2, ready, handler));
}

void stream::handle_ready(const boost_code& ec, size_t,
    const frame_ptr& ready, const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    // The first post-greeting frame must be a READY command.
    if (!ready->command())
    {
        fail_handshake("expected command", handler);
        return;
    }

    std::string name{};
    std::span<const uint8_t> metadata{};
    const std::span<const uint8_t> body{ ready->body };
    if (!command_name(name, metadata, body) || name != command_ready)
    {
        fail_handshake("expected READY", handler);
        return;
    }

    // Validate the peer socket type is a subscriber (SUB or XSUB).
    std::string type{};
    if (!ready_socket_type(type, metadata) || (type != "SUB" && type != "XSUB"))
    {
        fail_handshake("incompatible socket type", handler);
        return;
    }

    // Handshake complete; the tier above reads from here.
    handler(boost_code{});
}

void stream::fail_handshake(const std::string& reason,
    const handshake_handler& handler) NOEXCEPT
{
    // Best-effort ERROR command, then fail the handshake.
    const auto error = to_shared(make_error(reason));
    const boost::asio::const_buffer out{ error->data(), error->size() };
    boost::asio::async_write(socket_, out,
        [error, handler](const boost_code&, size_t) NOEXCEPT
        {
            handler(protocol_error);
        });
}

// Frame reader (one whole frame: flags, length, body).
// ----------------------------------------------------------------------------

void stream::async_read_frame(frame& out, io_handler&& handler) NOEXCEPT
{
    // Read the single flags byte into the caller's frame.
    const boost::asio::mutable_buffer in{ &out.flags, sizeof(out.flags) };
    boost::asio::async_read(socket_, in,
        std::bind(&stream::handle_frame_flags,
            this, _1, std::ref(out), std::move(handler)));
}

void stream::handle_frame_flags(const boost_code& ec, ref<frame> out,
    const io_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec, zero);
        return;
    }

    // LONG selects an 8-byte length, otherwise a single length byte.
    const auto long_size = !is_zero(out.get().flags & flag_long);
    const auto size = long_size ? sizeof(uint64_t) : sizeof(uint8_t);
    const boost::asio::mutable_buffer in{ length_.data(), size };
    boost::asio::async_read(socket_, in,
        std::bind(&stream::handle_frame_length,
            this, _1, long_size, out, handler));
}

void stream::handle_frame_length(const boost_code& ec, bool long_size,
    ref<frame> out, const io_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec, zero);
        return;
    }

    size_t length{};
    if (long_size)
    {
        length = possible_narrow_cast<size_t>(from_big_endian<uint64_t>(
            length_));
    }
    else
    {
        length = length_.front();
    }

    // Bound inbound frames; a publisher peer sends only small control traffic.
    if (length > maximum_inbound)
    {
        handler(protocol_error, zero);
        return;
    }

    auto& body = out.get().body;
    body.resize(length);
    if (is_zero(length))
    {
        handler(boost_code{}, zero);
        return;
    }

    const boost::asio::mutable_buffer in{ body.data(), length };
    boost::asio::async_read(socket_, in,
        std::bind(&stream::handle_frame_body,
            this, _1, _2, out, handler));
}

void stream::handle_frame_body(const boost_code& ec, size_t size, ref<frame>,
    const io_handler& handler) NOEXCEPT
{
    handler(ec, size);
}

// Buffer write (caller-framed, caller-retained until the handler fires).
// ----------------------------------------------------------------------------

void stream::async_write(const asio::const_buffer& in,
    io_handler&& handler) NOEXCEPT
{
    boost::asio::async_write(socket_, in, std::move(handler));
}

BC_POP_WARNING()

} // namespace zmtp
} // namespace network
} // namespace libbitcoin
