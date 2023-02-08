/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_CONFIG_UTILITIES_HPP
#define LIBBITCOIN_NETWORK_CONFIG_UTILITIES_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace config {

/// string/string conversions (ipv6 always bracketed).
std::string to_literal(const std::string& host, uint16_t port) NOEXCEPT;

/// asio/string conversions (normalize to ipv4 unmapped).
std::string to_host(const asio::address& ip) NOEXCEPT;
asio::address from_host(const std::string& host) NOEXCEPT;

/// asio/messages conversions.
messages::ip_address to_address(const asio::address& ip) NOEXCEPT;
asio::address from_address(const messages::ip_address& address) NOEXCEPT;

/// Valid if the host is not unspecified and port is non-zero.
bool is_valid(const messages::address_item& item) NOEXCEPT;

// test only

} // namespace config
} // namespace network
} // namespace libbitcoin

#endif
