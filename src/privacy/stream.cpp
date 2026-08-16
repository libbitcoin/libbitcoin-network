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
#include <bitcoin/network/privacy/stream.hpp>

#include <algorithm>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace privacy {

using namespace system;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

constexpr auto protocol_error = boost::asio::error::no_protocol_option;

// static
bool stream::detected_v1(const std::span<const uint8_t>& prefix,
    uint32_t identifier) NOEXCEPT
{
    // The v1 version message prefix (network magic and command padding).
    const auto expected = build_chunk(
    {
        to_little_endian(identifier),
        to_chunk("version"),
        data_chunk(5, 0x00)
    });

    return prefix.size() == expected.size() &&
        std::equal(prefix.begin(), prefix.end(), expected.begin());
}

// Constructor.
// ----------------------------------------------------------------------------

stream::stream(asio::socket&& socket, const context& context) NOEXCEPT
  : socket_(std::move(socket)), identifier_(context.identifier)
{
}

// Properies.
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

const hash_digest& stream::session_id() const NOEXCEPT
{
    return cipher_.session_id();
}

// Handshake.
// ----------------------------------------------------------------------------

void stream::async_handshake(handshake_handler&& handler) NOEXCEPT
{
    do_initiate(std::move(handler));
}

void stream::async_handshake(data_chunk&& detected,
    handshake_handler&& handler) NOEXCEPT
{
    do_respond(std::move(detected), std::move(handler));
}

void stream::do_initiate(const handshake_handler& handler) NOEXCEPT
{
    // The initiator sends its ellswift public key (with optional garbage).
    const auto& key = cipher_.public_key();
    const boost::asio::const_buffer out{ key.data(), key.size() };

    boost::asio::async_write(socket_, out,
        std::bind(&stream::handle_key_sent,
            this, _1, handler));
}

void stream::handle_key_sent(const boost_code& ec,
    const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    // The responder replies with its own ellswift public key.
    const auto peer = std::make_shared<cipher::key>();
    const boost::asio::mutable_buffer in{ peer->data(), peer->size() };

    boost::asio::async_read(socket_, in,
        std::bind(&stream::handle_their_key,
            this, _1, true, peer, handler));
}

void stream::do_respond(data_chunk&& detected,
    const handshake_handler& handler) NOEXCEPT
{
    // Detected v2 from initial bytes, balance of ellswift public key follows.
    const auto keyed = detected.size();
    const auto peer = std::make_shared<cipher::key>();
    std::copy(detected.begin(), detected.end(), peer->begin());
    const auto begin = std::next(peer->data(), keyed);
    const boost::asio::mutable_buffer in{ begin, peer->size() - keyed };

    boost::asio::async_read(socket_, in,
        std::bind(&stream::handle_their_key,
            this, _1, false, peer, handler));
}

void stream::handle_their_key(const boost_code& ec, bool initiate,
    const key_ptr& peer, const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    if (!cipher_.initialize(*peer, identifier_, initiate))
    {
        handler(protocol_error);
        return;
    }

    send_terminator_version(initiate, handler);
}

void stream::send_terminator_version(bool initiate,
    const handshake_handler& handler) NOEXCEPT
{
    // Send garbage terminator and version packet.
    const auto& terminator = cipher_.send_terminator();
    data_chunk packet(cipher::expansion);
    cipher_.encrypt({}, {}, false, packet);
    const auto frame = to_shared<data_chunk>();
    frame->reserve(cipher::key_size + terminator.size() + packet.size());

    if (!initiate) extend(*frame, cipher_.public_key());
    extend(*frame, terminator);
    extend(*frame, packet);

    const boost::asio::const_buffer out{ frame->data(), frame->size() };
    boost::asio::async_write(socket_, out,
        std::bind(&stream::handle_terminator_sent,
            this, _1, frame, handler));
}

// The frame is bound to preserve its buffer for the write.
void stream::handle_terminator_sent(const boost_code& ec, const chunk_ptr&,
    const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    scan_terminator(handler);
}

void stream::scan_terminator(const handshake_handler& handler) NOEXCEPT
{
    // Scan for the peer garbage terminator within garbage plus terminator.
    const auto& terminator = cipher_.receive_terminator();
    const auto position = std::search(garbage_.begin(), garbage_.end(),
        terminator.begin(), terminator.end());

    if (position != garbage_.end())
    {
        // Bytes beyond the terminator are packet stream residue.
        const auto end = std::next(position, terminator.size());
        residue_.assign(end, garbage_.end());
        garbage_.erase(position, garbage_.end());
        read_versioning(true, handler);
        return;
    }

    constexpr auto garbage = cipher::maximum_garbage + cipher::terminator_size;
    if (garbage_.size() >= garbage)
    {
        handler(protocol_error);
        return;
    }

    // Read more bytes (at least one, up to the remaining scan allowance).
    const auto start = garbage_.size();
    const auto allowance = garbage - start;
    garbage_.resize(start + allowance);
    const auto begin = std::next(garbage_.data(), start);
    const boost::asio::mutable_buffer in{ begin, allowance };

    socket_.async_read_some(in,
        std::bind(&stream::handle_scan,
            this, _1, _2, start, handler));
}

void stream::handle_scan(const boost_code& ec, size_t size, size_t start,
    const handshake_handler& handler) NOEXCEPT
{
    garbage_.resize(start + (ec ? zero : size));

    if (ec)
    {
        handler(ec);
        return;
    }

    scan_terminator(handler);
}

void stream::read_versioning(bool first, const handshake_handler& handler) NOEXCEPT
{
    // Read packets until the version packet (skipping decoys). 
    // The aad of first received packet is peer garbage (excluding terminator).
    packet_.resize(cipher::length_size);
    read_exactly(packet_,
        std::bind(&stream::handle_version_length,
            this, _1, first, handler));
}

void stream::handle_version_length(const boost_code& ec, bool first,
    const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    const auto length = cipher_.decrypt_length(packet_);
    if (length > cipher::maximum_content)
    {
        handler(protocol_error);
        return;
    }

    packet_.resize(cipher::header_size + length + cipher::tag_size);
    read_exactly(packet_,
        std::bind(&stream::handle_version_packet,
            this, _1, first, length, handler));
}

void stream::handle_version_packet(const boost_code& ec, bool first,
    size_t length, const handshake_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    bool ignore{};
    const std::span<uint8_t> packet{ packet_ };
    const auto plain = packet.first(cipher::header_size + length);
    const auto aad = first ? garbage_ : data_chunk{};

    if (!cipher_.decrypt(plain, aad, ignore, packet))
    {
        handler(protocol_error);
        return;
    }

    // Decoy packets discarded, contents ignored.
    if (ignore)
    {
        read_versioning(false, handler);
        return;
    }

    // Handshake is complete.
    garbage_.clear();
    garbage_.shrink_to_fit();
    handler(boost_code{});
}

// Read pump.
// ----------------------------------------------------------------------------

// Read exactly out size bytes into out, drawing from replay residue first.
void stream::read_exactly(const std::span<uint8_t>& out,
    pump_handler&& handler) NOEXCEPT
{
    const auto size = out.size();
    const auto residue = std::min(size, residue_.size());
    std::copy_n(residue_.begin(), residue, out.begin());
    residue_.erase(residue_.begin(), std::next(residue_.begin(), residue));

    if (residue == size)
    {
        boost::asio::post(get_executor(),
            std::bind(&stream::handle_read,
                this, boost_code{}, size, std::move(handler)));
        return;
    }

    const auto begin = std::next(out.data(), residue);
    const boost::asio::mutable_buffer in{ begin, size - residue };

    boost::asio::async_read(socket_, in,
        std::bind(&stream::handle_read,
            this, _1, _2, std::move(handler)));
}

// The read size is implied by the fixed buffer (dropped).
void stream::handle_read(const boost_code& ec, size_t,
    const pump_handler& handler) NOEXCEPT
{
    handler(ec);
}

// Read the encrypted length prefix and then the packet into the caller
// buffer, decrypting it in place (skipping decoys). The payload is a span
// over the buffer following the packet header and message type prefix.
void stream::async_read_message(data_chunk& buffer, size_t maximum,
    message_handler&& handler) NOEXCEPT
{
    packet_.resize(cipher::length_size);

    read_exactly(packet_,
        std::bind(&stream::handle_message_length,
            this, _1, std::ref(buffer), maximum, std::move(handler)));
}

void stream::handle_message_length(const boost_code& ec, data_chunk& buffer,
    size_t maximum, const message_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec, {}, {}, {});
        return;
    }

    using namespace messages::peer;
    const auto max = maximum + add1(heading::command_size);
    const auto bound = std::min(cipher::maximum_content, max);
    const auto length = cipher_.decrypt_length(packet_);
    if (length > bound)
    {
        handler(protocol_error, {}, {}, {});
        return;
    }

    buffer.resize(cipher::header_size + length + cipher::tag_size);

    read_exactly(buffer,
        std::bind(&stream::handle_message_read,
            this, _1, std::ref(buffer), length, maximum, handler));
}

void stream::handle_message_read(const boost_code& ec, data_chunk& buffer,
    size_t length, size_t maximum, const message_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec, uint8_t{}, std::string{}, payload_t{});
        return;
    }

    bool ignore{};
    const std::span<uint8_t> packet{ buffer };
    const auto plain = packet.first(cipher::header_size + length);

    if (!cipher_.decrypt(plain, {}, ignore, packet))
    {
        handler(protocol_error, {}, {}, {});
        return;
    }

    // Decoy packets are discarded, contents are ignored.
    if (ignore)
    {
        async_read_message(buffer, maximum, move_copy(handler));
        return;
    }

    size_t prefix{};
    uint8_t identifier{};
    std::string command{};
    const auto contents = plain.subspan(cipher::header_size);

    if (!split(identifier, command, prefix, contents))
    {
        handler(protocol_error, {}, {}, {});
        return;
    }

    handler({}, identifier, std::move(command), contents.subspan(prefix));
}

// static
bool stream::split(uint8_t& identifier, std::string& command, size_t& prefix,
    const std::span<const uint8_t>& contents) NOEXCEPT
{
    if (contents.empty())
        return false;

    // A zero first byte implies a 12 byte (padded ascii) message type.
    identifier = contents.front();
    if (!is_zero(identifier))
    {
        prefix = one;
        return true;
    }

    using namespace messages::peer;
    prefix = add1(heading::command_size);
    if (contents.size() < prefix)
        return false;

    const auto begin = std::next(contents.begin());
    const auto end = std::next(begin, heading::command_size);
    const auto terminus = std::find(begin, end, 0x00);
    command.assign(begin, terminus);

    // Trailing pad bytes must be null (deserialize symmetry).
    return std::all_of(terminus, end, [](uint8_t byte) NOEXCEPT
    {
        return is_zero(byte);
    });
}

// Write path.
// ----------------------------------------------------------------------------

void stream::async_write_message(uint8_t identifier,
    const std::string& command, const chunk_cptr& payload,
    io_handler&& handler) NOEXCEPT
{
    using namespace messages::peer;
    const auto prefix = is_zero(identifier) ? add1(heading::command_size) : one;

    if (!payload || command.size() > heading::command_size ||
        payload->size() > cipher::maximum_content - prefix)
    {
        boost::asio::post(get_executor(),
            std::bind(&stream::handle_message_sent,
                this, protocol_error, size_t{}, chunk_ptr{}, std::move(handler)));
        return;
    }

    data_chunk contents{};
    contents.reserve(prefix + payload->size());
    contents.push_back(identifier);

    if (is_zero(identifier))
    {
        contents.resize(prefix, 0x00);
        std::copy_n(command.begin(), command.size(),
            std::next(contents.begin()));
    }

    contents.insert(contents.end(), payload->begin(), payload->end());
    const auto bytes = contents.size() + cipher::expansion;
    const auto packet = emplace_shared<data_chunk>(bytes);
    cipher_.encrypt(contents, {}, false, *packet);
    const boost::asio::const_buffer out{ packet->data(), packet->size() };

    boost::asio::async_write(socket_, out,
        std::bind(&stream::handle_message_sent,
            this, _1, _2, packet, std::move(handler)));
}

// The packet is bound to preserve its buffer for the write.
void stream::handle_message_sent(const boost_code& ec, size_t size,
    const chunk_ptr&, const io_handler& handler) NOEXCEPT
{
    handler(ec, size);
}

BC_POP_WARNING()

} // namespace privacy
} // namespace network
} // namespace libbitcoin
