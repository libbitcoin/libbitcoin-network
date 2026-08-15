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
#include <bitcoin/network/privacy/commands.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace privacy {

using namespace system;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// bip324
// Errors are surfaced as generic boost codes, the socket logs and maps them.
// Handshake and stream failures are unrecoverable, producing socket close.
constexpr auto protocol_error = boost::asio::error::no_protocol_option;

stream::stream(asio::socket&& socket, const context& context) NOEXCEPT
  : socket_(std::move(socket)), cipher_{}, identifier_(context.identifier)
{
}

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

bool stream::encrypted() const NOEXCEPT
{
    return !passthrough_;
}

const hash_digest& stream::session_id() const NOEXCEPT
{
    return cipher_.session_id();
}

// Handshake.
// ----------------------------------------------------------------------------

void stream::async_handshake(bool initiate,
    handshake_handler&& handler) NOEXCEPT
{
    if (initiate)
        do_initiate(std::move(handler));
    else
        do_respond(std::move(handler));
}

void stream::do_initiate(const handshake_handler& handler) NOEXCEPT
{
    // bip324
    // The initiator sends its ellswift public key (with optional garbage).
    const auto& key = cipher_.public_key();
    boost::asio::async_write(socket_,
        boost::asio::const_buffer{ key.data(), key.size() },
        [this, handler](const boost_code& ec, size_t)
        {
            if (ec) { handler(ec); return; }

            // bip324
            // The responder replies with its own ellswift public key.
            replay_.resize(cipher::key_size);
            boost::asio::async_read(socket_,
                boost::asio::mutable_buffer{ replay_.data(), replay_.size() },
                std::bind(&stream::handle_their_key, this, _1, true, handler));
        });
}

void stream::do_respond(const handshake_handler& handler) NOEXCEPT
{
    // bip324
    // The responder matches initial bytes against the v1 version message
    // prefix (network magic and "version" command padding, 16 bytes).
    replay_.resize(messages::peer::heading::command_size + sizeof(uint32_t));
    boost::asio::async_read(socket_,
        boost::asio::mutable_buffer{ replay_.data(), replay_.size() },
        std::bind(&stream::handle_detect, this, _1, handler));
}

void stream::handle_detect(const boost_code& ec,
    const handshake_handler& handler) NOEXCEPT
{
    if (ec) { handler(ec); return; }

    const auto prefix = build_chunk(
    {
        to_little_endian(identifier_),
        to_chunk("version"),
        data_chunk(5, 0x00)
    });

    if (replay_ == prefix)
    {
        // v1 peer detected, become transparent and replay the prefix.
        passthrough_ = true;
        handler(boost_code{});
        return;
    }

    // v2 peer, the balance of the ellswift public key follows.
    const auto detected = replay_.size();
    replay_.resize(cipher::key_size);
    boost::asio::async_read(socket_, boost::asio::mutable_buffer
        { std::next(replay_.data(), detected), replay_.size() - detected },
        std::bind(&stream::handle_their_key, this, _1, false, handler));
}

void stream::handle_their_key(const boost_code& ec, bool initiate,
    const handshake_handler& handler) NOEXCEPT
{
    if (ec) { handler(ec); return; }

    cipher::key theirs{};
    std::copy(replay_.begin(), replay_.end(), theirs.begin());
    replay_.clear();

    if (!cipher_.initialize(theirs, identifier_, initiate))
    {
        handler(protocol_error);
        return;
    }

    send_terminator_version(initiate, handler);
}

void stream::send_terminator_version(bool initiate,
    const handshake_handler& handler) NOEXCEPT
{
    // bip324
    // Send garbage terminator and version packet (aad is sent garbage, none).
    // The responder additionally prepends its own ellswift public key, as the
    // initiator sent its key before reading the peer key.
    const auto& terminator = cipher_.send_terminator();
    data_chunk packet(cipher::expansion);
    cipher_.encrypt({}, {}, false, packet);

    const auto frame = std::make_shared<data_chunk>();
    frame->reserve(cipher::key_size + terminator.size() + packet.size());

    if (!initiate)
        extend(*frame, cipher_.public_key());

    extend(*frame, terminator);
    extend(*frame, packet);

    boost::asio::async_write(socket_,
        boost::asio::const_buffer{ frame->data(), frame->size() },
        [this, frame, handler](const boost_code& code, size_t)
        {
            if (code) { handler(code); return; }
            scan_terminator(handler);
        });
}

void stream::scan_terminator(const handshake_handler& handler) NOEXCEPT
{
    // bip324
    // Scan for the peer garbage terminator within garbage plus terminator.
    const auto& terminator = cipher_.receive_terminator();
    const auto position = std::search(garbage_.begin(), garbage_.end(),
        terminator.begin(), terminator.end());

    if (position != garbage_.end())
    {
        // Bytes beyond the terminator are packet stream residue.
        const auto end = std::next(position, terminator.size());
        replay_.assign(end, garbage_.end());
        garbage_.erase(position, garbage_.end());
        read_versioning(true, handler);
        return;
    }

    if (garbage_.size() >= cipher::maximum_garbage + cipher::terminator_size)
    {
        handler(protocol_error);
        return;
    }

    // Read more bytes (at least one, up to the remaining scan allowance).
    const auto start = garbage_.size();
    const auto allowance = cipher::maximum_garbage +
        cipher::terminator_size - start;
    garbage_.resize(start + allowance);
    socket_.async_read_some(boost::asio::mutable_buffer
        { std::next(garbage_.data(), start), allowance },
        [this, start, handler](const boost_code& ec, size_t size)
        {
            garbage_.resize(start + (ec ? zero : size));
            if (ec) { handler(ec); return; }
            scan_terminator(handler);
        });
}

void stream::read_versioning(bool first, const handshake_handler& handler) NOEXCEPT
{
    // bip324
    // Read packets until the version packet (skipping decoys). The aad of the
    // first received packet is the peer garbage (excluding terminator).
    packet_.resize(cipher::length_size);
    read_exactly(packet_,
        [this, first, handler](const boost_code& ec)
        {
            if (ec) { handler(ec); return; }

            const auto length = cipher_.decrypt_length(packet_);
            if (length > cipher::maximum_content)
            {
                handler(protocol_error);
                return;
            }

            packet_.resize(cipher::header_size + length + cipher::tag_size);
            read_exactly(packet_,
                [this, first, length, handler](const boost_code& code)
                {
                    if (code) { handler(code); return; }

                    bool ignore{};
                    const std::span<uint8_t> packet{ packet_ };
                    const auto plain = packet.first(
                        cipher::header_size + length);
                    const auto aad = first ? garbage_ : data_chunk{};
                    if (!cipher_.decrypt(plain, aad, ignore, packet))
                    {
                        handler(protocol_error);
                        return;
                    }

                    // Decoy packets are discarded, contents are ignored.
                    if (ignore)
                    {
                        read_versioning(false, handler);
                        return;
                    }

                    // Handshake is complete.
                    garbage_.clear();
                    garbage_.shrink_to_fit();
                    handler(boost_code{});
                });
        });
}

// Read pump.
// ----------------------------------------------------------------------------

void stream::read_some(const copy_handler& copy, size_t limit,
    io_handler&& handler) NOEXCEPT
{
    // Passthrough replay of the v1 detection prefix.
    if (passthrough_)
    {
        const auto size = std::min(limit, replay_.size());
        copy(replay_.data(), size);
        replay_.erase(replay_.begin(), std::next(replay_.begin(), size));
        boost::asio::post(get_executor(),
            [handler = std::move(handler), size]() mutable
            {
                std::move(handler)(boost_code{}, size);
            });
        return;
    }

    if (is_zero(limit))
    {
        boost::asio::post(get_executor(),
            [handler = std::move(handler)]() mutable
            {
                std::move(handler)(boost_code{}, zero);
            });
        return;
    }

    // The encrypted stream surfaces messages, not bytes.
    boost::asio::post(get_executor(),
        [handler = std::move(handler)]() mutable
        {
            std::move(handler)(protocol_error, zero);
        });
}

// Read exactly out size bytes into out, drawing from replay residue first.
void stream::read_exactly(const std::span<uint8_t>& out,
    pump_handler&& handler) NOEXCEPT
{
    const auto size = out.size();
    const auto residue = std::min(size, replay_.size());
    std::copy_n(replay_.begin(), residue, out.begin());
    replay_.erase(replay_.begin(), std::next(replay_.begin(), residue));

    if (residue == size)
    {
        boost::asio::post(get_executor(),
            [handler = std::move(handler)]() mutable
            {
                std::move(handler)(boost_code{});
            });
        return;
    }

    boost::asio::async_read(socket_, boost::asio::mutable_buffer
        { std::next(out.data(), residue), size - residue },
        [handler = std::move(handler)](const boost_code& ec, size_t) mutable
        {
            std::move(handler)(ec);
        });
}

// Read the encrypted length prefix and then the packet into the caller
// buffer, decrypting it in place (skipping decoys). The payload is a span
// over the buffer following the packet header and message type prefix.
void stream::async_read_message(data_chunk& buffer,
    message_handler&& handler) NOEXCEPT
{
    packet_.resize(cipher::length_size);
    read_exactly(packet_,
        [this, &buffer, handler = std::move(handler)](
            const boost_code& ec) mutable
        {
            if (ec)
            {
                std::move(handler)(ec, uint8_t{}, std::string{}, payload_t{});
                return;
            }

            const auto length = cipher_.decrypt_length(packet_);
            if (length > cipher::maximum_content)
            {
                std::move(handler)(protocol_error, uint8_t{}, std::string{}, payload_t{});
                return;
            }

            buffer.resize(cipher::header_size + length + cipher::tag_size);
            read_exactly(buffer,
                [this, &buffer, length, handler = std::move(handler)](
                    const boost_code& code) mutable
                {
                    if (code)
                    {
                        std::move(handler)(code, uint8_t{}, std::string{}, payload_t{});
                        return;
                    }

                    bool ignore{};
                    const std::span<uint8_t> packet{ buffer };
                    const auto plain = packet.first(
                        cipher::header_size + length);

                    if (!cipher_.decrypt(plain, {}, ignore, packet))
                    {
                        std::move(handler)(protocol_error, uint8_t{}, std::string{}, payload_t{});
                        return;
                    }

                    // Decoy packets are discarded, contents are ignored.
                    if (ignore)
                    {
                        async_read_message(buffer, std::move(handler));
                        return;
                    }

                    size_t prefix{};
                    uint8_t identifier{};
                    std::string command{};
                    const auto contents = plain.subspan(cipher::header_size);

                    if (!split(identifier, command, prefix, contents))
                    {
                        std::move(handler)(protocol_error, uint8_t{}, std::string{}, payload_t{});
                        return;
                    }

                    std::move(handler)(boost_code{}, identifier,
                        std::move(command), contents.subspan(prefix));
                });
        });
}

// static
bool stream::split(uint8_t& identifier, std::string& command, size_t& prefix,
    const std::span<const uint8_t>& contents) NOEXCEPT
{
    using namespace messages::peer;

    if (contents.empty())
        return false;

    identifier = contents.front();

    // bip324
    // A nonzero first byte is a short message type identifier, otherwise it
    // introduces a 12 byte (padded ascii) message type.
    if (!is_zero(identifier))
    {
        prefix = one;
        return true;
    }

    prefix = add1(heading::command_size);
    if (contents.size() < prefix)
        return false;

    const auto begin = std::next(contents.begin());
    const auto end = std::next(begin, heading::command_size);
    const auto terminus = std::find(begin, end, 0x00);
    command.assign(begin, terminus);

    // Trailing pad bytes must be null (deserialize symmetry).
    return std::all_of(terminus, end, [](uint8_t byte) NOEXCEPT
        { return is_zero(byte); });
}

// Write path.
// ----------------------------------------------------------------------------

void stream::write_pending(size_t size, io_handler&& handler) NOEXCEPT
{
    if (!encrypt_frames())
    {
        boost::asio::post(get_executor(),
            [handler = std::move(handler)]() mutable
            {
                std::move(handler)(protocol_error, zero);
            });
        return;
    }

    if (ciphertext_.empty())
    {
        // Nothing complete to send yet, input is buffered.
        boost::asio::post(get_executor(),
            [handler = std::move(handler), size]() mutable
            {
                std::move(handler)(boost_code{}, size);
            });
        return;
    }

    // Move ciphertext into a stable buffer for the async write.
    const auto out = std::make_shared<data_chunk>(std::move(ciphertext_));
    ciphertext_ = {};

    boost::asio::async_write(socket_,
        boost::asio::const_buffer{ out->data(), out->size() },
        [out, size, handler = std::move(handler)](
            const boost_code& ec, size_t) mutable
        {
            std::move(handler)(ec, ec ? zero : size);
        });
}

// Encrypt all complete v1 messages in pending_ into ciphertext_.
bool stream::encrypt_frames() NOEXCEPT
{
    using namespace messages::peer;

    while (pending_.size() >= heading::size())
    {
        system::stream::in::fast source{ pending_ };
        system::read::bytes::fast reader{ source };
        const auto head = heading::deserialize(reader);
        if (!reader || head.magic != identifier_)
            return false;

        const auto frame = ceilinged_add(heading::size(), head.payload_size);
        if (head.payload_size > cipher::maximum_content)
            return false;

        if (pending_.size() < frame)
            break;

        // bip324
        // Contents are the short type identifier (or zero byte and padded
        // command) followed by the payload (magic/checksum are dropped).
        const auto id = to_identifier(head.command);
        data_chunk contents{};
        contents.reserve(add1(head.payload_size));

        if (is_zero(id))
        {
            contents.push_back(0x00);
            contents.resize(add1(heading::command_size), 0x00);
            std::copy_n(head.command.begin(), head.command.size(),
                std::next(contents.begin()));
        }
        else
        {
            contents.push_back(id);
        }

        contents.insert(contents.end(),
            std::next(pending_.begin(), heading::size()),
            std::next(pending_.begin(), frame));

        // Append the encrypted packet to the ciphertext buffer.
        const auto start = ciphertext_.size();
        ciphertext_.resize(start + contents.size() + cipher::expansion);
        cipher_.encrypt(contents, {}, false, std::span<uint8_t>
            { std::next(ciphertext_.data(), start),
                contents.size() + cipher::expansion });

        pending_.erase(pending_.begin(), std::next(pending_.begin(), frame));
    }

    return true;
}

BC_POP_WARNING()

} // namespace privacy
} // namespace network
} // namespace libbitcoin
