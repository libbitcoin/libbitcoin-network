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
#ifndef LIBBITCOIN_NETWORK_CONFIG_AUTHORITY_HPP
#define LIBBITCOIN_NETWORK_CONFIG_AUTHORITY_HPP

#include <memory>
#include <bitcoin/network/config/address.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/p2p/messages.hpp>

namespace libbitcoin {
namespace network {
namespace config {

/// Add message address types to base authority configuration class.
/// Message addresses are 16 byte ipv6 encoding with ipv4 addresses mapped.
class BCT_API authority
  : public system::config::authority
{
public:
    typedef std::shared_ptr<authority> ptr;

    DEFAULT_COPY_MOVE_DESTRUCT(authority);

    /// Use base class constructors.
    using system::config::authority::authority;

    authority(const messages::p2p::address_item& item) NOEXCEPT;

    /// Authority converted to messages::p2p::ip_address.
    messages::p2p::ip_address to_ip_address() const NOEXCEPT;

    /// Authority converted to messages::p2p::address_item.
    messages::p2p::address_item to_address_item() const NOEXCEPT;
    messages::p2p::address_item to_address_item(uint32_t timestamp,
        uint64_t services) const NOEXCEPT;

    /// Equality treats zero port as * and non-zero CIDR as subnet identifier.
    /// Equality is subnet containment when one subnet identifier is present.
    /// Distinct subnets are unequal even if intersecting, same subnets equal.
    bool operator==(const messages::p2p::address_item& other) const NOEXCEPT;
    bool operator!=(const messages::p2p::address_item& other) const NOEXCEPT;
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
