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
    
// IPv6 normalizes to bracketed form.
// IPv6 (read) : [2001:db8::2] or 2001:db8::2
// IPv4 (read) : 1.2.240.1
// IPv6 (write): [2001:db8::2]
// IPv4 (write): 1.2.240.1
    
/// string/string conversions.
std::string to_host_name(const std::string& host) NOEXCEPT;
std::string to_text(const std::string& host, uint16_t port) NOEXCEPT;
std::string to_ipv6(const std::string& ipv4_address) NOEXCEPT;

/// asio/asio conversions.
asio::ipv6 to_ipv6(const asio::ipv4& ipv4_address) NOEXCEPT;
asio::ipv6 to_ipv6(const asio::address& ip_address) NOEXCEPT;

/// asio/string conversions.
std::string to_ipv4_host(const asio::address& ip_address) NOEXCEPT;
std::string to_ipv6_host(const asio::address& ip_address) NOEXCEPT;
std::string to_ip_host(const asio::address& ip_address) NOEXCEPT;

/// asio/messages conversions.
messages::ip_address to_address(const asio::ipv6& in) NOEXCEPT;
asio::ipv6 to_address(const messages::ip_address& in) NOEXCEPT;

/// Conditions.
inline bool is_valid(const messages::address_item& host) NOEXCEPT
{
    return host.port != messages::unspecified_ip_port
        && host.ip != messages::unspecified_ip_address;
}

} // namespace config
} // namespace network
} // namespace libbitcoin

#endif
