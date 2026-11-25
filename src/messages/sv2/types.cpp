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
#include <bitcoin/network/messages/sv2/types.hpp>

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

constexpr uint8_t first_printable = 0x20;
constexpr uint8_t last_printable = 0x7e;
constexpr uint32_t sign_bit = 0x80000000;
constexpr uint32_t exponent_bits = 0x7f800000;

// A count above the limit invalidates the source and reads nothing.
static data_chunk read_counted(reader& source, size_t count,
    size_t limit) NOEXCEPT
{
    if (count > limit)
    {
        source.invalidate();
        return {};
    }

    return source.read_bytes(count);
}

// BOOL
// ----------------------------------------------------------------------------

bool read_bool(reader& source) NOEXCEPT
{
    return to_bool(bit_and<uint8_t>(source.read_byte(), 0x01));
}

void write_bool(writer& sink, bool value) NOEXCEPT
{
    sink.write_byte(to_int<uint8_t>(value));
}

// F32
// ----------------------------------------------------------------------------

// An exponent of all ones is NaN or infinite.
float read_f32(reader& source) NOEXCEPT
{
    const auto bits = source.read_4_bytes_little_endian();
    if (to_bool(bit_and(bits, sign_bit)) ||
        bit_and(bits, exponent_bits) == exponent_bits)
    {
        source.invalidate();
        return {};
    }

    return std::bit_cast<float>(bits);
}

void write_f32(writer& sink, float value) NOEXCEPT
{
    sink.write_4_bytes_little_endian(std::bit_cast<uint32_t>(value));
}

// STR0_255
// ----------------------------------------------------------------------------

std::string read_str0_255(reader& source) NOEXCEPT
{
    const auto bytes = source.read_bytes(source.read_byte());
    const auto printable = [](uint8_t byte) NOEXCEPT
    {
        return byte >= first_printable && byte <= last_printable;
    };

    if (!std::ranges::all_of(bytes, printable))
    {
        source.invalidate();
        return {};
    }

    return { bytes.begin(), bytes.end() };
}

void write_str0_255(writer& sink, const std::string& value) NOEXCEPT
{
    BC_ASSERT(value.size() <= max_b0_255);
    sink.write_byte(possible_narrow_cast<uint8_t>(value.size()));
    sink.write_bytes(to_chunk(value));
}

// B0_32, B0_255, B0_64K
// ----------------------------------------------------------------------------

data_chunk read_b0_32(reader& source) NOEXCEPT
{
    return read_counted(source, source.read_byte(), max_b0_32);
}

data_chunk read_b0_255(reader& source) NOEXCEPT
{
    return source.read_bytes(source.read_byte());
}

data_chunk read_b0_64k(reader& source) NOEXCEPT
{
    return source.read_bytes(source.read_2_bytes_little_endian());
}

void write_b0_32(writer& sink, const data_chunk& value) NOEXCEPT
{
    BC_ASSERT(value.size() <= max_b0_32);
    write_b0_255(sink, value);
}

void write_b0_255(writer& sink, const data_chunk& value) NOEXCEPT
{
    BC_ASSERT(value.size() <= max_b0_255);
    sink.write_byte(possible_narrow_cast<uint8_t>(value.size()));
    sink.write_bytes(value);
}

void write_b0_64k(writer& sink, const data_chunk& value) NOEXCEPT
{
    BC_ASSERT(value.size() <= max_b0_64k);
    sink.write_2_bytes_little_endian(
        possible_narrow_cast<uint16_t>(value.size()));
    sink.write_bytes(value);
}

// OPTION[U32]
// ----------------------------------------------------------------------------

// OPTION is a sequence of at most one element.
std::optional<uint32_t> read_option_u32(reader& source) NOEXCEPT
{
    const auto count = source.read_byte();
    if (is_zero(count))
        return {};

    if (!is_one(count))
    {
        source.invalidate();
        return {};
    }

    return source.read_4_bytes_little_endian();
}

void write_option_u32(writer& sink,
    const std::optional<uint32_t>& value) NOEXCEPT
{
    sink.write_byte(to_int<uint8_t>(value.has_value()));
    if (value.has_value())
        sink.write_4_bytes_little_endian(value.value());
}

// SEQ0_255[U256], SEQ0_64K[U32]
// ----------------------------------------------------------------------------

// Elements are read only while the source is valid, as its count is not
// trusted to bound an allocation.
u256s read_seq0_255_u256(reader& source) NOEXCEPT
{
    u256s out{};
    const auto count = source.read_byte();
    for (size_t index{}; index < count && source; ++index)
        out.push_back(source.read_hash());

    return out;
}

std::vector<uint32_t> read_seq0_64k_u32(reader& source) NOEXCEPT
{
    std::vector<uint32_t> out{};
    const auto count = source.read_2_bytes_little_endian();
    for (size_t index{}; index < count && source; ++index)
        out.push_back(source.read_4_bytes_little_endian());

    return out;
}

void write_seq0_255_u256(writer& sink, const u256s& value) NOEXCEPT
{
    BC_ASSERT(value.size() <= max_b0_255);
    sink.write_byte(possible_narrow_cast<uint8_t>(value.size()));
    for (const auto& element: value)
        sink.write_bytes(element);
}

void write_seq0_64k_u32(writer& sink,
    const std::vector<uint32_t>& value) NOEXCEPT
{
    BC_ASSERT(value.size() <= max_b0_64k);
    sink.write_2_bytes_little_endian(
        possible_narrow_cast<uint16_t>(value.size()));
    for (const auto element: value)
        sink.write_4_bytes_little_endian(element);
}

// Sizes
// ----------------------------------------------------------------------------

size_t size_str0_255(const std::string& value) NOEXCEPT
{
    return add1(value.size());
}

size_t size_b0_255(const data_chunk& value) NOEXCEPT
{
    return add1(value.size());
}

size_t size_b0_64k(const data_chunk& value) NOEXCEPT
{
    return sizeof(uint16_t) + value.size();
}

size_t size_option_u32(const std::optional<uint32_t>& value) NOEXCEPT
{
    return add1(value.has_value() ? sizeof(uint32_t) : zero);
}

size_t size_seq0_255_u256(const u256s& value) NOEXCEPT
{
    return add1(value.size() * hash_size);
}

size_t size_seq0_64k_u32(const std::vector<uint32_t>& value) NOEXCEPT
{
    return sizeof(uint16_t) + value.size() * sizeof(uint32_t);
}

BC_POP_WARNING()

} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin
