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
#ifndef LIBBITCOIN_NETWORK_IMPL_NOISE_STREAM_IPP
#define LIBBITCOIN_NETWORK_IMPL_NOISE_STREAM_IPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace noise {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

template <typename Buffers, typename Handler>
void stream::async_read_some(const Buffers& buffers, Handler&& handler) NOEXCEPT
{
    const auto buffer = first<io::mutable_buffer>(buffers);
    const auto data = system::pointer_cast<uint8_t>(buffer.data());
    do_read_some({ data, buffer.size() },
        erase(std::forward<Handler>(handler)));
}

template <typename Buffers, typename Handler>
void stream::async_write_some(const Buffers& buffers,
    Handler&& handler) NOEXCEPT
{
    do_write_some(flatten(buffers), erase(std::forward<Handler>(handler)));
}

template <typename Buffers>
size_t stream::write_some(const Buffers& buffers, code& ec) NOEXCEPT
{
    return do_write(flatten(buffers), ec);
}

// private
template <typename Buffer, typename Buffers>
Buffer stream::first(const Buffers& buffers) NOEXCEPT
{
    const auto end = io::buffer_sequence_end(buffers);
    for (auto it = io::buffer_sequence_begin(buffers); it != end; ++it)
    {
        const Buffer buffer{ *it };
        if (is_nonzero(buffer.size()))
            return buffer;
    }

    return {};
}

// private
// The written bytes, gathered only when they span buffers.
template <typename Buffers>
const_byte_span stream::flatten(const Buffers& buffers) NOEXCEPT
{
    const auto end = io::buffer_sequence_end(buffers);
    auto it = io::buffer_sequence_begin(buffers);
    while (it != end && is_zero(io::const_buffer{ *it }.size()))
        ++it;

    if (it == end)
        return {};

    const io::const_buffer head{ *it };
    const auto data = system::pointer_cast<const uint8_t>(head.data());
    if (std::next(it) == end)
        return { data, head.size() };

    plaintext_.clear();
    for (; it != end; ++it)
    {
        const io::const_buffer buffer{ *it };
        const auto bytes = system::pointer_cast<const uint8_t>(buffer.data());
        plaintext_.insert(plaintext_.cend(), bytes,
            std::next(bytes, buffer.size()));
    }

    return plaintext_;
}

// private
template <typename Handler>
stream::io_handler stream::erase(Handler&& handler) NOEXCEPT
{
    const auto shared = system::emplace_shared<std::decay_t<Handler>>(
        std::forward<Handler>(handler));

    return [shared](const code& ec, size_t size) NOEXCEPT
    {
        std::move(*shared)(ec, size);
    };
}

BC_POP_WARNING()

} // namespace noise
} // namespace network
} // namespace libbitcoin

#endif
