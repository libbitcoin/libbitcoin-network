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

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace config {

/// IPv6 normalizes IPv4 addresses as "mapped" addresses, i.e. mapped into the
/// IPv6 address space. P2P protocol encodes all addresses in this normal form.
/// For serialization purposes we encode/decode only to/from denormalized form.
/// IPv6 "host names" are not bracketed, however IPv6 addresses are bracked.
/// This provides distinction from the port number (otherwise conflating ":").
/// This form is referred to as "literal" IPv6 encoding (from IPv6 URIs). All
/// addresses must be literal encodings, all host names are serialized as non-
/// literal, and deserialized as either literal or non-literal.

/// datatracker.ietf.org/doc/html/rfc4291
constexpr size_t ipv4_size = 4;
constexpr size_t ipv6_size = 16;
static constexpr system::data_array<ipv6_size - ipv4_size> ip_map_prefix
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff
};

/// Valid if the host is not unspecified and port is non-zero.
constexpr bool is_valid(const messages::address_item& item) NOEXCEPT
{
    return !is_zero(item.port) && item.ip != messages::unspecified_ip_address;
}

/// True if ip_address starts with the ip map prefix (maps to a v4 address).
constexpr bool is_v4(const messages::ip_address& ip) NOEXCEPT
{
    return std::equal(ip_map_prefix.begin(), ip_map_prefix.end(), ip.begin());
}

/// Member if subnet addresses contain host.
bool is_member(const asio::address& ip, const asio::address& subnet,
    uint8_t cidr) NOEXCEPT;

/// Unmap IPv6-mapped addresses.
asio::address denormalize(const asio::address& ip) NOEXCEPT;

/// Denormalizes to IPv4 (unmapped), literal emits unbracketed.
std::string to_host(const asio::address& ip) NOEXCEPT;
std::string to_literal(const asio::address& ip) NOEXCEPT;
asio::address from_host(const std::string& host) NOEXCEPT(false);

/// Not denormalizing.
messages::ip_address to_address(const asio::address& ip) NOEXCEPT;
asio::address from_address(const messages::ip_address& address) NOEXCEPT;

/// Parsers.
bool parse_authority(asio::address& ip, uint16_t& port, uint8_t& cidr,
    const std::string& value) NOEXCEPT;
bool parse_endpoint(std::string& scheme, std::string& host, uint16_t& port,
    const std::string& value) NOEXCEPT;

} // namespace config
} // namespace network
} // namespace libbitcoin

#endif
