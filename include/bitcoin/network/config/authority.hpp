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

/// This is a container for an {ip address, port} tuple.
class BCT_API authority
{
public:
    typedef std::shared_ptr<authority> ptr;

    DEFAULT_COPY_MOVE_DESTRUCT(authority);

    authority() NOEXCEPT;

    /// Deserialize an IPv4 or IPv6 address-based hostname[:port].
    /// Host can be either [2001:db8::2]:port or 1.2.240.1:port.
    authority(const std::string& authority) NOEXCEPT(false);
    authority(const std::string& ip, uint16_t port) NOEXCEPT(false);

    /// message conversion.
    ////authority(const messages::address_item& item) NOEXCEPT;
    authority(const messages::ip_address& ip, uint16_t port) NOEXCEPT;

    /// asio conversion.
    authority(const asio::address& ip, uint16_t port) NOEXCEPT;
    authority(const asio::endpoint& endpoint) NOEXCEPT;

    /// config conversion.
    authority(const config::address& address) NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// The ip address of the authority.
    const asio::ipv6& ip() const NOEXCEPT;

    /// The tcp port of the authority.
    uint16_t port() const NOEXCEPT;

    /// Methods.
    /// -----------------------------------------------------------------------

    /// The host of the authority as a string.
    /// The form of the return is determined by the type of address, either:
    /// 2001:db8::2 or 1.2.240.1
    std::string to_host() const NOEXCEPT;

    /// The authority as a string.
    /// The form of the return is determined by the type of address.
    /// The port is optional and not included if zero-valued.
    /// The authority in one of two forms: [2001:db8::2]:port or 1.2.240.1:port
    std::string to_string() const NOEXCEPT;

    /// The authority converted to a network messages address/ip_address.
    messages::ip_address to_ip_address() const NOEXCEPT;
    messages::address_item to_address_item() const NOEXCEPT;
    messages::address_item to_address_item(uint32_t timestamp,
        uint64_t services) const NOEXCEPT;

    /// Operators.
    /// -----------------------------------------------------------------------

    /// True if the port is non-zero.
    operator bool() const NOEXCEPT;

    /// Equality does consider port.
    bool operator==(const authority& other) const NOEXCEPT;
    bool operator!=(const authority& other) const NOEXCEPT;

    friend std::istream& operator>>(std::istream& input,
        authority& argument) NOEXCEPT(false);
    friend std::ostream& operator<<(std::ostream& output,
        const authority& argument) NOEXCEPT;

private:
    // These are not thread safe.
    asio::ipv6 ip_{};
    uint16_t port_{};
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
        return std::hash<std::string>{}(value.to_string());
    }
};
} // namespace std

#endif
