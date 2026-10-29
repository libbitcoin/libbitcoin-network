/**
 * Copyright (c) 2011-2026 libbitcoin developers
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

#include <optional>
#include <bitcoin/network/config/endpoint.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace config {

/// Not denormalizing.
BCT_API messages::peer::ip_address to_address(
    const system::ip::address& ip) NOEXCEPT;
BCT_API system::ip::address from_address(
    const messages::peer::ip_address& address) NOEXCEPT;

/// Conversions between system ip and asio types.
BCT_API asio::address to_asio(const system::ip::address& ip) NOEXCEPT;
BCT_API asio::endpoint to_asio(const system::ip::endpoint& endpoint) NOEXCEPT;
BCT_API system::ip::address from_asio(const asio::address& ip) NOEXCEPT;
BCT_API system::ip::endpoint from_asio(const asio::endpoint& endpoint) NOEXCEPT;

/// Get ascii lowered host:port form, with default substitution.
BCT_API std::string to_normal_host(const std::string& host,
    uint16_t default_port) NOEXCEPT;

/// Get ascii lowered host:port form of a url, with default substitution.
BCT_API std::string to_normal_origin(const std::string& origin,
    uint16_t default_port) NOEXCEPT;

} // namespace config
} // namespace network
} // namespace libbitcoin

#endif
