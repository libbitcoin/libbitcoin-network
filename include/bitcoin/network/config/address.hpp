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
#ifndef LIBBITCOIN_NETWORK_CONFIG_ADDRESS_HPP
#define LIBBITCOIN_NETWORK_CONFIG_ADDRESS_HPP

#include <iostream>
#include <memory>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace config {

// Always serializes to three tokens.
// Deserialization formats (ipv4/ipv6).
// 188.240.57.122:8333
// 188.240.57.122:8333/1675574490
// 188.240.57.122:8333/1675574490/1033

/// This is a container for messages::address_item.
class BCT_API address
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(address);

    typedef std::shared_ptr<address> ptr;

    address() NOEXCEPT;
    address(const std::string& host) NOEXCEPT(false);
    address(const messages::address_item& host) NOEXCEPT;

    /// True if the port is non-zero.
    operator bool() const NOEXCEPT;

    /// The address item.
    const messages::address_item& item() const NOEXCEPT;

    /// The address as a string.
    std::string to_string() const NOEXCEPT;

    bool operator==(const address& other) const NOEXCEPT;
    bool operator!=(const address& other) const NOEXCEPT;

    friend std::istream& operator>>(std::istream& input,
        address& argument) NOEXCEPT(false);
    friend std::ostream& operator<<(std::ostream& output,
        const address& argument) NOEXCEPT;

private:
    // This is not thread safe.
    messages::address_item address_{};
};

typedef std::vector<address> addresses;

} // namespace config
} // namespace network
} // namespace libbitcoin

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
