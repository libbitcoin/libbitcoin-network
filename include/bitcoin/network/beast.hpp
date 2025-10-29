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
using empty_request = boost::beast::http::request<empty_body>;
using empty_response = boost::beast::http::response<empty_body>;
using empty_parser = boost::beast::http::request_parser<empty_body>;
using empty_serializer = boost::beast::http::serializer<false, empty_body>;
using empty_response_cptr = std::shared_ptr<const empty_response>;
using empty_request_cptr = std::shared_ptr<const empty_request>;
using empty_response_ptr = std::shared_ptr<empty_response>;
using empty_request_ptr = std::shared_ptr<empty_request>;

/// beast::http::data_body
using data_body = boost::beast::http::vector_body<uint8_t>;
using data_request = boost::beast::http::request<data_body>;
using data_response = boost::beast::http::response<data_body>;
using data_parser = boost::beast::http::request_parser<data_body>;
using data_serializer = boost::beast::http::serializer<false, data_body>;
using data_response_cptr = std::shared_ptr<const data_response>;
using data_request_cptr = std::shared_ptr<const data_request>;
using data_response_ptr = std::shared_ptr<data_response>;
using data_request_ptr = std::shared_ptr<data_request>;

/// beast::http::string_body
using string_body = boost::beast::http::string_body;
using string_request = boost::beast::http::request<string_body>;
using string_response = boost::beast::http::response<string_body>;
using string_parser = boost::beast::http::request_parser<string_body>;
using string_serializer = boost::beast::http::serializer<false, string_body>;
using string_request_cptr = std::shared_ptr<const string_request>;
using string_response_cptr = std::shared_ptr<const string_response>;
using string_response_ptr = std::shared_ptr<string_response>;
using string_request_ptr = std::shared_ptr<string_request>;

/// beast::http::file_body
using file_body = boost::beast::http::file_body;
using file_request = boost::beast::http::request<file_body>;
using file_response = boost::beast::http::response<file_body>;
using file_parser = boost::beast::http::request_parser<file_body>;
using file_serializer = boost::beast::http::serializer<false, file_body>;
using file_response_cptr = std::shared_ptr<const file_response>;
using file_request_cptr = std::shared_ptr<const file_request>;
using file_response_ptr = std::shared_ptr<file_response>;
using file_request_ptr = std::shared_ptr<file_request>;

/////// http::json::body
////using json_request = boost::beast::http::request<json_body>;
////using json_response = boost::beast::http::response<json_body>;
////using json_parser = boost::beast::http::request_parser<json_body>;
////using json_serializer = boost::beast::http::serializer<false, json_body>;
////using json_response_cptr = std::shared_ptr<const json_response>;
////using json_request_cptr = std::shared_ptr<const json_request>;
////using json_response_ptr = std::shared_ptr<json_response>;
////using json_request_ptr = std::shared_ptr<json_request>;

// Defined in types.hpp.
/////// http::body (variant)
////using request = boost::beast::http::request<body>;
////using response = boost::beast::http::response<body>;
////using parser = boost::beast::http::request_parser<body>;
////using serializer = boost::beast::http::serializer<false, body>;
////using response_cptr = std::shared_ptr<const response>;
////using request_cptr = std::shared_ptr<const request>;
////using response_ptr = std::shared_ptr<response>;
////using request_ptr = std::shared_ptr<request>;

/// general purpose
using file = file_body::value_type;
using field = boost::beast::http::field;
using fields = boost::beast::http::fields;
using flat_buffer = boost::beast::flat_buffer;
using flat_buffer_ptr = std::shared_ptr<flat_buffer>;
using flat_buffer_cptr = std::shared_ptr<const flat_buffer>;
using error_code = boost::system::error_code;

/// Required types for custom beast::http::body definition.
template <bool IsRequest, class Fields>
using header = boost::beast::http::header<IsRequest, Fields>;
template <class Buffer>
using get_buffer = boost::optional<std::pair<Buffer, bool>>;
using length_type = boost::optional<uint64_t>;
using request_header = header<true, fields>;
using response_header = header<false, fields>;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
