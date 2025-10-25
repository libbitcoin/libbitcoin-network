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

#include <bitcoin/network/messages/json/serializer.hpp>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {

TEMPLATE
void CLASS::init(error_code& ec) NOEXCEPT
{
    ec.clear();
}

TEMPLATE
void CLASS::finish(error_code& ec) NOEXCEPT
{
    // BUGBUG: only serializing request (parser::value_type).
    buffer_ = json::serializer<value_type>::write(body_);
    if (buffer_.empty())
    {
        using namespace boost::system::errc;
        ec = make_error_code(protocol_error);
        return;
    }

    ec.clear();
}
    
TEMPLATE
template <class ConstBufferSequence>
size_t CLASS::get(ConstBufferSequence& buffers, error_code& ec) NOEXCEPT
{
    if (buffer_.empty())
    {
        ec.clear();
        return {};
    }

    auto view = boost::asio::buffer(buffer_);
    boost::asio::buffer_copy(buffers, view);
    return view.size();
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
