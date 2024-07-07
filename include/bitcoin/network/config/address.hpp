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

/// Container for messages::address_item (with timstamp and services).
/// IPv4 addresses are converted to IPv6-mapped for message encoding.
/// Provided for connect/session, and serialization to/from hosts file.
class BCT_API address
{
public:
    typedef std::shared_ptr<address> ptr;

    DEFAULT_COPY_MOVE_DESTRUCT(address);

    address() NOEXCEPT;

    /// [IPv6]|IPv4[:8333][/timestamp[/services]] (IPv6 [literal]).
    address(const std::string& host) THROWS;
    address(messages::address_item&& item) NOEXCEPT;
    address(const messages::address_item& item) NOEXCEPT;
    address(const messages::address_item::cptr& message) NOEXCEPT;

    // Methods.
    // ------------------------------------------------------------------------
    // All values are denormalized (IPv6 or IPv4).

    /// The IPv4 or IPv6 address.
    asio::address to_ip() const NOEXCEPT;

    /// IPv6|IPv4.
    std::string to_host() const NOEXCEPT;

    /// [IPv6]|IPv4[:8333]/timestamp/services (IPv6 [literal]).
    std::string to_string() const NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// The address properties.
    bool is_v4() const NOEXCEPT;
    bool is_v6() const NOEXCEPT;
    const messages::ip_address& ip() const NOEXCEPT;
    uint16_t port() const NOEXCEPT;
    uint32_t timestamp() const NOEXCEPT;
    uint64_t services() const NOEXCEPT;

    /// Operators.
    /// -----------------------------------------------------------------------

    /// The address item.
    operator const messages::address_item& () const NOEXCEPT;
    operator const messages::address_item::cptr& () const NOEXCEPT;

    /// False if the port is zero.
    operator bool() const NOEXCEPT;

    /// Equality treats zero port as *.
    /// Does not compare times or services (used in address protocols).
    bool operator==(const address& other) const NOEXCEPT;
    bool operator!=(const address& other) const NOEXCEPT;
    bool operator==(const messages::address_item& other) const NOEXCEPT;
    bool operator!=(const messages::address_item& other) const NOEXCEPT;

    /// Same format as construct(string) and to_string().
    friend std::istream& operator>>(std::istream& input,
        address& argument) THROWS;
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

// TODO: define, advertise, and store proper URI, such as:
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
