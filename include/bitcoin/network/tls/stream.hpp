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
#ifndef LIBBITCOIN_NETWORK_TLS_STREAM_HPP
#define LIBBITCOIN_NETWORK_TLS_STREAM_HPP

#include <vector>
#include <bitcoin/network/io/io.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tls/context.hpp>
#include <bitcoin/network/tls/server.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

/// The server side of a TLS 1.3 connection over a tcp socket. A read fills
/// the first non-empty buffer of a sequence, a write protects the sequence as
/// one record (up to maximum_plaintext). Records produced by reads (alerts and
/// key updates) are sent in order with written records.
/// All calls must be sequenced on the underlying socket's executor. Read and
/// write chains may overlap each other but not themselves.
class BCT_API stream
{
public:
    DELETE_COPY_MOVE(stream);

    typedef std::function<void(const code&)> result_handler;
    typedef std::function<void(const code&, size_t)> io_handler;
    using executor_type = io::socket::executor_type;

    /// Assume ownership of the connected tcp socket.
    /// The context must outlive the stream.
    stream(io::socket&& socket, const context& context) NOEXCEPT;

    /// The executor and the socket of the stream.
    executor_type get_executor() NOEXCEPT;
    io::socket& next_layer() NOEXCEPT;
    const io::socket& next_layer() const NOEXCEPT;

    /// Perform the server handshake.
    void async_handshake(result_handler&& handler) NOEXCEPT;

    /// Read received application bytes.
    template <typename Buffers, typename Handler>
    void async_read_some(const Buffers& buffers, Handler&& handler) NOEXCEPT;

    /// Write application bytes.
    template <typename Buffers, typename Handler>
    void async_write_some(const Buffers& buffers, Handler&& handler) NOEXCEPT;

    /// Write application bytes (blocking), with no asynchronous
    /// write in progress.
    template <typename Buffers>
    size_t write_some(const Buffers& buffers, code& ec) NOEXCEPT;

    /// Send close_notify and await the peer's.
    void async_shutdown(result_handler&& handler) NOEXCEPT;

private:
    template <typename Buffer, typename Buffers>
    static Buffer first(const Buffers& buffers) NOEXCEPT;
    template <typename Buffers>
    const_byte_span flatten(const Buffers& buffers) NOEXCEPT;
    template <typename Handler>
    static io_handler erase(Handler&& handler) NOEXCEPT;

    code failure() const NOEXCEPT;
    code transport(const code& ec) const NOEXCEPT;

    // handshake
    void handle_handshake(const code& ec,
        const result_handler& handler) NOEXCEPT;

    // read
    void do_read_some(const byte_span& out,
        const io_handler& handler) NOEXCEPT;
    bool try_read(code& ec, size_t& size,
        const byte_span& out) NOEXCEPT;
    void handle_read_some(const code& ec, const byte_span& out,
        const io_handler& handler) NOEXCEPT;

    // write
    void do_write_some(const const_byte_span& data,
        const io_handler& handler) NOEXCEPT;
    size_t do_write(const const_byte_span& data,
        code& ec) NOEXCEPT;

    // shutdown
    void handle_close_sent(const code& ec,
        const result_handler& handler) NOEXCEPT;
    void handle_close_received(const code& ec,
        const result_handler& handler) NOEXCEPT;

    // transport
    void receive(result_handler&& handler) NOEXCEPT;
    void handle_receive(const code& ec, size_t size) NOEXCEPT;
    void flush(const result_handler& handler) NOEXCEPT;
    void write_next() NOEXCEPT;
    void handle_write(const code& ec,
        const std::vector<result_handler>& handlers) NOEXCEPT;

    // These are protected by stream (executor) sequencing.
    io::socket socket_;
    const context& context_;
    server server_;
    system::data_chunk input_;
    system::data_chunk plaintext_{};
    system::data_chunk sending_{};
    std::vector<result_handler> receivers_{};
    std::vector<result_handler> waiting_{};
    bool receiving_{};
    bool writing_{};
};

} // namespace tls
} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/tls/stream.ipp>

#endif
