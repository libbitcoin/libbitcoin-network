/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_BODY_WRITER_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_BODY_WRITER_IPP

#include <optional>
#include <bitcoin/network/messages/json/parser.hpp>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {

TEMPLATE
template <class Buffers>
size_t CLASS::put(const Buffers& buffers, error_code& ec) NOEXCEPT
{
    using namespace boost::asio;
    const auto increase = buffer_size(buffers);
    buffer_.reserve(system::ceilinged_add(buffer_.size(), increase));

    size_t added{};
    for (auto const& buffer: buffers)
    {
        const auto size = buffer_size(buffer);
        const auto data = buffer_cast<const char_t*>(buffer);
        buffer_.append(data, size);
        added += size;
    }

    ec.clear();
    return added;
}

TEMPLATE
void CLASS::init(error_code& ec) NOEXCEPT
{
    buffer_.clear();
    ec.clear();
}

TEMPLATE
void CLASS::finish(error_code& ec) NOEXCEPT
{
    // Nothing written to the response implies an error.
    if (buffer_.empty())
    {
        using namespace boost::system::errc;
        ec = make_error_code(protocol_error);
    }
}

TEMPLATE
CLASS::buffer_t CLASS::buffer() NOEXCEPT
{
    return buffer_;
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
