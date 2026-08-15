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
#include <bitcoin/network/privacy/cipher.hpp>
#include <bitcoin/network/privacy/context.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace privacy {

/// bip324 (v2) transport stream over a tcp socket.
///
/// Presents the v1 (heading framed) wire image to the caller while carrying
/// v2 encrypted packets on the wire, so socket framing above is unchanged:
/// written v1 messages are translated to encrypted packets, and received
/// packets are surfaced as whole messages. On accept, a v1 peer is
/// detected from its first bytes and the stream degrades to passthrough.
///
/// Models asio AsyncReadStream/AsyncWriteStream (handlers must be copyable).
/// All calls must be sequenced on the underlying socket's executor. Read and
/// write chains may overlap each other but not themselves (as asio streams).
/// Not final, asio trait detection requires derivability.
class BCT_API stream
{
public:
    DELETE_COPY_MOVE(stream);

    typedef std::function<void(const boost_code&)> handshake_handler;
    using io_handler =
        boost::asio::any_completion_handler<void(boost_code, size_t)>;
    using payload_t = std::span<const uint8_t>;
    using message_handler = boost::asio::any_completion_handler<
        void(boost_code, uint8_t, std::string, const payload_t&)>;
    using executor_type = asio::socket::executor_type;

    /// Assume ownership of the connected tcp socket.
    stream(asio::socket&& socket, const context& context) NOEXCEPT;

    /// asio stream conventions (next_layer enables get_lowest_layer).
    executor_type get_executor() NOEXCEPT;
    asio::socket& next_layer() NOEXCEPT;
    const asio::socket& next_layer() const NOEXCEPT;

    /// Perform the v2 handshake, set initiate if this side connected.
    /// A v1 peer detected on accept completes success with passthrough set.
    void async_handshake(bool initiate, handshake_handler&& handler) NOEXCEPT;

    /// The stream is encrypting, false if the accepted peer was detected v1.
    bool encrypted() const NOEXCEPT;

    /// The session identifier (valid after v2 handshake).
    const system::hash_digest& session_id() const NOEXCEPT;

    /// Read the next message into the buffer, decrypted in place (v2).
    /// Identity is the short identifier, or the command if it is zero.
    /// The payload is a span over the buffer (excludes framing and tag).
    void async_read_message(system::data_chunk& buffer,
        message_handler&& handler) NOEXCEPT;

    /// Read some bytes of the surfaced v1 stream.
    template <typename MutableBuffers, typename Handler>
    void async_read_some(const MutableBuffers& buffers, Handler&& handler)
    {
        if (passthrough_ && replay_.empty())
        {
            socket_.async_read_some(buffers, std::forward<Handler>(handler));
            return;
        }

        read_some(
            [buffers](const uint8_t* data, size_t size) NOEXCEPT
            {
                return boost::asio::buffer_copy(buffers,
                    boost::asio::const_buffer{ data, size });
            },
            boost::asio::buffer_size(buffers),
            io_handler{ std::forward<Handler>(handler) });
    }

    /// Write a message as one encrypted packet (v2 only).
    /// Identity is the short identifier, or the command if it is zero.
    void async_write_message(uint8_t identifier, const std::string& command,
        const system::chunk_cptr& payload, io_handler&& handler) NOEXCEPT;

    /// Write some bytes of the v1 stream (passthrough only).
    template <typename ConstBuffers, typename Handler>
    void async_write_some(const ConstBuffers& buffers, Handler&& handler)
    {
        if (passthrough_)
        {
            socket_.async_write_some(buffers, std::forward<Handler>(handler));
            return;
        }

        // The encrypted stream accepts messages, not bytes.
        boost::asio::post(get_executor(),
            [handler = std::forward<Handler>(handler)]() mutable
            {
                handler(boost::asio::error::no_protocol_option, zero);
            });
    }

private:
    typedef std::function<size_t(const uint8_t*, size_t)> copy_handler;
    using pump_handler =
        boost::asio::any_completion_handler<void(boost_code)>;

    // handshake
    void do_initiate(const handshake_handler& handler) NOEXCEPT;
    void do_respond(const handshake_handler& handler) NOEXCEPT;
    void handle_detect(const boost_code& ec,
        const handshake_handler& handler) NOEXCEPT;
    void handle_their_key(const boost_code& ec, bool initiate,
        const handshake_handler& handler) NOEXCEPT;
    void send_terminator_version(bool initiate,
        const handshake_handler& handler) NOEXCEPT;
    void scan_terminator(const handshake_handler& handler) NOEXCEPT;
    void read_versioning(bool first, const handshake_handler& handler) NOEXCEPT;

    // packet pump (read)
    void read_some(const copy_handler& copy, size_t limit,
        io_handler&& handler) NOEXCEPT;
    void read_exactly(const std::span<uint8_t>& out,
        pump_handler&& handler) NOEXCEPT;
    static bool split(uint8_t& identifier, std::string& command,
        size_t& prefix, const std::span<const uint8_t>& contents) NOEXCEPT;


    // These are protected by stream (executor) sequencing.
    asio::socket socket_;
    cipher cipher_;
    const uint32_t identifier_;
    bool passthrough_{};

    // v1 detection replay (passthrough) or handshake residue (v2).
    system::data_chunk replay_{};

    // read state
    system::data_chunk garbage_{};
    system::data_chunk packet_{};


};

} // namespace privacy
} // namespace network
} // namespace libbitcoin

#endif
