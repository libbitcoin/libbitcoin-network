/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <boost/asio.hpp>

/// Convenience namespace for commonly used boost async i/o aliases.

namespace libbitcoin {
namespace network {
namespace asio {

typedef boost::asio::io_context io_context;
typedef boost::asio::io_context::strand strand;
typedef boost::asio::io_context::executor_type work_guard;

typedef boost::asio::ip::address address;
typedef boost::asio::ip::address_v4 ipv4;
typedef boost::asio::ip::address_v6 ipv6;
typedef boost::asio::ip::tcp tcp;

typedef tcp::endpoint endpoint;
typedef tcp::socket socket;
typedef tcp::acceptor acceptor;
typedef tcp::resolver resolver;
typedef resolver::results_type iterator;
typedef std::shared_ptr<socket> socket_ptr;

constexpr auto max_connections = boost::asio::socket_base::max_listen_connections;

} // namespace asio
} // namespace network
} // namespace libbitcoin

#endif
