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
#ifndef LIBBITCOIN_NETWORK_ASYNC_ASIO_HPP
#define LIBBITCOIN_NETWORK_ASYNC_ASIO_HPP

#include <memory>
#include <bitcoin/network/async/time.hpp>
#include <bitcoin/network/define.hpp>

/// Convenience namespace for commonly used boost async i/o aliases.

namespace libbitcoin {
namespace network {
namespace asio {

// stackoverflow.com/questions/66765121/
// asiostrandasioio-contextexecutor-type-vs-io-contextstrand

// asio
typedef boost::asio::io_context io_context;
typedef boost::asio::io_context::executor_type executor_type;
typedef boost::asio::strand<executor_type> strand;
typedef boost::asio::steady_timer steady_timer;
typedef boost::asio::mutable_buffer mutable_buffer;
typedef boost::asio::const_buffer const_buffer;

// addressing
typedef boost::asio::ip::v6_only v6_only;
typedef boost::asio::ip::address address;
typedef boost::asio::ip::address_v4 ipv4;
typedef boost::asio::ip::address_v6 ipv6;
typedef boost::asio::ip::tcp tcp;

// accept/connect
typedef tcp::acceptor acceptor;
typedef tcp::resolver resolver;
typedef tcp::endpoint endpoint;
typedef resolver::results_type endpoints;
typedef asio::acceptor::reuse_address reuse_address;

// connect
typedef tcp::socket socket;
typedef std::shared_ptr<socket> socket_ptr;

constexpr auto max_connections = boost::asio::socket_base::max_listen_connections;

} // namespace asio

namespace http = boost::beast::http;

// flat_buffer
typedef boost::beast::flat_buffer http_flat_buffer;

// beast::http::vector_body<uint8_t>
typedef http::vector_body<uint8_t> http_data_body;
typedef http::request<http_data_body> http_data_request;
typedef http::response<http_data_body> http_data_response;
typedef http::request_parser<http_data_body> http_data_parser;
typedef http::serializer<false, http_data_body> http_data_serializer;
typedef std::shared_ptr<const http_data_request> http_data_request_cptr;
typedef std::shared_ptr<http_data_request> http_data_request_ptr;

// beast::http::string_body
typedef http::string_body http_string_body;
typedef http::request<http_string_body> http_string_request;
typedef http::response<http_string_body> http_string_response;
typedef http::request_parser<http_string_body> http_string_parser;
typedef http::serializer<false, http_string_body> http_string_serializer;
typedef std::shared_ptr<const http_string_request> http_string_request_cptr;
typedef std::shared_ptr<http_string_request> http_string_request_ptr;

// beast::http::file_body
typedef http::file_body http_file_body;
typedef http::request<http_file_body> http_file_request;
typedef http::response<http_file_body> http_file_response;
typedef http::request_parser<http_file_body> http_file_parser;
typedef http::serializer<false, http_file_body> http_file_serializer;
typedef std::shared_ptr<const http_file_request> http_file_request_cptr;
typedef std::shared_ptr<http_file_request> http_file_request_ptr;

typedef http_file_body::value_type http_file;
typedef http::fields http_fields;

constexpr int32_t http_version_1_1 = 11;
constexpr int32_t http_version_1_0 = 10;

} // namespace network
} // namespace libbitcoin

#endif
