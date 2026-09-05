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
#include <bitcoin/network/zmtp/cipher.hpp>

namespace libbitcoin {
namespace network {
namespace zmtp {

using namespace system;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

constexpr auto protocol_error = boost::asio::error::no_protocol_option;

// The mechanism name occupies the 20-byte mechanism field.
constexpr auto mechanism_size = 20_size;
constexpr auto mechanism_null = "NULL";
constexpr auto mechanism_curve = "CURVE";

// Command name literals (self-describing, length-prefixed on the wire).
constexpr auto command_ready = "READY";
constexpr auto command_error = "ERROR";
constexpr auto command_pong = "PONG";
constexpr auto command_hello = "HELLO";
constexpr auto command_initiate = "INITIATE";
constexpr auto command_message = "MESSAGE";

// Constructor.
// ----------------------------------------------------------------------------

// The context selects the upgrade and configures the CURVE mechanism.
stream::stream(asio::socket&& socket, const context& context) NOEXCEPT
  : socket_(std::move(socket))
{
    if (context.curve())
        cipher_.emplace(context.secret(), context.public_key());
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

data_chunk stream::make_greeting(bool as_server, bool curve) NOEXCEPT
{
    data_chunk greeting(greeting_size, 0x00);

    // Signature: 0xff, eight pad bytes, 0x7f (low bit set gates validity).
    greeting.front() = 0xff;
    greeting.at(signature_size - 1u) = 0x7f;

    // Version: major.minor (revision 3.1).
    greeting.at(signature_size) = revision_major;
    greeting.at(add1(signature_size)) = revision_minor;

    // Mechanism: name left-justified in a 20-byte null-padded field.
    const auto mechanism = curve ? mechanism_curve : mechanism_null;
    const auto position = std::next(greeting.begin(), signature_size + 2u);
    std::copy_n(mechanism, std::char_traits<char>::length(mechanism),
        position);

    // As-server byte follows the mechanism (NULL has no role asymmetry),
    // and the 31-byte filler remains zero.
    greeting.at(signature_size + 2u + mechanism_size) =
        (curve && as_server) ? 0x01 : 0x00;

    return greeting;
}

bool stream::parse_greeting(const std::span<const uint8_t>& greeting,
    uint8_t& minor, bool& curve, bool& as_server) NOEXCEPT
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

    // Mechanism must byte-match a name in a 20-byte null-padded field.
    const auto position = std::next(greeting.begin(), signature_size + 2u);
    const auto matches = [&](const char* name) NOEXCEPT
    {
        const auto length = std::char_traits<char>::length(name);
        return std::equal(name, std::next(name, length), position) &&
            std::all_of(std::next(position, length),
                std::next(position, mechanism_size),
                    [](uint8_t byte) NOEXCEPT { return is_zero(byte); });
    };

    if (matches(mechanism_null))
        curve = false;
    else if (matches(mechanism_curve))
        curve = true;
    else
        return false;

    as_server = !is_zero(greeting[signature_size + 2u + mechanism_size]);
    return true;
}

data_chunk stream::make_property(const std::string& name,
    const std::string& value) NOEXCEPT
{
    // Property: name (u8-len) then value (u32be-len).
    data_chunk property{};
    property.push_back(possible_narrow_cast<uint8_t>(name.size()));
    property.insert(property.end(), name.begin(), name.end());
    const auto size = to_big_endian(possible_narrow_cast<uint32_t>(
        value.size()));
    property.insert(property.end(), size.begin(), size.end());
    property.insert(property.end(), value.begin(), value.end());
    return property;
}

data_chunk stream::make_ready_pub() NOEXCEPT
{
    // READY command body: length-prefixed name then metadata properties.
    data_chunk body{};
    const std::string name{ command_ready };
    body.push_back(possible_narrow_cast<uint8_t>(name.size()));
    body.insert(body.end(), name.begin(), name.end());
    const auto property = make_property("Socket-Type", "PUB");
    body.insert(body.end(), property.begin(), property.end());
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

bool stream::frame_decode(uint8_t& flags, std::span<const uint8_t>& body,
    std::span<const uint8_t>& buffer) NOEXCEPT
{
    if (buffer.empty())
        return false;

    flags = buffer.front();
    buffer = buffer.subspan(sizeof(flags));

    size_t length{};
    if (!is_zero(flags & flag_long))
    {
        if (buffer.size() < sizeof(uint64_t))
            return false;

        data_array<sizeof(uint64_t)> bytes{};
        std::copy_n(buffer.begin(), bytes.size(), bytes.begin());
        length = possible_narrow_cast<size_t>(from_big_endian<uint64_t>(
            bytes));
        buffer = buffer.subspan(bytes.size());
    }
    else
    {
        if (buffer.empty())
            return false;

        length = buffer.front();
        buffer = buffer.subspan(sizeof(uint8_t));
    }

    if (buffer.size() < length)
        return false;

    body = buffer.first(length);
    buffer = buffer.subspan(length);
    return true;
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
    const auto greeting = to_shared(make_greeting(as_server,
        cipher_.has_value()));
    const boost::asio::const_buffer out{ greeting->data(), greeting->size() };
    boost::asio::async_write(socket_, out,
        std::bind(&stream::handle_greeting_sent,
            this, _1, as_server, greeting, handler));
}

void stream::handle_greeting_sent(const boost_code& ec, bool as_server,
    const chunk_cptr&, const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    read_greeting(as_server, handler);
}

void stream::read_greeting(bool as_server,
    const handshake_handler& handler) NOEXCEPT
{
    // Read the peer greeting progressively: signature and major first.
    greeting_.resize(greeting_size);
    const boost::asio::mutable_buffer in{ greeting_.data(), greeting_stage1 };
    boost::asio::async_read(socket_, in,
        std::bind(&stream::handle_greeting_stage1,
            this, _1, as_server, handler));
}

void stream::handle_greeting_stage1(const boost_code& ec, bool as_server,
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
            this, _1, as_server, handler));
}

void stream::handle_greeting_stage2(const boost_code& ec, bool as_server,
    const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    uint8_t minor{};
    bool curve{};
    bool peer_server{};
    const std::span<const uint8_t> greeting{ greeting_ };
    if (!parse_greeting(greeting, minor, curve, peer_server))
    {
        handler(protocol_error);
        return;
    }

    greeting_.clear();
    greeting_.shrink_to_fit();

    // The peer mechanism must match ours, and under CURVE the peer must take
    // the role opposite to ours (only the server role is implemented).
    if (curve != cipher_.has_value() || (curve && (peer_server || !as_server)))
    {
        fail_handshake("mechanism mismatch", handler);
        return;
    }

    if (curve)
        read_hello(handler);
    else
        write_ready(handler);
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

// The peer socket type must be a subscriber (SUB or XSUB).
bool stream::subscriber(const std::span<const uint8_t>& metadata) const NOEXCEPT
{
    std::string type{};
    return ready_socket_type(type, metadata) &&
        (type == "SUB" || type == "XSUB");
}

// NULL mechanism.
// ----------------------------------------------------------------------------

void stream::write_ready(const handshake_handler& handler) NOEXCEPT
{
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

    if (!subscriber(metadata))
    {
        fail_handshake("incompatible socket type", handler);
        return;
    }

    // Handshake complete; the tier above reads from here.
    handler(boost_code{});
}

// CURVE mechanism (server).
// ----------------------------------------------------------------------------

void stream::read_hello(const handshake_handler& handler) NOEXCEPT
{
    // The frame is bound to outlive the read.
    const auto hello = to_shared<frame>();
    async_read_frame(*hello,
        std::bind(&stream::handle_hello,
            this, _1, _2, hello, handler));
}

void stream::handle_hello(const boost_code& ec, size_t,
    const frame_ptr& hello, const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    // The first post-greeting frame must be a HELLO command.
    std::string name{};
    std::span<const uint8_t> content{};
    const std::span<const uint8_t> body{ hello->body };
    if (!hello->command() || !command_name(name, content, body) ||
        name != command_hello)
    {
        fail_handshake("expected HELLO", handler);
        return;
    }

    // The WELCOME carries the server transient key and cookie.
    data_chunk welcome{};
    if (!cipher_->welcome(welcome, body))
    {
        fail_handshake("invalid HELLO", handler);
        return;
    }

    const auto frame = to_shared(frame_encode(welcome, true, false));
    const boost::asio::const_buffer out{ frame->data(), frame->size() };
    boost::asio::async_write(socket_, out,
        std::bind(&stream::handle_welcome_sent,
            this, _1, frame, handler));
}

void stream::handle_welcome_sent(const boost_code& ec, const chunk_cptr&,
    const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    // The frame is bound to outlive the read.
    const auto initiate = to_shared<frame>();
    async_read_frame(*initiate,
        std::bind(&stream::handle_initiate,
            this, _1, _2, initiate, handler));
}

void stream::handle_initiate(const boost_code& ec, size_t,
    const frame_ptr& initiate, const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    std::string name{};
    std::span<const uint8_t> content{};
    const std::span<const uint8_t> body{ initiate->body };
    if (!initiate->command() || !command_name(name, content, body) ||
        name != command_initiate)
    {
        fail_handshake("expected INITIATE", handler);
        return;
    }

    // The READY carries our metadata, the INITIATE the peer's.
    data_chunk ready{};
    data_chunk metadata{};
    if (!cipher_->ready(ready, metadata, body,
        make_property("Socket-Type", "PUB")))
    {
        fail_handshake("invalid INITIATE", handler);
        return;
    }

    if (!subscriber(metadata))
    {
        fail_handshake("incompatible socket type", handler);
        return;
    }

    const auto frame = to_shared(frame_encode(ready, true, false));
    const boost::asio::const_buffer out{ frame->data(), frame->size() };
    boost::asio::async_write(socket_, out,
        std::bind(&stream::handle_curve_ready_sent,
            this, _1, frame, handler));
}

void stream::handle_curve_ready_sent(const boost_code& ec, const chunk_cptr&,
    const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    // Handshake complete; frames are boxed from here.
    secured_ = true;
    handler(boost_code{});
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
        handle_frame_body(boost_code{}, zero, out, handler);
        return;
    }

    const boost::asio::mutable_buffer in{ body.data(), length };
    boost::asio::async_read(socket_, in,
        std::bind(&stream::handle_frame_body,
            this, _1, _2, out, handler));
}

void stream::handle_frame_body(const boost_code& ec, size_t size,
    ref<frame> out, const io_handler& handler) NOEXCEPT
{
    if (ec || !secured_)
    {
        handler(ec, size);
        return;
    }

    // Under CURVE every frame is a command: a boxed MESSAGE, or an ERROR
    // (delivered as is, the tier above treats it as any other command).
    auto& frame = out.get();
    std::string name{};
    std::span<const uint8_t> content{};
    const std::span<const uint8_t> body{ frame.body };
    if (!frame.command() || !command_name(name, content, body))
    {
        handler(protocol_error, zero);
        return;
    }

    if (name == command_error)
    {
        handler(ec, size);
        return;
    }

    uint8_t payload{};
    data_chunk plain{};
    if (name != command_message || !cipher_->decode(payload, plain, body))
    {
        handler(protocol_error, zero);
        return;
    }

    // The payload flags map to frame flags (LONG is a framing artifact).
    frame.body.swap(plain);
    frame.flags = 0x00;
    if (!is_zero(payload & cipher::payload_more))
        frame.flags |= flag_more;

    if (!is_zero(payload & cipher::payload_command))
        frame.flags |= flag_command;

    handler(ec, frame.body.size());
}

// Buffer write (caller-framed, caller-retained until the handler fires).
// ----------------------------------------------------------------------------

void stream::async_write(const asio::const_buffer& in,
    io_handler&& handler) NOEXCEPT
{
    if (!secured_)
    {
        boost::asio::async_write(socket_, in, std::move(handler));
        return;
    }

    // Under CURVE each frame is boxed into a MESSAGE command for this peer,
    // into the stream's write scratch (one write in flight at a time).
    boxed_.clear();
    uint8_t flags{};
    std::span<const uint8_t> body{};
    std::span<const uint8_t> buffer
    {
        pointer_cast<const uint8_t>(in.data()), in.size()
    };

    while (!buffer.empty())
    {
        data_chunk message{};
        uint8_t payload{};
        if (!frame_decode(flags, body, buffer))
        {
            handler(protocol_error, zero);
            return;
        }

        if (!is_zero(flags & flag_more))
            payload |= cipher::payload_more;

        if (!is_zero(flags & flag_command))
            payload |= cipher::payload_command;

        if (!cipher_->encode(message, payload, body))
        {
            handler(protocol_error, zero);
            return;
        }

        const auto frame = frame_encode(message, true, false);
        boxed_.insert(boxed_.end(), frame.begin(), frame.end());
    }

    const boost::asio::const_buffer out{ boxed_.data(), boxed_.size() };
    boost::asio::async_write(socket_, out, std::move(handler));
}

BC_POP_WARNING()

} // namespace zmtp
} // namespace network
} // namespace libbitcoin
