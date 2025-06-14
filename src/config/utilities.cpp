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
#include <bitcoin/network/config/utilities.hpp>

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {
namespace config {
    
using namespace boost::asio;
using namespace system::config;

static_assert(array_count<messages::ip_address> == ipv6_size);
static_assert(is_same_type<ip::address_v6::bytes_type, messages::ip_address>);

static asio::ipv6 to_v6(const asio::ipv4& ip4) NOEXCEPT
{
    try
    {
        return asio::ipv6{ system::splice(ip_map_prefix, ip4.to_bytes()) };
    }
    catch (const std::exception&)
    {
        return {};
    }
}

static asio::ipv6 get_ipv6(const asio::address& ip) NOEXCEPT
{
    try
    {
        return ip.is_v6() ? ip.to_v6() : to_v6(ip.to_v4());
    }
    catch (const std::exception&)
    {
        return {};
    }
}

messages::ip_address to_address(const asio::address& ip) NOEXCEPT
{
    return get_ipv6(ip).to_bytes();
}

asio::address from_address(const messages::ip_address& address) NOEXCEPT
{
    try
    {
        return { asio::ipv6{ address } };
    }
    catch (const std::exception&)
    {
        return { asio::ipv6{} };
    }
}

} // namespace config
} // namespace network
} // namespace libbitcoin
