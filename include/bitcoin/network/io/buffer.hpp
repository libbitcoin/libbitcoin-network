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
#ifndef LIBBITCOIN_NETWORK_IO_BUFFER_HPP
#define LIBBITCOIN_NETWORK_IO_BUFFER_HPP

#include <iterator>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {
namespace io {

/// A writable region of memory (not owned).
class mutable_buffer
{
public:
    constexpr mutable_buffer() NOEXCEPT = default;
    constexpr mutable_buffer(void* data, size_t size) NOEXCEPT
      : data_(data), size_(size)
    {
    }

    constexpr void* data() const NOEXCEPT
    {
        return data_;
    }

    constexpr size_t size() const NOEXCEPT
    {
        return size_;
    }

    /// Advance the start of the region (bounded by its size).
    constexpr mutable_buffer& operator+=(size_t count) NOEXCEPT
    {
        const auto step = std::min(count, size_);
        data_ = std::next(static_cast<uint8_t*>(data_), step);
        size_ -= step;
        return *this;
    }

private:
    void* data_{};
    size_t size_{};
};

/// A readable region of memory (not owned).
class const_buffer
{
public:
    constexpr const_buffer() NOEXCEPT = default;
    constexpr const_buffer(const void* data, size_t size) NOEXCEPT
      : data_(data), size_(size)
    {
    }

    constexpr const_buffer(const mutable_buffer& buffer) NOEXCEPT
      : data_(buffer.data()), size_(buffer.size())
    {
    }

    constexpr const void* data() const NOEXCEPT
    {
        return data_;
    }

    constexpr size_t size() const NOEXCEPT
    {
        return size_;
    }

    /// Advance the start of the region (bounded by its size).
    constexpr const_buffer& operator+=(size_t count) NOEXCEPT
    {
        const auto step = std::min(count, size_);
        data_ = std::next(static_cast<const uint8_t*>(data_), step);
        size_ -= step;
        return *this;
    }

private:
    const void* data_{};
    size_t size_{};
};

constexpr mutable_buffer operator+(mutable_buffer buffer,
    size_t count) NOEXCEPT
{
    return buffer += count;
}

constexpr const_buffer operator+(const_buffer buffer, size_t count) NOEXCEPT
{
    return buffer += count;
}

/// Buffer construction.
/// ---------------------------------------------------------------------------

constexpr mutable_buffer buffer(void* data, size_t size) NOEXCEPT
{
    return { data, size };
}

constexpr const_buffer buffer(const void* data, size_t size) NOEXCEPT
{
    return { data, size };
}

constexpr mutable_buffer buffer(const mutable_buffer& value) NOEXCEPT
{
    return value;
}

constexpr const_buffer buffer(const const_buffer& value) NOEXCEPT
{
    return value;
}

constexpr mutable_buffer buffer(const mutable_buffer& value,
    size_t maximum) NOEXCEPT
{
    return { value.data(), std::min(value.size(), maximum) };
}

constexpr const_buffer buffer(const const_buffer& value,
    size_t maximum) NOEXCEPT
{
    return { value.data(), std::min(value.size(), maximum) };
}

/// The bytes of a contiguous range (mutable unless of const elements).
template <typename Range>
    requires std::ranges::contiguous_range<Range> &&
        std::ranges::sized_range<Range>
constexpr auto buffer(Range&& range) NOEXCEPT
{
    using element = std::remove_reference_t<std::ranges::range_reference_t<
        Range>>;

    const auto size = std::ranges::size(range) * sizeof(element);
    if constexpr (std::is_const_v<element>)
        return const_buffer{ std::ranges::data(range), size };
    else
        return mutable_buffer{ std::ranges::data(range), size };
}

/// Buffer sequences (a buffer, or a range of buffers).
/// ---------------------------------------------------------------------------

template <typename Type>
constexpr bool is_buffer = std::is_same_v<Type, mutable_buffer> ||
    std::is_same_v<Type, const_buffer>;

constexpr const mutable_buffer* buffer_sequence_begin(
    const mutable_buffer& buffer) NOEXCEPT
{
    return &buffer;
}

constexpr const mutable_buffer* buffer_sequence_end(
    const mutable_buffer& buffer) NOEXCEPT
{
    return std::next(&buffer);
}

constexpr const const_buffer* buffer_sequence_begin(
    const const_buffer& buffer) NOEXCEPT
{
    return &buffer;
}

constexpr const const_buffer* buffer_sequence_end(
    const const_buffer& buffer) NOEXCEPT
{
    return std::next(&buffer);
}

template <typename Sequence>
    requires (!is_buffer<Sequence>)
constexpr auto buffer_sequence_begin(const Sequence& sequence) NOEXCEPT
{
    return std::ranges::begin(sequence);
}

template <typename Sequence>
    requires (!is_buffer<Sequence>)
constexpr auto buffer_sequence_end(const Sequence& sequence) NOEXCEPT
{
    return std::ranges::end(sequence);
}

/// The total size of the buffers of a sequence.
template <typename Sequence>
constexpr size_t buffer_size(const Sequence& sequence) NOEXCEPT
{
    size_t size{};
    const auto end = buffer_sequence_end(sequence);
    for (auto it = buffer_sequence_begin(sequence); it != end; ++it)
        size += const_buffer{ *it }.size();

    return size;
}

using const_buffers = std::vector<const_buffer>;
using mutable_buffers = std::vector<mutable_buffer>;

/// The buffers of a sequence.
template <typename Buffer, typename Sequence>
inline std::vector<Buffer> to_buffers(const Sequence& sequence) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    std::vector<Buffer> out{};
    const auto end = buffer_sequence_end(sequence);
    for (auto it = buffer_sequence_begin(sequence); it != end; ++it)
        out.push_back(Buffer{ *it });

    return out;
    BC_POP_WARNING()
}

} // namespace io
} // namespace network
} // namespace libbitcoin

#endif
