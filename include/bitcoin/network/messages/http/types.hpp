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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_TYPES_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_TYPES_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/messages.hpp>

namespace libbitcoin {
namespace network {
namespace http {
namespace json {

// TODO: for non-rpc http json (REST) messages, define:
// TODO: http::json::request
// TODO: http::json::response.

namespace rpc {

// TODO: need to split up the body to get request and response value_type.

using request = boost::beast::http::request
<
    network::json::body
    <
        network::json::parser<true>,
        network::json::serializer
    >
>;

using response = boost::beast::http::response
<
    network::json::body
    <
        network::json::parser<true>,
        network::json::serializer
    >
>;

} // namespace rpc
} // namespace json
} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
