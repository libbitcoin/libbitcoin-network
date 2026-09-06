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
#ifndef LIBBITCOIN_NETWORK_ZMTP_STREAM_HPP
#define LIBBITCOIN_NETWORK_ZMTP_STREAM_HPP

#include <optional>
#include <span>
#include <bitcoin/network/asio.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/zmtp/cipher.hpp>
#include <bitcoin/network/zmtp/context.hpp>
#include <bitcoin/network/zmtp/role.hpp>

namespace libbitcoin {
namespace network {
namespace zmtp {

/// Native ZMTP (ZeroMQ 3.x) framing over a tcp socket.
/// The stream performs the greeting and mechanism handshake (NULL, or CURVE
/// as server when the context holds a keypair) and then frames the wire: one
/// frame is read at a time into a caller-owned frame, and a caller-framed
/// buffer is written at a time. Under CURVE each frame is boxed into a
/// MESSAGE command on write and unboxed on read, so the tiers above see the
/// same frames either way. It is a transport, not a framework. The role is
/// the socket type advertised in the handshake, which admits only compatible
/// peers; message assembly and its mapping to rpc belong to the socket, and
/// control traffic (PING/PONG) and write serialization to the tier above,
/// which uses its own write queue; the stream buffers only the in-flight
/// write.
/// All calls must be sequenced on the underlying socket's executor. Read and
/// write chains may overlap each other but not themselves (as asio streams).
class BCT_API stream
{
public:
    DELETE_COPY_MOVE(stream);

    typedef std::function<void(const boost_code&)> handshake_handler;
    typedef std::function<void(const boost_code&, size_t)> io_handler;
    using executor_type = asio::socket::executor_type;

    /// One wire frame: the flags byte and the body (caller-owned on read).
    struct frame
    {
        uint8_t flags{};
        system::data_chunk body{};

        /// The frame is a command (not a message part).
        inline bool command() const NOEXCEPT
        {
            return !is_zero(flags & flag_command);
        }

        /// A message part follows this one (multipart continuation).
        inline bool more() const NOEXCEPT
        {
            return !is_zero(flags & flag_more);
        }
    };

    /// ZMTP framing flag bits (frame flags byte).
    static constexpr uint8_t flag_more = 0x01;
    static constexpr uint8_t flag_long = 0x02;
    static constexpr uint8_t flag_command = 0x04;

    /// The ZMTP greeting is a fixed 64 bytes.
    static constexpr size_t greeting_size = 64;

    /// Signature (first ten bytes of the greeting) gates peer validity.
    static constexpr size_t signature_size = 10;

    /// The greeting is read in two stages: signature plus version-major, then
    /// the balance (version-minor, mechanism, as-server, filler).
    static constexpr size_t greeting_stage1 = signature_size + 1u;
    static constexpr size_t greeting_stage2 = greeting_size - greeting_stage1;

    /// The revision (major version) this implementation advertises and floors.
    static constexpr uint8_t revision_major = 3;
    static constexpr uint8_t revision_minor = 1;

    /// Maximum accepted inbound frame contents. Inbound traffic is control
    /// (subscriptions, pings) and rpc requests, so this is modest.
    static constexpr size_t maximum_inbound = 8192;

    /// Maximum echoed PING context (per ZMTP, truncated to this).
    static constexpr size_t maximum_ping_context = 16;

    /// Maximum parts of one inbound multipart message.
    static constexpr size_t maximum_parts = 32;

    /// Assume ownership of the connected tcp socket, in the given role.
    stream(asio::socket&& socket, const context& context,
        zmtp::role role) NOEXCEPT;

    /// asio stream conventions (next_layer enables get_lowest_layer).
    executor_type get_executor() NOEXCEPT;
    asio::socket& next_layer() NOEXCEPT;
    const asio::socket& next_layer() const NOEXCEPT;

    /// Perform the ZMTP greeting and mechanism handshake.
    /// as_server is set for accepted (bound) connections, clear for connected.
    /// The CURVE mechanism is implemented for the server role only.
    void async_handshake(bool as_server, handshake_handler&& handler) NOEXCEPT;

    /// Read the next frame into the caller-owned frame (flags and body), one
    /// frame per read (the handshake reads one command at a time).
    void async_read_frame(frame& out, io_handler&& handler) NOEXCEPT;

    /// Decode the frame at the front of a buffer into flags and an owned body
    /// (unboxed under CURVE), setting the buffer to the remainder. Returns
    /// need_more if the frame is incomplete (the buffer is unchanged),
    /// oversized_payload if its length exceeds the limit, and a protocol
    /// violation if it is malformed or its box does not open.
    code decode(uint8_t& flags, system::data_chunk& body,
        std::span<const uint8_t>& buffer, size_t limit) NOEXCEPT;

    /// Write a caller-framed buffer (see frame_message). The caller retains
    /// the buffer until the handler fires, when the peer has accepted it.
    void async_write(const asio::const_buffer& in,
        io_handler&& handler) NOEXCEPT;

    // Codec (static, stateless). Exposed for the tier above and for test.
    // ------------------------------------------------------------------------

    /// Frame one whole multipart message (v3.1 rules, MORE on all but the
    /// last part) into a single contiguous buffer. Frame once, write to all.
    static system::data_chunk frame_message(
        const system::data_stack& parts) NOEXCEPT;

    /// Build our complete 64-byte greeting (sent eagerly on handshake start).
    /// The as-server byte is set only for the CURVE mechanism server role.
    static system::data_chunk make_greeting(bool as_server,
        bool curve) NOEXCEPT;

    /// Validate a peer greeting and extract its minor version, mechanism
    /// (NULL or CURVE) and as-server byte. Requires the full greeting_size
    /// bytes. Returns false on signature/mechanism refusal.
    static bool parse_greeting(const std::span<const uint8_t>& greeting,
        uint8_t& minor, bool& curve, bool& as_server) NOEXCEPT;

    /// Build one metadata property (name u8-length, value u32be-length).
    static system::data_chunk make_property(const std::string& name,
        const std::string& value) NOEXCEPT;

    /// Build the NULL-mechanism READY command advertising the role's
    /// Socket-Type.
    static system::data_chunk make_ready(zmtp::role role) NOEXCEPT;

    /// Build an ERROR command with the given reason.
    static system::data_chunk make_error(const std::string& reason) NOEXCEPT;

    /// Build a PONG command echoing the given context (truncated to 16 bytes).
    static system::data_chunk make_pong(
        const std::span<const uint8_t>& context) NOEXCEPT;

    /// Encode a single frame (flags + length + body) using the v3.1 rules.
    static system::data_chunk frame_encode(const std::span<const uint8_t>& body,
        bool command, bool more) NOEXCEPT;

    /// Decode the frame at the front of a buffer into flags and body, setting
    /// the buffer to the remainder. False if the buffer is malformed.
    static bool frame_decode(uint8_t& flags, std::span<const uint8_t>& body,
        std::span<const uint8_t>& buffer) NOEXCEPT;

    /// Parse the frame header (flags and length) at the front of a buffer
    /// without consuming it, setting header to the size of the prefix. False
    /// if the buffer holds less than the prefix.
    static bool frame_header(uint8_t& flags, size_t& length, size_t& header,
        const std::span<const uint8_t>& buffer) NOEXCEPT;

    /// Extract the command name from a command frame body, or empty if the
    /// self-describing length prefix is malformed. Sets body to the remainder.
    static bool command_name(std::string& name, std::span<const uint8_t>& body,
        const std::span<const uint8_t>& frame) NOEXCEPT;

    /// Parse the Socket-Type property from metadata. Returns false if the
    /// property is absent or malformed; type is the property value.
    static bool ready_socket_type(std::string& type,
        const std::span<const uint8_t>& body) NOEXCEPT;

private:
    using frame_ptr = std::shared_ptr<frame>;

    // handshake
    void write_greeting(bool as_server,
        const handshake_handler& handler) NOEXCEPT;
    void handle_greeting_sent(const boost_code& ec, bool as_server,
        const system::chunk_cptr& greeting,
        const handshake_handler& handler) NOEXCEPT;
    void read_greeting(bool as_server,
        const handshake_handler& handler) NOEXCEPT;
    void handle_greeting_stage1(const boost_code& ec, bool as_server,
        const handshake_handler& handler) NOEXCEPT;
    void handle_greeting_stage2(const boost_code& ec, bool as_server,
        const handshake_handler& handler) NOEXCEPT;
    void fail_handshake(const std::string& reason,
        const handshake_handler& handler) NOEXCEPT;

    // NULL mechanism
    void write_ready(const handshake_handler& handler) NOEXCEPT;
    void handle_ready_sent(const boost_code& ec,
        const system::chunk_cptr& ready,
        const handshake_handler& handler) NOEXCEPT;
    void read_ready(const handshake_handler& handler) NOEXCEPT;
    void handle_ready(const boost_code& ec, size_t size,
        const frame_ptr& ready, const handshake_handler& handler) NOEXCEPT;

    // CURVE mechanism (server)
    void read_hello(const handshake_handler& handler) NOEXCEPT;
    void handle_hello(const boost_code& ec, size_t size,
        const frame_ptr& hello, const handshake_handler& handler) NOEXCEPT;
    void handle_welcome_sent(const boost_code& ec,
        const system::chunk_cptr& welcome,
        const handshake_handler& handler) NOEXCEPT;
    void handle_initiate(const boost_code& ec, size_t size,
        const frame_ptr& initiate, const handshake_handler& handler) NOEXCEPT;
    void handle_curve_ready_sent(const boost_code& ec,
        const system::chunk_cptr& ready,
        const handshake_handler& handler) NOEXCEPT;
    bool compatible(const std::span<const uint8_t>& metadata) const NOEXCEPT;

    // frame reader (one whole frame: flags, length, body)
    void handle_frame_flags(const boost_code& ec, ref<frame> out,
        const io_handler& handler) NOEXCEPT;
    void handle_frame_length(const boost_code& ec, bool long_size,
        ref<frame> out, const io_handler& handler) NOEXCEPT;
    void handle_frame_body(const boost_code& ec, size_t size,
        ref<frame> out, const io_handler& handler) NOEXCEPT;

    // These are protected by stream (executor) sequencing.
    asio::socket socket_;
    const context& context_;
    const zmtp::role role_;
    std::optional<cipher> cipher_{};
    bool secured_{};

    // Frame reader scratch (one in-flight frame at a time on the read chain).
    system::data_chunk greeting_{};
    system::data_array<sizeof(uint64_t)> length_{};

    // Boxed writer scratch (one in-flight write at a time on the write chain).
    system::data_chunk boxed_{};
};

} // namespace zmtp
} // namespace network
} // namespace libbitcoin

#endif
