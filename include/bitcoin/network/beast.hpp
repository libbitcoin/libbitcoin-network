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
#ifndef LIBBITCOIN_NETWORK_ASYNC_BEAST_HPP
#define LIBBITCOIN_NETWORK_ASYNC_BEAST_HPP

#include <bitcoin/network/asio.hpp>

#include <optional>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

/// Convenience namespace for commonly used boost beast aliases.

namespace libbitcoin {
namespace network {

/// ws::socket
namespace ws
{
    using socket = boost::beast::websocket::stream<asio::tcp::socket>;
    using decorator = boost::beast::websocket::stream_base::decorator;
    using frame_type = boost::beast::websocket::frame_type;

    /// ws::ssl::socket
    namespace ssl
    {
        using socket = boost::beast::websocket::stream<asio::ssl::socket>;
    }
}

} // namespace network
} // namespace libbitcoin

#endif
