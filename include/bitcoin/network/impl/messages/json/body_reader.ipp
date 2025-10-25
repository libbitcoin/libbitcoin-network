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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_BODY_READER_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_BODY_READER_IPP

#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {

TEMPLATE
template <class ConstBufferSequence>
size_t CLASS::put(const ConstBufferSequence& buffers, error_code& ec) NOEXCEPT
{
    // Prioritize existing parser error.
    if ((ec = parser_.get_error()))
        return {};

    // Already complete implies an error.
    if (parser_.is_done())
    {
        using namespace boost::system::errc;
        ec = make_error_code(protocol_error);
        return {};
    }

    size_t added{};
    for (auto const& buffer: buffers)
    {
        using namespace boost::asio;
        const auto size = buffer_size(buffer);
        const auto data = buffer_cast<const char*>(buffer);
        added += parser_.write({ data, size });
        if (parser_.is_done())
            break;
    }

    ec = parser_.get_error();
    return added;
}

TEMPLATE
void CLASS::init(const length_t&, error_code& ec) NOEXCEPT
{
    parser_.reset();
    ec.clear();
}

TEMPLATE
void CLASS::finish(error_code& ec) NOEXCEPT
{
    // Prioritize existing parser error.
    if ((ec = parser_.get_error()))
        return;

    // Premature completion implies an error.
    if (!parser_.is_done())
    {
        using namespace boost::system::errc;
        ec = make_error_code(protocol_error);
    }
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
