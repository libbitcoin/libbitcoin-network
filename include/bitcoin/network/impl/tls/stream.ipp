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
#ifndef LIBBITCOIN_NETWORK_IMPL_TLS_STREAM_IPP
#define LIBBITCOIN_NETWORK_IMPL_TLS_STREAM_IPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tls/constants.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

template <typename Buffers, typename Handler>
void stream::async_read_some(const Buffers& buffers, Handler&& handler) NOEXCEPT
{
    const auto buffer = first<io::mutable_buffer>(buffers);
    do_read_some({ system::pointer_cast<uint8_t>(buffer.data()),
        buffer.size() }, erase(std::forward<Handler>(handler)));
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
    for (auto it = io::buffer_sequence_begin(buffers); it != end;
        ++it)
    {
        const Buffer buffer{ *it };
        if (is_nonzero(buffer.size()))
            return buffer;
    }

    return {};
}

// private
// One record of plaintext, gathered only when it spans buffers.
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
    if (head.size() >= maximum_plaintext || std::next(it) == end)
        return { data, std::min(head.size(), maximum_plaintext) };

    plaintext_.clear();
    for (; it != end && plaintext_.size() < maximum_plaintext; ++it)
    {
        const io::const_buffer buffer{ *it };
        const auto size = std::min(buffer.size(),
            maximum_plaintext - plaintext_.size());
        const auto bytes = system::pointer_cast<const uint8_t>(buffer.data());
        plaintext_.insert(plaintext_.cend(), bytes, std::next(bytes, size));
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

} // namespace tls
} // namespace network
} // namespace libbitcoin

#endif
