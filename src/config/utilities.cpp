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
#include <bitcoin/network/config/utilities.hpp>

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {
namespace config {

using namespace system;
using namespace boost::asio;

// asio/string host conversions.
// ----------------------------------------------------------------------------

// C++11: use std::regex.
static const boost::regex mapped{ "^::ffff:([0-9\\.]+)$" };

bool is_mapped(const asio::ipv6& ip6) NOEXCEPT
{
    try
    {
        const auto host = ip6.to_string();
        boost::sregex_iterator it{ host.begin(), host.end(), mapped }, end{};
        return it != end;
    }
    catch (std::exception)
    {
        return false;
    }
}

static asio::ipv4 unmap(const asio::ipv6& ip6) NOEXCEPT
{
    try
    {
        const auto host = ip6.to_string();
        boost::sregex_iterator it{ host.begin(), host.end(), mapped }, end{};
        return it == end ? asio::ipv4{} : from_host((*it)[1]).to_v4();
    }
    catch (std::exception)
    {
        return {};
    }
}

static asio::ipv6 map(const asio::ipv4& ip4) NOEXCEPT
{
    try
    {
        return from_host("::ffff:" + ip4.to_string()).to_v6();
    }
    catch (std::exception)
    {
        return {};
    }
}

static std::string to_host(const asio::ipv6& ip6) NOEXCEPT
{
    try
    {
        return ip6.to_string();
    }
    catch (std::exception)
    {
        return { "::" };
    }
}

static std::string to_host(const asio::ipv4& ip4) NOEXCEPT
{
    try
    {
        return ip4.to_string();
    }
    catch (std::exception)
    {
        return { "0.0.0.0" };
    }
}

// Serialize to host normal form (unmapped).
std::string to_host(const asio::address& ip) NOEXCEPT
{
    try
    {
        if (ip.is_v4())
            return to_host(ip.to_v4());

        const auto ip6 = ip.to_v6();
        return is_mapped(ip6) ? to_host(unmap(ip6)) : to_host(ip6);
    }
    catch (std::exception)
    {
        return { "0.0.0.0" };
    }
}

// Deserialize any host.
asio::address from_host(const std::string& host) NOEXCEPT(false)
{
    try
    {
        return ip::make_address(host);
    }
    catch (std::exception)
    {
        throw istream_exception(host);
    }
}

// asio/string literal conversions.
// ----------------------------------------------------------------------------

static std::string bracket(const std::string& host6) NOEXCEPT
{
    // IPv6 URIs use a bracketed IPv6 address (literal), see rfc2732.
    return "[" + host6 + "]";
}

std::string to_literal(const asio::address& ip) NOEXCEPT
{
    const auto host = to_host(ip);
    return is_zero(host.find("[")) || host.find(":") == std::string::npos ?
        host : bracket(host);
}

asio::address from_literal(const std::string& host) NOEXCEPT(false)
{
    static const boost::regex litter{ "^(([0-9\\.]+)|\\[([0-9a-f:\\.]+)])$" };
    boost::sregex_iterator it{ host.begin(), host.end(), litter }, end{};
    if (it == end) throw istream_exception(host);
    const auto& token = *it;
    return from_host(is_zero(token[3].length()) ? token[2] : token[3]);
}

// asio/messages conversions.
// ----------------------------------------------------------------------------
// messages::ip_address <=> asio::ipv6 (mapped ipv4 as applicable).

static asio::ipv6 get_ipv6(const asio::address& ip) NOEXCEPT
{
    try
    {
        return ip.is_v6() ? ip.to_v6() : map(ip.to_v4());
    }
    catch (std::exception)
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
        const asio::ipv6 ip6(address);
        return is_mapped(ip6) ? asio::address(unmap(ip6)) : asio::address(ip6);
    }
    catch (std::exception)
    {
        return {};
    }
}

// Conditions.
// ----------------------------------------------------------------------------

bool is_valid(const messages::address_item& item) NOEXCEPT
{
    return !is_zero(item.port) && item.ip != messages::unspecified_ip_address;
}

} // namespace config
} // namespace network
} // namespace libbitcoin
