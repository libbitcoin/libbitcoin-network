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

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace config {

/// This is a container for a {ip address, port} tuple.
class BCT_API authority
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(authority);

    typedef std::shared_ptr<authority> ptr;

    authority() NOEXCEPT;

    /// Deserialize a IPv4 or IPv6 address-based hostname[:port].
    /// The port is optional and will be set to zero if not provided.
    /// The host can be in one of two forms:
    /// [2001:db8::2]:port or 1.2.240.1:port.
    authority(const std::string& authority) NOEXCEPT(false);

    authority(const messages::address_item& address) NOEXCEPT;
    authority(const messages::ip_address& ip, uint16_t port) NOEXCEPT;

    /// The host can be in one of three forms:
    /// [2001:db8::2] or 2001:db8::2 or 1.2.240.1
    authority(const std::string& host, uint16_t port) NOEXCEPT(false);
    authority(const asio::address& ip, uint16_t port) NOEXCEPT;
    authority(const asio::endpoint& endpoint) NOEXCEPT;

    /// True if the port is non-zero.
    operator bool() const NOEXCEPT;

    /// The ip address of the authority.
    const asio::ipv6& ip() const NOEXCEPT;

    /// The tcp port of the authority.
    uint16_t port() const NOEXCEPT;

    /// The hostname of the authority as a string.
    /// The form of the return is determined by the type of address, either:
    /// 2001:db8::2 or 1.2.240.1
    std::string to_hostname() const NOEXCEPT;

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

    bool operator==(const authority& other) const NOEXCEPT;
    bool operator!=(const authority& other) const NOEXCEPT;

    friend std::istream& operator>>(std::istream& input,
        authority& argument) NOEXCEPT(false);
    friend std::ostream& operator<<(std::ostream& output,
        const authority& argument) NOEXCEPT;

private:
    // These are not thread safe.
    asio::ipv6 ip_;
    uint16_t port_;
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
