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

/// asio/string conversions (normalize to ipv4 unmapped).
std::string to_host(const asio::address& ip) NOEXCEPT;
asio::address from_host(const std::string& host) NOEXCEPT(false);

/// string/string conversions (unmapped, ipv6 bracketed).
std::string to_literal(const asio::address& ip) NOEXCEPT;
asio::address from_literal(const std::string& host)  NOEXCEPT(false);

/// asio/messages conversions.
messages::ip_address to_address(const asio::address& ip) NOEXCEPT;
asio::address from_address(const messages::ip_address& address) NOEXCEPT;

/// unmap IPv6-mapped addresses.
asio::address normalize(const asio::address& ip) NOEXCEPT;

/// Valid if the host is not unspecified and port is non-zero.
bool is_valid(const messages::address_item& item) NOEXCEPT;

/// Member if subnet addresses contain host.
bool is_member(const asio::address& ip, const asio::address& subnet,
    uint8_t cidr) NOEXCEPT;

} // namespace config
} // namespace network
} // namespace libbitcoin

#endif
