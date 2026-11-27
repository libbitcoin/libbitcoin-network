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
#ifndef LIBBITCOIN_NETWORK_NOISE_STREAM_HPP
#define LIBBITCOIN_NETWORK_NOISE_STREAM_HPP

#include <optional>
#include <vector>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/io/io.hpp>
#include <bitcoin/network/noise/cipher.hpp>
#include <bitcoin/network/noise/context.hpp>

namespace libbitcoin {
namespace network {
namespace noise {

/// Stratum v2 Noise responder or initiator connection over tcp socket.
/// Application bytes are sv2 frames: each whole written frame is sealed as one
/// transport frame, and each received transport frame is opened to the bytes
/// of its sv2 frame. Calls sequenced on socket executor; read and write chains
/// may overlap each other but not themselves.
class BCT_API stream
{
public:
    DELETE_COPY_MOVE(stream);

    typedef std::function<void(const code&)> result_handler;
    typedef std::function<void(const code&, size_t)> io_handler;
    using executor_type = io::socket::executor_type;

    /// Own connected tcp socket as responder, context outlives stream.
    stream(io::socket&& socket, const context& context) NOEXCEPT;

    /// Own connected tcp socket as initiator.
    stream(io::socket&& socket) NOEXCEPT;

    /// Stream executor and socket.
    executor_type get_executor() NOEXCEPT;
    io::socket& next_layer() NOEXCEPT;
    const io::socket& next_layer() const NOEXCEPT;

    /// Responder static key and certificate (initiator, after handshake).
    const cipher::key& static_key() const NOEXCEPT;
    const cipher::certificate& certificate() const NOEXCEPT;

    /// Perform handshake (initiator sends act1, responder answers act2).
    void async_handshake(result_handler&& handler) NOEXCEPT;

    /// Perform handshake as responder, given act1 prefix bytes consumed by
    /// socket (detection).
    void async_handshake(const const_byte_span& detected,
        result_handler&& handler) NOEXCEPT;

    /// Read received application bytes.
    template <typename Buffers, typename Handler>
    void async_read_some(const Buffers& buffers, Handler&& handler) NOEXCEPT;

    /// Write application bytes.
    template <typename Buffers, typename Handler>
    void async_write_some(const Buffers& buffers, Handler&& handler) NOEXCEPT;

    /// Write application bytes (blocking), no asynchronous write in progress.
    template <typename Buffers>
    size_t write_some(const Buffers& buffers, code& ec) NOEXCEPT;

    /// Complete once written frames are sent (Noise has no close message).
    void async_shutdown(result_handler&& handler) NOEXCEPT;

private:
    template <typename Buffer, typename Buffers>
    static Buffer first(const Buffers& buffers) NOEXCEPT;
    template <typename Buffers>
    const_byte_span flatten(const Buffers& buffers) NOEXCEPT;
    template <typename Handler>
    static io_handler erase(Handler&& handler) NOEXCEPT;

    // handshake
    void do_respond(size_t detected, const result_handler& handler) NOEXCEPT;
    void handle_act1(const code& ec,
        const result_handler& handler) NOEXCEPT;
    void do_initiate(const result_handler& handler) NOEXCEPT;
    void handle_act1_sent(const code& ec,
        const result_handler& handler) NOEXCEPT;
    void handle_act2(const code& ec,
        const result_handler& handler) NOEXCEPT;

    // read
    void do_read_some(const byte_span& out,
        const io_handler& handler) NOEXCEPT;
    size_t take(const byte_span& out) NOEXCEPT;
    void handle_heading(const code& ec, const byte_span& out,
        const io_handler& handler) NOEXCEPT;
    void handle_payload(const code& ec, size_t length, const byte_span& out,
        const io_handler& handler) NOEXCEPT;

    // write
    bool seal(const const_byte_span& data) NOEXCEPT;
    void do_write_some(const const_byte_span& data,
        const io_handler& handler) NOEXCEPT;
    size_t do_write(const const_byte_span& data, code& ec) NOEXCEPT;
    void flush(const result_handler& handler) NOEXCEPT;
    void write_next() NOEXCEPT;
    void handle_write(const code& ec,
        const std::vector<result_handler>& handlers) NOEXCEPT;

    // These are protected by stream (executor) sequencing.
    io::socket socket_;
    const context* context_;
    std::optional<cipher> cipher_{};
    cipher::act1 act1_{};
    cipher::act2 act2_{};
    cipher::key static_key_{};
    cipher::certificate certificate_{};
    system::data_chunk input_{};
    system::data_chunk received_{};
    size_t offset_{};
    system::data_chunk plaintext_{};
    system::data_chunk pending_{};
    system::data_chunk output_{};
    system::data_chunk sending_{};
    std::vector<result_handler> waiting_{};
    bool writing_{};
};

} // namespace noise
} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/noise/stream.ipp>

#endif
