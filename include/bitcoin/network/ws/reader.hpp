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
#ifndef LIBBITCOIN_NETWORK_WS_READER_HPP
#define LIBBITCOIN_NETWORK_WS_READER_HPP

#include <span>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/flat_buffer.hpp>
#include <bitcoin/network/ws/frame.hpp>

namespace libbitcoin {
namespace network {
namespace ws {

/// Not thread safe, non-virtual.
/// Assemble data messages, and surface control frames, from received bytes.
/// Data payload is unmasked into the message buffer as it arrives, a control
/// frame is surfaced once its payload is complete.
class BCT_API reader
{
public:
    /// The completion of a message or control frame.
    enum class event
    {
        none,
        message,
        ping,
        pong,
        close
    };

    /// A message larger than maximum is rejected (message_too_big).
    reader(size_t maximum, bool server) NOEXCEPT;

    /// Consume received bytes, appending data payload to message, returning
    /// the number consumed. Consumption stops at an event. Sets need_more if
    /// incomplete and the remaining bytes are insufficient to proceed.
    size_t put(const std::span<const uint8_t>& data, http::flat_buffer& message,
        event& out, code& ec) NOEXCEPT;

    /// The completed message was binary (otherwise utf8 text).
    bool binary() const NOEXCEPT;

    /// The payload of the completed control frame.
    const system::data_chunk& control() const NOEXCEPT;

private:
    size_t put_header(const std::span<const uint8_t>& data,
        http::flat_buffer& message, event& out, code& ec) NOEXCEPT;
    size_t put_payload(const std::span<const uint8_t>& data,
        http::flat_buffer& message, event& out, code& ec) NOEXCEPT;

    const uint64_t maximum_;
    const bool server_;
    frame_header frame_{};
    uint64_t offset_{};
    bool payload_{};
    bool continuing_{};
    bool binary_{};
    uint64_t size_{};
    utf8_validator utf8_{};
    system::data_chunk control_{};
};

} // namespace ws
} // namespace network
} // namespace libbitcoin

#endif
