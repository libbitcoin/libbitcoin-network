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
#ifndef LIBBITCOIN_NETWORK_WS_CLIENT_HPP
#define LIBBITCOIN_NETWORK_WS_CLIENT_HPP

#include <string>
#include <string_view>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/flat_buffer.hpp>
#include <bitcoin/network/ws/frame.hpp>
#include <bitcoin/network/ws/handshake.hpp>
#include <bitcoin/network/ws/reader.hpp>

namespace libbitcoin {
namespace network {
namespace ws {

/// Not thread safe. The client side of a websocket over a stream, by
/// blocking read_some and write_some. Frames are masked, pings are answered
/// with pongs, and a peer's close is answered with its close code.
template <typename Stream>
class client
{
public:
    DELETE_COPY_MOVE(client);

    /// The stream must outlive the client. A message larger than maximum is
    /// rejected (message_too_big).
    client(Stream& stream, size_t maximum=max_size_t) NOEXCEPT;

    /// Upgrade the connection (rfc6455), keeping the server's response, with
    /// any additional request fields (such as authorization).
    code handshake(response& out, const std::string& host,
        const std::string& target, const http::fields& extra={}) NOEXCEPT;
    code handshake(const std::string& host,
        const std::string& target) NOEXCEPT;

    /// Write a whole message as one frame.
    code write(const std::string_view& data, bool binary=false) NOEXCEPT;

    /// Write control frames.
    code ping(const std::string_view& payload) NOEXCEPT;
    code pong(const std::string_view& payload) NOEXCEPT;

    /// Send close and read until the peer's close (data is discarded).
    code close(close_code value) NOEXCEPT;

    /// Read the next data message into out (appended), websocket_closed once
    /// the peer has closed.
    code read(http::flat_buffer& out) NOEXCEPT;

    /// The last message read was binary.
    bool binary() const NOEXCEPT;

private:
    code send(opcode kind, const std::string_view& payload) NOEXCEPT;
    code receive() NOEXCEPT;

    Stream& stream_;
    reader reader_;
    http::flat_buffer input_{};
    bool closing_{};
};

} // namespace ws
} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/ws/client.ipp>

#endif
