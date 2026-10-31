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
#include <bitcoin/network/config/utilities.hpp>

#include <optional>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace config {

using namespace system::config;

static_assert(array_count<messages::peer::ip_address> == system::ip::ipv6::size);

messages::peer::ip_address to_address(const system::ip::address& ip) NOEXCEPT
{
    return system::ip::normalize(ip).to_bytes();
}

system::ip::address from_address(
    const messages::peer::ip_address& address) NOEXCEPT
{
    return { system::ip::ipv6{ address } };
}

std::string to_normal_host(const std::string& host,
    uint16_t default_port) NOEXCEPT
{
    try
    {
        return endpoint{ host }.to_lower(default_port);
    }
    catch (const std::exception&)
    {
        return {};
    }
}

std::string to_normal_origin(const std::string& origin,
    uint16_t default_port) NOEXCEPT
{
    try
    {
        const url url{ origin };
        return endpoint{ url.host(), url.port() }.to_lower(default_port);
    }
    catch (const std::exception&)
    {
        return {};
    }
}

} // namespace config
} // namespace network
} // namespace libbitcoin
