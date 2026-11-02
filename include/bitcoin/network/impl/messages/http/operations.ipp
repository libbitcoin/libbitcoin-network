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
#ifndef LIBBITCOIN_NETWORK_IMPL_MESSAGES_HTTP_OPERATIONS_IPP
#define LIBBITCOIN_NETWORK_IMPL_MESSAGES_HTTP_OPERATIONS_IPP

#include <stdexcept>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

template <typename Stream, bool IsRequest, class Body>
void read(Stream& stream, flat_buffer& buffer,
    message<IsRequest, Body>& out, code& ec) NOEXCEPT
{
    parser<IsRequest, Body> instance{};
    auto ended = false;
    while (true)
    {
        // Parse until more bytes are needed (the parser pauses at the header).
        while (true)
        {
            const auto used = instance.put(buffer.data(), ec);
            buffer.consume(used);
            if (instance.is_done())
            {
                out = instance.release();
                ec = error::success;
                return;
            }

            if (ec == error::http_error_t::need_more)
            {
                ec = error::success;
                break;
            }

            if (ec)
                return;

            if (is_zero(used))
                break;
        }

        // The end of the stream completes a body delimited by it.
        if (ended)
        {
            instance.put_eof(ec);
            if (!ec && instance.is_done())
                out = instance.release();

            return;
        }

        const auto room = std::min(transfer_size,
            system::floored_subtract(buffer.max_size(), buffer.size()));

        if (is_zero(room))
        {
            ec = error::http_error_t::buffer_overflow;
            return;
        }

        size_t size{};
        try
        {
            size = stream.read_some(buffer.prepare(room), ec);
        }
        catch (const std::length_error&)
        {
            ec = error::http_error_t::buffer_overflow;
            return;
        }

        buffer.commit(size);
        if (ec == error::peer_disconnect)
        {
            ended = true;
            ec = error::success;
            continue;
        }

        if (ec)
            return;
    }
}

template <typename Stream, bool IsRequest, class Body>
void write(Stream& stream, message<IsRequest, Body>& in, code& ec) NOEXCEPT
{
    serializer<IsRequest, Body> instance{ in };
    ec = error::success;
    while (!instance.is_done())
    {
        const auto buffers = instance.next(ec);
        if (ec)
            return;

        io::write(stream, buffers, ec);
        if (ec)
            return;
    }
}

template <typename Stream, bool IsRequest, class Body>
void write(Stream& stream, message<IsRequest, Body>&& in, code& ec) NOEXCEPT
{
    write(stream, in, ec);
}

BC_POP_WARNING()

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
