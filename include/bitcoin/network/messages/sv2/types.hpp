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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_SV2_TYPES_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_SV2_TYPES_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {

/// Stratum v2 data types (multibyte values are little-endian).
/// A reader invalidates its source on a value that is not of the type.

/// U256, the bytes of the integer in little-endian order.
using u256 = system::hash_digest;
using u256s = system::hashes;

/// Largest counts of the length-prefixed types.
constexpr size_t max_b0_32 = 32;
constexpr size_t max_b0_255 = max_uint8;
constexpr size_t max_b0_64k = max_uint16;

/// BOOL, only the least significant bit is interpreted.
BCT_API bool read_bool(system::reader& source) NOEXCEPT;
BCT_API void write_bool(system::writer& sink, bool value) NOEXCEPT;

/// F32, not NaN, infinite or negative (including negative zero).
BCT_API float read_f32(system::reader& source) NOEXCEPT;
BCT_API void write_f32(system::writer& sink, float value) NOEXCEPT;

/// STR0_255, printable ascii (0x20 to 0x7e).
BCT_API std::string read_str0_255(system::reader& source) NOEXCEPT;
BCT_API void write_str0_255(system::writer& sink,
    const std::string& value) NOEXCEPT;

/// B0_32, B0_255 and B0_64K.
BCT_API system::data_chunk read_b0_32(system::reader& source) NOEXCEPT;
BCT_API system::data_chunk read_b0_255(system::reader& source) NOEXCEPT;
BCT_API system::data_chunk read_b0_64k(system::reader& source) NOEXCEPT;
BCT_API void write_b0_32(system::writer& sink,
    const system::data_chunk& value) NOEXCEPT;
BCT_API void write_b0_255(system::writer& sink,
    const system::data_chunk& value) NOEXCEPT;
BCT_API void write_b0_64k(system::writer& sink,
    const system::data_chunk& value) NOEXCEPT;

/// OPTION[U32].
BCT_API std::optional<uint32_t> read_option_u32(
    system::reader& source) NOEXCEPT;
BCT_API void write_option_u32(system::writer& sink,
    const std::optional<uint32_t>& value) NOEXCEPT;

/// SEQ0_255[U256] and SEQ0_64K[U32].
BCT_API u256s read_seq0_255_u256(system::reader& source) NOEXCEPT;
BCT_API std::vector<uint32_t> read_seq0_64k_u32(
    system::reader& source) NOEXCEPT;
BCT_API void write_seq0_255_u256(system::writer& sink,
    const u256s& value) NOEXCEPT;
BCT_API void write_seq0_64k_u32(system::writer& sink,
    const std::vector<uint32_t>& value) NOEXCEPT;

/// Serialized sizes.
BCT_API size_t size_str0_255(const std::string& value) NOEXCEPT;
BCT_API size_t size_b0_255(const system::data_chunk& value) NOEXCEPT;
BCT_API size_t size_b0_64k(const system::data_chunk& value) NOEXCEPT;
BCT_API size_t size_option_u32(
    const std::optional<uint32_t>& value) NOEXCEPT;
BCT_API size_t size_seq0_255_u256(const u256s& value) NOEXCEPT;
BCT_API size_t size_seq0_64k_u32(
    const std::vector<uint32_t>& value) NOEXCEPT;

} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
