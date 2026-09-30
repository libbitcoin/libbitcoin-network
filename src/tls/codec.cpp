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
#include <bitcoin/network/tls/codec.hpp>

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

using namespace system;

BC_PUSH_WARNING(NO_ARRAY_INDEXING)

constexpr uint32_t maximum_24 = 0x00ffffff;

// reader
// ----------------------------------------------------------------------------

reader::reader(const const_byte_span& data) NOEXCEPT
  : data_(data), valid_(true)
{
}

reader::operator bool() const NOEXCEPT
{
    return valid_;
}

bool reader::is_complete() const NOEXCEPT
{
    return valid_ && data_.empty();
}

uint8_t reader::read_8() NOEXCEPT
{
    const auto bytes = read_bytes(one);
    return bytes.empty() ? uint8_t{} : bytes[0];
}

uint16_t reader::read_16() NOEXCEPT
{
    const auto bytes = read_bytes(two);
    if (bytes.empty())
        return {};

    const auto high = shift_left(wide_cast<uint16_t>(bytes[0]), byte_bits);
    return bit_or(high, wide_cast<uint16_t>(bytes[1]));
}

uint32_t reader::read_24() NOEXCEPT
{
    const auto bytes = read_bytes(3);
    if (bytes.empty())
        return {};

    uint32_t value{};
    for (const auto byte: bytes)
    {
        const auto shifted = shift_left(value, byte_bits);
        value = bit_or(shifted, wide_cast<uint32_t>(byte));
    }

    return value;
}

const_byte_span reader::read_bytes(size_t size) NOEXCEPT
{
    if (!valid_ || (size > data_.size()))
    {
        invalidate();
        return {};
    }

    const auto bytes = data_.first(size);
    data_ = data_.subspan(size);
    return bytes;
}

const_byte_span reader::read_vector_8() NOEXCEPT
{
    const auto size = read_8();
    return valid_ ? read_bytes(size) : const_byte_span{};
}

const_byte_span reader::read_vector_16() NOEXCEPT
{
    const auto size = read_16();
    return valid_ ? read_bytes(size) : const_byte_span{};
}

const_byte_span reader::read_vector_24() NOEXCEPT
{
    const auto size = read_24();
    return valid_ ? read_bytes(size) : const_byte_span{};
}

std::vector<uint16_t> reader::read_list_8() NOEXCEPT
{
    return read_list(read_vector_8());
}

std::vector<uint16_t> reader::read_list_16() NOEXCEPT
{
    return read_list(read_vector_16());
}

std::vector<uint16_t> reader::read_list(const const_byte_span& bytes) NOEXCEPT
{
    std::vector<uint16_t> out{};
    reader list{ bytes };
    while (valid_ && list && !list.is_complete())
        out.push_back(list.read_16());

    if (!list)
    {
        invalidate();
        return {};
    }

    return out;
}

void reader::invalidate() NOEXCEPT
{
    valid_ = false;
    data_ = {};
}

// writer
// ----------------------------------------------------------------------------

const data_chunk& writer::data() const NOEXCEPT
{
    return data_;
}

void writer::write_8(uint8_t value) NOEXCEPT
{
    data_.push_back(value);
}

void writer::write_16(uint16_t value) NOEXCEPT
{
    data_.push_back(narrow_cast<uint8_t>(shift_right(value, byte_bits)));
    data_.push_back(narrow_cast<uint8_t>(value));
}

void writer::write_24(uint32_t value) NOEXCEPT
{
    BC_ASSERT(value <= maximum_24);
    data_.push_back(narrow_cast<uint8_t>(shift_right(value, 16)));
    data_.push_back(narrow_cast<uint8_t>(shift_right(value, byte_bits)));
    data_.push_back(narrow_cast<uint8_t>(value));
}

void writer::write_bytes(const const_byte_span& bytes) NOEXCEPT
{
    data_.insert(data_.end(), bytes.begin(), bytes.end());
}

void writer::write_vector_8(const const_byte_span& bytes) NOEXCEPT
{
    BC_ASSERT(bytes.size() <= max_uint8);
    write_8(possible_narrow_cast<uint8_t>(bytes.size()));
    write_bytes(bytes);
}

void writer::write_vector_16(const const_byte_span& bytes) NOEXCEPT
{
    BC_ASSERT(bytes.size() <= max_uint16);
    write_16(possible_narrow_cast<uint16_t>(bytes.size()));
    write_bytes(bytes);
}

void writer::write_vector_24(const const_byte_span& bytes) NOEXCEPT
{
    BC_ASSERT(bytes.size() <= maximum_24);
    write_24(possible_narrow_cast<uint32_t>(bytes.size()));
    write_bytes(bytes);
}

BC_POP_WARNING()

} // namespace tls
} // namespace network
} // namespace libbitcoin
