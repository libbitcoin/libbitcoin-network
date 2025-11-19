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
#ifndef LIBBITCOIN_NETWORK_CONFIG_UTILITIES_HPP
#define LIBBITCOIN_NETWORK_CONFIG_UTILITIES_HPP

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/endpoint.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/peer/peer.hpp>

namespace libbitcoin {
namespace network {
namespace config {

/// True if ip_address starts with the ip map prefix (maps to a v4 address).
constexpr bool is_v4(const messages::peer::ip_address& ip) NOEXCEPT
{
    using namespace system::config;
    return std::equal(ip_map_prefix.begin(), ip_map_prefix.end(), ip.begin());
}

constexpr bool is_v6(const messages::peer::ip_address& ip) NOEXCEPT
{
    return !is_v4(ip);
}

/// Not denormalizing.
BCT_API messages::peer::ip_address to_address(const asio::address& ip) NOEXCEPT;
BCT_API asio::address from_address(const messages::peer::ip_address& address) NOEXCEPT;

/// Get ascii lowered host:port form, with default substitution.
BCT_API std::string to_normal_host(const std::string& host,
    uint16_t default_port) NOEXCEPT;

} // namespace config
} // namespace network
} // namespace libbitcoin

#endif
