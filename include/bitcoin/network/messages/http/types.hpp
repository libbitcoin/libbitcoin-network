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

#include <memory>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/body.hpp>

namespace libbitcoin {
namespace network {
namespace http {

using json_parser = json::parser;
using json_serializer = json::serializer;
using json_body = json::body<json_parser, json_serializer>;
using json_request = boost::beast::http::request<json_body>;
using json_response = boost::beast::http::response<json_body>;
using json_request_cptr = std::shared_ptr<const json_request>;
using json_request_ptr = std::shared_ptr<json_response>;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
