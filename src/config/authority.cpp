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
#include <bitcoin/network/config/authority.hpp>

#include <bitcoin/network/config/utilities.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/p2p/messages.hpp>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {
namespace config {

authority::authority(const messages::p2p::address_item& item) NOEXCEPT
  : authority(from_address(item.ip), item.port)
{
}

messages::p2p::address_item authority::to_address_item() const NOEXCEPT
{
    return to_address_item({}, messages::p2p::service::node_none);
}

messages::p2p::address_item authority::to_address_item(uint32_t timestamp,
    uint64_t services) const NOEXCEPT
{
    return { timestamp, services, to_ip_address(), port() };
}

messages::p2p::ip_address authority::to_ip_address() const NOEXCEPT
{
    return config::to_address(ip());
}

bool authority::operator==(const messages::p2p::address_item& other) const NOEXCEPT
{
    // both non-zero ports must match (zero/non-zero or both zero are matched).
    if ((!is_zero(port()) && !is_zero(other.port)) && port() != other.port)
        return false;

    using namespace system::config;
    const auto host = denormalize(from_address(other.ip));

    // if both zero cidr, match hosts, otherwise host membership in subnet.
    return is_zero(cidr()) ? host == ip() : is_member(host, ip(), cidr());
}

bool authority::operator!=(const messages::p2p::address_item& other) const NOEXCEPT
{
    return !(*this == other);
}

} // namespace config
} // namespace network
} // namespace libbitcoin
