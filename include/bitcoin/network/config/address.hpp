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
#ifndef LIBBITCOIN_NETWORK_CONFIG_ADDRESS_HPP
#define LIBBITCOIN_NETWORK_CONFIG_ADDRESS_HPP

#include <iostream>
#include <memory>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace config {

/// Container for messages::address_item.
/// Provided for connect/session, and serialization to/from hosts file.
class BCT_API address
{
public:
    typedef std::shared_ptr<address> ptr;

    DEFAULT_COPY_MOVE_DESTRUCT(address);

    address() NOEXCEPT;

    /// 188.240.57.122[:8333][/1675574490[/1033]]
    /// Host can be either [2001:db8::2]:port or 1.2.240.1:port.
    address(const std::string& host) NOEXCEPT(false);
    address(messages::address_item&& item) NOEXCEPT;
    address(const messages::address_item& item) NOEXCEPT;
    address(const messages::address_item::cptr& message) NOEXCEPT;

    // Methods.
    // ------------------------------------------------------------------------

    /// Conversions.
    std::string to_string() const NOEXCEPT;
    std::string to_host() const NOEXCEPT;
    asio::address to_ip() const NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// The address item.
    const messages::address_item& item() const NOEXCEPT;
    const messages::address_item::cptr& message() const NOEXCEPT;

    /// The address properties.
    uint16_t port() const NOEXCEPT;
    uint32_t timestamp() const NOEXCEPT;
    uint64_t services() const NOEXCEPT;

    /// Operators.
    /// -----------------------------------------------------------------------

    /// False if the port is zero.
    operator bool() const NOEXCEPT;

    /// Equality does not consider timestamp/services (same as address_item).
    bool operator==(const address& other) const NOEXCEPT;
    bool operator!=(const address& other) const NOEXCEPT;

    friend std::istream& operator>>(std::istream& input,
        address& argument) NOEXCEPT(false);
    friend std::ostream& operator<<(std::ostream& output,
        const address& argument) NOEXCEPT;

private:
    // This is not thread safe.
    messages::address_item::cptr address_;
};

typedef std::vector<address> addresses;

} // namespace config
} // namespace network
} // namespace libbitcoin

// TODO: advertise and store proper URI.
// ipv4 uri: [btc://]1.2.3.4[:8333]
// ipv6 uri: [btc://]\[ab:cd::30:40\][:8333]
// name uri: [btc://]mainnet.libbitcoin.org[:8333]
// [?name[=value][&name[=value]]...]
// ?time=12345&version=700015&services=1033&sendaddrv2

namespace std
{
template<>
struct hash<bc::network::config::address>
{
    size_t operator()(const bc::network::config::address& value) const NOEXCEPT
    {
        return std::hash<std::string>{}(value.to_string());
    }
};
} // namespace std

#endif
