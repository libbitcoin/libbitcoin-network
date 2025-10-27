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

#include <memory>
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

/// beast::http::data_body
typedef boost::beast::http::vector_body<uint8_t> data_body;
typedef boost::beast::http::request<data_body> data_request;
typedef boost::beast::http::response<data_body> data_response;
////typedef boost::beast::http::request_parser<data_body> data_parser;
////typedef boost::beast::http::serializer<false, data_body> data_serializer;
typedef std::shared_ptr<const data_request> data_request_cptr;
typedef std::shared_ptr<data_request> data_request_ptr;

/// beast::http::string_body
typedef boost::beast::http::string_body string_body;
typedef boost::beast::http::request<string_body> string_request;
typedef boost::beast::http::response<string_body> string_response;
////typedef boost::beast::http::request_parser<string_body> string_parser;
////typedef boost::beast::http::serializer<false, string_body> string_serializer;
typedef std::shared_ptr<const string_request> string_request_cptr;
typedef std::shared_ptr<string_request> string_request_ptr;

/// beast::http::file_body
typedef boost::beast::http::file_body file_body;
typedef boost::beast::http::request<file_body> file_request;
typedef boost::beast::http::response<file_body> file_response;
////typedef boost::beast::http::request_parser<file_body> file_parser;
typedef boost::beast::http::serializer<false, file_body> file_serializer;
typedef std::shared_ptr<const file_request> file_request_cptr;
typedef std::shared_ptr<file_request> file_request_ptr;

/// general purpose
template <bool IsRequest, class Fields>
using header = boost::beast::http::header<IsRequest, Fields>;
typedef file_body::value_type file;
typedef boost::beast::http::field field;
typedef boost::beast::http::fields fields;
typedef boost::beast::flat_buffer flat_buffer;

/////// websockets
////using websocket = boost::beast::websocket::stream<asio::socket>;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
