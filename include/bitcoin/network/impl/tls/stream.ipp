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
    const auto buffer = first<io::const_buffer>(buffers);
    do_write_some({ system::pointer_cast<const uint8_t>(buffer.data()),
        buffer.size() }, erase(std::forward<Handler>(handler)));
}

template <typename Buffers>
size_t stream::write_some(const Buffers& buffers, code& ec) NOEXCEPT
{
    const auto buffer = first<io::const_buffer>(buffers);
    return do_write({ system::pointer_cast<const uint8_t>(buffer.data()),
        buffer.size() }, ec);
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
