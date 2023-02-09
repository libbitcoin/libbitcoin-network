/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_CONFIG_AUTHORITY_HPP
#define LIBBITCOIN_NETWORK_CONFIG_AUTHORITY_HPP

#include <iostream>
#include <memory>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/address.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace config {

/// Container for an [ip-address, port] tuple.
/// Subnet matching is employed when nonzero CIDR suffix is present.
/// Does not support serialization of ipv4 mapped (to ipv6) addresses.
/// Instead serialize ipv4 natively (using dotted vs. colon notation).
/// Provided for connection management (not p2p network messaging).
class BCT_API authority
{
public:
    typedef std::shared_ptr<authority> ptr;

    DEFAULT_COPY_MOVE_DESTRUCT(authority);

    authority() NOEXCEPT;

    /// Deserialize an IPv4 or IPv6 address-based hostname[:port].
    /// Literal IPv6 required (bracketed with []).
    authority(const std::string& authority) NOEXCEPT(false);
    authority(const asio::address& ip, uint16_t port, uint8_t cidr=0) NOEXCEPT;
    authority(const asio::endpoint& endpoint) NOEXCEPT;
    authority(const config::address& address) NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// The ip address of the authority.
    const asio::address& ip() const NOEXCEPT;

    /// The ip port of the authority.
    uint16_t port() const NOEXCEPT;

    /// The ip subnet mask in cidr format (zero implies none).
    uint8_t cidr() const NOEXCEPT;

    /// Methods.
    /// -----------------------------------------------------------------------

    /// The host of the authority as a string.
    /// The port is optional and not included if zero-valued.
    /// Form by type of address, either: 2001:db8::2 or 1.2.240.1
    std::string to_host() const NOEXCEPT;

    /// The host of the authority as a literal.
    /// The port is optional and not included if zero-valued.
    /// Form by type of address, either: [2001:db8::2] or 1.2.240.1
    std::string to_literal() const NOEXCEPT;

    /// The authority as a literal with optional port.
    /// Form either: [2001:db8::2][:port][/cidr] or 1.2.240.1[:port][/cidr].
    std::string to_string() const NOEXCEPT;

    /// Authority converted to messages::ip_address or messages::address_item.
    /// Message addresses are 16 byte ipv6 encoding with ipv4 addresses mapped.
    messages::ip_address to_ip_address() const NOEXCEPT;
    messages::address_item to_address_item() const NOEXCEPT;
    messages::address_item to_address_item(uint32_t timestamp,
        uint64_t services) const NOEXCEPT;

    /// Operators.
    /// -----------------------------------------------------------------------

    /// False if ip address is unspecified or port is zero.
    operator bool() const NOEXCEPT;

    /// Subnet matching is employed when a CIDR suffix is present.
    /// Equality considers ip:port (not for black/white list matching).
    bool operator==(const authority& other) const NOEXCEPT;
    bool operator!=(const authority& other) const NOEXCEPT;

    friend std::istream& operator>>(std::istream& input,
        authority& argument) NOEXCEPT(false);
    friend std::ostream& operator<<(std::ostream& output,
        const authority& argument) NOEXCEPT;

private:
    // These are not thread safe.
    asio::address ip_;
    uint16_t port_;
    uint8_t cidr_;
};

typedef std::vector<authority> authorities;

} // namespace config
} // namespace network
} // namespace libbitcoin

namespace std
{
template<>
struct hash<bc::network::config::authority>
{
    size_t operator()(const bc::network::config::authority& value) const NOEXCEPT
    {
        return std::hash<std::string>{}(value.to_literal());
    }
};
} // namespace std

#endif
