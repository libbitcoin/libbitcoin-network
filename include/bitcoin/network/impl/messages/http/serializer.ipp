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
#ifndef LIBBITCOIN_NETWORK_IMPL_MESSAGES_HTTP_SERIALIZER_IPP
#define LIBBITCOIN_NETWORK_IMPL_MESSAGES_HTTP_SERIALIZER_IPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

#define TEMPLATE template <bool IsRequest, class Body>
#define CLASS serializer<IsRequest, Body>

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

TEMPLATE
CLASS::serializer(message_type& message) NOEXCEPT
  : message_(message),
    writer_(message.base(), message.body()),
    chunked_(message.chunked())
{
}

TEMPLATE
const_buffers CLASS::next(code& ec) NOEXCEPT
{
    ec.clear();
    switch (state_)
    {
        case state::header:
        {
            header_ = serialize_header(message_.base());
            writer_.init(ec);
            if (ec)
                return {};

            state_ = state::body;
            return { io::const_buffer{ header_.data(), header_.size() } };
        }
        case state::body:
        {
            const auto out = writer_.get(ec);
            if (ec)
                return {};

            const auto more = out.has_value() && out.value().second;
            const auto data = out.has_value() ? out.value().first :
                io::const_buffer{};

            if (!more)
                state_ = state::done;

            if (!chunked_)
                return { data };

            const_buffers buffers{};
            if (!is_zero(data.size()))
            {
                prefix_ = chunk_header(data.size());
                buffers.emplace_back(prefix_.data(), prefix_.size());
                buffers.push_back(data);
                buffers.emplace_back(chunk_end.data(), chunk_end.size());
            }

            if (!more)
                buffers.emplace_back(last_chunk.data(), last_chunk.size());

            return buffers;
        }
        case state::done:
        default:
            return {};
    }
}

TEMPLATE
bool CLASS::is_header_done() const NOEXCEPT
{
    return state_ != state::header;
}

TEMPLATE
bool CLASS::is_done() const NOEXCEPT
{
    return state_ == state::done;
}

TEMPLATE
bool CLASS::binary() const NOEXCEPT
{
    return writer_.binary();
}

TEMPLATE
typename CLASS::message_type& CLASS::get() NOEXCEPT
{
    return message_;
}

#undef CLASS
#undef TEMPLATE

#define TEMPLATE template <bool IsRequest, class Body>
#define CLASS body_serializer<IsRequest, Body>

TEMPLATE
CLASS::body_serializer(message_type& message) NOEXCEPT
  : writer_(message.base(), message.body())
{
}

TEMPLATE
const_buffers CLASS::next(code& ec) NOEXCEPT
{
    ec.clear();
    if (done_)
        return {};

    if (!initialized_)
    {
        initialized_ = true;
        writer_.init(ec);
        if (ec)
            return {};
    }

    const auto out = writer_.get(ec);
    if (ec)
        return {};

    done_ = !out.has_value() || !out.value().second;
    return { out.has_value() ? out.value().first : io::const_buffer{} };
}

TEMPLATE
bool CLASS::is_done() const NOEXCEPT
{
    return done_;
}

TEMPLATE
bool CLASS::binary() const NOEXCEPT
{
    return writer_.binary();
}

BC_POP_WARNING()

#undef CLASS
#undef TEMPLATE

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
