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

/// Container for an [ip-address, port, CIDR] tuple.
/// Subnet matching is employed when nonzero CIDR suffix is present.
/// Internal storage always denormalized to native IPv4/IPv6 (no mapped IPv6).
/// Provided for connection management (not p2p network messaging).
class BCT_API authority
{
public:
    typedef std::shared_ptr<authority> ptr;

    DEFAULT_COPY_MOVE_DESTRUCT(authority);

    authority() NOEXCEPT;

    /// Deserialize [IPv6]|IPv4[:port][/cidr] (IPv6 [literal]).
    authority(const std::string& authority) THROWS;
    authority(const asio::address& ip, uint16_t port, uint8_t cidr=0) NOEXCEPT;
    authority(const messages::address_item& item) NOEXCEPT;
    authority(const asio::endpoint& endpoint) NOEXCEPT;
    authority(const config::address& address) NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// The IPv4 or IPv6 address, denormalized (no mapped IPv6).
    const asio::address& ip() const NOEXCEPT;

    /// The ip port of the authority.
    uint16_t port() const NOEXCEPT;

    /// The ip subnet mask in cidr format (zero implies none).
    uint8_t cidr() const NOEXCEPT;

    /// Methods.
    /// -----------------------------------------------------------------------
    /// All serializations are denormalized (IPv6 or IPv4).

    /// The IPv4 or IPv6 address and port as an asio endpoint.
    asio::endpoint to_endpoint() const NOEXCEPT;

    /// IPv6|IPv4
    std::string to_host() const NOEXCEPT;

    /// [IPv6]|IPv4
    std::string to_literal() const NOEXCEPT;

    /// Serialize [IPv6]|IPv4[:port][/cidr] (IPv6 [literal]).
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

    /// Equality treats zero port as * and non-zero CIDR as subnet identifier.
    /// Equality is subnet containment when one subnet identifier is present.
    /// Distinct subnets are usequal even if intersecting, same subnets equal.
    bool operator==(const authority& other) const NOEXCEPT;
    bool operator!=(const authority& other) const NOEXCEPT;
    bool operator==(const messages::address_item& other) const NOEXCEPT;
    bool operator!=(const messages::address_item& other) const NOEXCEPT;

    /// Same format as construct(string) and to_string().
    friend std::istream& operator>>(std::istream& input,
        authority& argument) THROWS;
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
