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
#include <bitcoin/network/messages/http/flat_buffer.hpp>

#include <algorithm>
#include <cstring>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

using namespace system;

BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

flat_buffer::flat_buffer() NOEXCEPT
  : flat_buffer(max_size_t)
{
}

flat_buffer::flat_buffer(size_t limit) NOEXCEPT
  : limit_(limit)
{
}

flat_buffer::const_buffers_type flat_buffer::data() const NOEXCEPT
{
    return { std::next(buffer_.data(), begin_), size() };
}

flat_buffer::const_buffers_type flat_buffer::cdata() const NOEXCEPT
{
    return data();
}

flat_buffer::mutable_buffers_type flat_buffer::data() NOEXCEPT
{
    return { std::next(buffer_.data(), begin_), size() };
}

size_t flat_buffer::size() const NOEXCEPT
{
    return end_ - begin_;
}

size_t flat_buffer::max_size() const NOEXCEPT
{
    return limit_;
}

size_t flat_buffer::capacity() const NOEXCEPT
{
    return buffer_.size();
}

flat_buffer::mutable_buffers_type flat_buffer::prepare(size_t size) THROWS
{
    if (size > limit_ - this->size())
        throw std::length_error{ "flat_buffer overflow" };

    // Move readable bytes to the front when that makes sufficient room.
    if (buffer_.size() - end_ < size && !is_zero(begin_))
    {
        std::copy(std::next(buffer_.begin(), begin_),
            std::next(buffer_.begin(), end_), buffer_.begin());
        end_ -= begin_;
        begin_ = zero;
    }

    if (buffer_.size() - end_ < size)
        buffer_.resize(end_ + size);

    return { std::next(buffer_.data(), end_), size };
}

void flat_buffer::commit(size_t size) NOEXCEPT
{
    end_ += std::min(size, buffer_.size() - end_);
}

void flat_buffer::append(
    const std::span<const const_buffers_type>& buffers) THROWS
{
    // Buffer within allocation located by offset, as prepare may reallocate.
    const const_buffers_type* inner{};
    size_t from{};
    size_t to{};
    size_t size{};
    const auto begin = buffer_.data();
    const auto end = std::next(begin, buffer_.size());
    for (const auto& buffer: buffers)
    {
        const auto data = pointer_cast<const uint8_t>(buffer.data());
        if (!is_zero(buffer.size()) && !std::less{}(data, begin) &&
            std::less{}(data, end))
        {
            inner = &buffer;
            from = sign_cast<size_t>(std::distance<const uint8_t*>(begin,
                data));
            to = size;
        }

        size += buffer.size();
    }

    auto out = pointer_cast<uint8_t>(prepare(size).data());
    if (!is_null(inner))
        std::memmove(std::next(out, to), std::next(buffer_.data(), from),
            inner->size());

    for (const auto& buffer: buffers)
    {
        if (&buffer != inner)
            std::copy_n(pointer_cast<const uint8_t>(buffer.data()),
                buffer.size(), out);

        std::advance(out, buffer.size());
    }

    end_ += size;
}

void flat_buffer::consume(size_t size) NOEXCEPT
{
    if (size >= this->size())
    {
        clear();
        return;
    }

    begin_ += size;
}

void flat_buffer::clear() NOEXCEPT
{
    begin_ = zero;
    end_ = zero;
}

void flat_buffer::reserve(size_t size) NOEXCEPT
{
    if (buffer_.size() < size)
        buffer_.resize(size);
}

void flat_buffer::shrink_to_fit() NOEXCEPT
{
    if (!is_zero(begin_))
    {
        std::copy(std::next(buffer_.begin(), begin_),
            std::next(buffer_.begin(), end_), buffer_.begin());
        end_ -= begin_;
        begin_ = zero;
    }

    buffer_.resize(end_);
    buffer_.shrink_to_fit();
}

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace http
} // namespace network
} // namespace libbitcoin
