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
#ifndef LIBBITCOIN_NETWORK_PRIVACY_STREAM_HPP
#define LIBBITCOIN_NETWORK_PRIVACY_STREAM_HPP

#include <functional>
#include <memory>
#include <span>
#include <bitcoin/network/asio.hpp>
#include <bitcoin/network/messages/peer/heading.hpp>
#include <bitcoin/network/privacy/cipher.hpp>
#include <bitcoin/network/privacy/context.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace privacy {

/// bip324 (v2) transport stream over a tcp socket.
/// The stream is message oriented: whole messages are written as encrypted
/// packets and received packets are surfaced as whole messages. The socket
/// upgrades to the stream only once the peer is detected (or connected) v2.
/// All calls must be sequenced on the underlying socket's executor. Read and
/// write chains may overlap each other but not themselves (as asio streams).
class BCT_API stream
{
public:
    DELETE_COPY_MOVE(stream);

    typedef std::function<void(const boost_code&)> handshake_handler;
    typedef std::function<void(const boost_code&, size_t)> io_handler;
    using payload_t = std::span<const uint8_t>;
    typedef std::function<void(const boost_code&, uint8_t,
        const std::string&, const payload_t&)> message_handler;
    using executor_type = asio::socket::executor_type;

    /// The size of the v1 detection prefix (magic and command padding).
    static constexpr size_t detection_size =
        sizeof(uint32_t) + messages::peer::heading::command_size;

    /// The detection prefix indicates a v1 peer (v1 version message).
    static bool detected_v1(const std::span<const uint8_t>& prefix,
        uint32_t identifier) NOEXCEPT;

    /// Assume ownership of the connected tcp socket (initiator).
    stream(asio::socket&& socket, const context& context) NOEXCEPT;

    /// Assume ownership of the accepted tcp socket (responder), with the
    /// detection prefix bytes consumed by the socket (partial peer key).
    stream(asio::socket&& socket, const context& context,
        system::data_chunk&& detected) NOEXCEPT;

    /// asio stream conventions (next_layer enables get_lowest_layer).
    executor_type get_executor() NOEXCEPT;
    asio::socket& next_layer() NOEXCEPT;
    const asio::socket& next_layer() const NOEXCEPT;

    /// Perform the v2 handshake, set initiate if this side connected.
    void async_handshake(bool initiate, handshake_handler&& handler) NOEXCEPT;

    /// The session identifier (valid after v2 handshake).
    const system::hash_digest& session_id() const NOEXCEPT;

    /// Read the next message into the buffer, decrypted in place (v2).
    /// A message with contents exceeding maximum is rejected unread.
    /// Identity is the short identifier, or the command if it is zero.
    /// The payload is a span over the buffer (excludes framing and tag).
    void async_read_message(system::data_chunk& buffer, size_t maximum,
        message_handler&& handler) NOEXCEPT;

    /// Write a message as one encrypted packet (v2 only).
    /// Identity is the short identifier, or the command if it is zero.
    void async_write_message(uint8_t identifier, const std::string& command,
        const system::chunk_cptr& payload, io_handler&& handler) NOEXCEPT;

private:
    typedef std::function<void(const boost_code&)> pump_handler;

    // handshake
    void do_initiate(const handshake_handler& handler) NOEXCEPT;
    void do_respond(const handshake_handler& handler) NOEXCEPT;
    void handle_key_sent(const boost_code& ec,
        const handshake_handler& handler) NOEXCEPT;
    void handle_their_key(const boost_code& ec, bool initiate,
        const handshake_handler& handler) NOEXCEPT;
    void send_terminator_version(bool initiate,
        const handshake_handler& handler) NOEXCEPT;
    void handle_terminator_sent(const boost_code& ec,
        const system::chunk_ptr& frame,
        const handshake_handler& handler) NOEXCEPT;
    void scan_terminator(const handshake_handler& handler) NOEXCEPT;
    void handle_scan(const boost_code& ec, size_t size, size_t start,
        const handshake_handler& handler) NOEXCEPT;
    void read_versioning(bool first, const handshake_handler& handler) NOEXCEPT;
    void handle_version_length(const boost_code& ec, bool first,
        const handshake_handler& handler) NOEXCEPT;
    void handle_version_packet(const boost_code& ec, bool first,
        size_t length, const handshake_handler& handler) NOEXCEPT;

    // packet pump (read)
    void read_exactly(const std::span<uint8_t>& out,
        pump_handler&& handler) NOEXCEPT;
    void handle_read(const boost_code& ec, size_t size,
        const pump_handler& handler) NOEXCEPT;
    void handle_message_length(const boost_code& ec,
        system::data_chunk& buffer, size_t maximum,
        const message_handler& handler) NOEXCEPT;
    void handle_message_read(const boost_code& ec,
        system::data_chunk& buffer, size_t length, size_t maximum,
        const message_handler& handler) NOEXCEPT;
    static bool split(uint8_t& identifier, std::string& command,
        size_t& prefix, const std::span<const uint8_t>& contents) NOEXCEPT;

    // packet write
    void handle_message_sent(const boost_code& ec, size_t size,
        const system::chunk_ptr& packet, const io_handler& handler) NOEXCEPT;


    // These are protected by stream (executor) sequencing.
    asio::socket socket_;
    cipher cipher_;
    const uint32_t identifier_;

    // Peer key accumulation and handshake packet stream residue.
    system::data_chunk replay_{};

    // read state
    system::data_chunk garbage_{};
    system::data_chunk packet_{};


};

} // namespace privacy
} // namespace network
} // namespace libbitcoin

#endif
