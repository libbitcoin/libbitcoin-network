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
#ifndef LIBBITCOIN_NETWORK_ASYNC_BEAST_HPP
#define LIBBITCOIN_NETWORK_ASYNC_BEAST_HPP

#include <bitcoin/network/boost.hpp>

#include <optional>
#include <memory>
#include <utility>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

/// Convenience namespace for commonly used boost beast aliases.

namespace libbitcoin {
namespace network {
namespace http {

/// http protocol versions
constexpr int32_t version_1_1 = 11;
constexpr int32_t version_1_0 = 10;

/// beast::http::empty_body
using empty_body = boost::beast::http::empty_body;

/// beast::http::vector_body<uint8_t, bc::allocator<uint8_t>>
using chunk_body = boost::beast::http::vector_body<uint8_t, allocator<uint8_t>>;

/// beast::http::file_body
using file_body = boost::beast::http::file_body;

/// beast::http::span_body<uint8_t>
/// Must cast write span uint8_t* to non-const.
using span_body = boost::beast::http::span_body<uint8_t>;

/// beast::http::buffer_body
/// Must cast write buffer void* to non-const.
using buffer_body = boost::beast::http::buffer_body;

/// beast::http::string_body
using string_body = boost::beast::http::string_body;

/// general purpose
using file = file_body::value_type;
using field = boost::beast::http::field;
using fields = boost::beast::http::fields;
using flat_buffer = boost::beast::flat_buffer;
using flat_buffer_ptr = std::shared_ptr<flat_buffer>;
using flat_buffer_cptr = std::shared_ptr<const flat_buffer>;

/// Types required for custom http::body/head<> definitions.
template <bool IsRequest>
using empty_parser = boost::beast::http::parser<IsRequest, empty_body>;
template <bool IsRequest>
using empty_serializer = boost::beast::http::serializer<IsRequest, empty_body>;
template <bool IsRequest>
using empty_message = boost::beast::http::message<IsRequest, empty_body>;
template <bool IsRequest, class Fields = fields>
using message_header = boost::beast::http::header<IsRequest, Fields>;
template <class Buffer>
using get_buffer = boost::optional<std::pair<Buffer, bool>>;
using length_type = boost::optional<uint64_t>;
using request_header = message_header<true, fields>;
using response_header = message_header<false, fields>;
using empty_request = boost::beast::http::request<empty_body>;
using empty_response = boost::beast::http::response<empty_body>;

} // namespace http

namespace ws {
    
template <class Socket>
using stream = boost::beast::websocket::stream<Socket>;
using websocket = stream<boost::asio::ip::tcp::socket>;
using frame_type = boost::beast::websocket::frame_type;
using decorator = boost::beast::websocket::stream_base::decorator;

} // namespace ws
} // namespace network
} // namespace libbitcoin

#endif
