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
#ifndef LIBBITCOIN_NETWORK_WS_FRAME_HPP
#define LIBBITCOIN_NETWORK_WS_FRAME_HPP

#include <optional>
#include <span>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace ws {

/// Frame opcodes (rfc6455).
enum class opcode : uint8_t
{
    continuation = 0x0,
    text = 0x1,
    binary = 0x2,
    close = 0x8,
    ping = 0x9,
    pong = 0xa
};

/// Close status codes (rfc6455).
enum class close_code : uint16_t
{
    normal = 1000,
    going_away = 1001,
    protocol_error = 1002,
    unknown_data = 1003,
    bad_payload = 1007,
    policy_error = 1008,
    too_big = 1009,
    needs_extension = 1010,
    internal_error = 1011
};

/// The frame masking key.
using mask = std::array<uint8_t, 4>;

/// A frame header.
struct frame_header
{
    bool fin{};
    opcode code{};
    bool masked{};
    uint64_t length{};
    mask key{};
};

/// The largest frame header (two bytes, eight of length, four of mask).
constexpr size_t maximum_header = 14;

/// The largest control frame payload.
constexpr size_t maximum_control = 125;

/// Parse a frame header from the front of data, returning its size, or zero
/// with need_more if incomplete. Frames received by a server must be masked,
/// those received by a client must not be.
BCT_API size_t parse_header(frame_header& out,
    const std::span<const uint8_t>& data, bool server, code& ec) NOEXCEPT;

/// The bytes of a frame header, masked if key is provided.
BCT_API system::data_chunk serialize_header(opcode code, bool fin,
    uint64_t length, const std::optional<mask>& key={}) NOEXCEPT;

/// Apply (or remove) the mask to the payload bytes at the payload offset.
BCT_API void apply_mask(const std::span<uint8_t>& data, const mask& key,
    uint64_t offset) NOEXCEPT;

/// The close code is valid in a close frame.
BCT_API bool is_valid_close_code(uint16_t value) NOEXCEPT;

/// Parse a close frame payload (empty, or a code and a utf8 reason).
BCT_API bool parse_close(uint16_t& value, std::string& reason,
    const std::span<const uint8_t>& payload, code& ec) NOEXCEPT;

/// The payload of a close frame with the code.
BCT_API system::data_chunk close_payload(close_code value) NOEXCEPT;

/// Not thread safe, non-virtual.
/// Incremental utf8 (rfc3629) validation across message fragments.
class BCT_API utf8_validator
{
public:
    /// False if the bytes are invalid utf8 (given preceding bytes).
    bool write(const std::span<const uint8_t>& data) NOEXCEPT;

    /// False if the bytes end within a character.
    bool finish() NOEXCEPT;

private:
    uint8_t needed_{};
    uint8_t lower_{ 0x80 };
    uint8_t upper_{ 0xbf };
};

} // namespace ws
} // namespace network
} // namespace libbitcoin

#endif
