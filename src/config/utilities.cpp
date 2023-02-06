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

// C++11: use std::regex.
////#include <format>
#include <sstream>
#include <boost/format.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {
namespace config {

// string/string conversions.
// ----------------------------------------------------------------------------

// host:    [2001:db8::2] or  2001:db8::2  or 1.2.240.1
// returns: [2001:db8::2] or [2001:db8::2] or 1.2.240.1
std::string to_host_name(const std::string& host) NOEXCEPT
{
    if (host.find(":") == std::string::npos || is_zero(host.find("[")))
        return host;

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return (boost::format("[%1%]") % host).str();
    BC_POP_WARNING()
}

// host: [2001:db8::2] or 2001:db8::2 or 1.2.240.1
std::string to_text(const std::string& host, uint16_t port) NOEXCEPT
{
    std::stringstream authority{};

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    authority << to_host_name(host);
    if (port != messages::unspecified_ip_port)
        authority << ":" << port;

    return authority.str();
    BC_POP_WARNING()
}

std::string to_ipv6(const std::string& ipv4_address) NOEXCEPT
{
    return std::string("::ffff:") + ipv4_address;
}

// asio/asio conversions.
// ----------------------------------------------------------------------------

asio::ipv6 to_ipv6(const asio::ipv4& ipv4_address) NOEXCEPT
{
    boost::system::error_code ignore{};

    try
    {
        // Create an IPv6 mapped IPv4 address via serialization.
        const auto ipv6 = to_ipv6(ipv4_address.to_string());
        return asio::ipv6::from_string(ipv6, ignore);
    }
    catch (std::exception)
    {
        return {};
    }
}

asio::ipv6 to_ipv6(const asio::address& ip_address) NOEXCEPT
{
    BC_ASSERT_MSG(ip_address.is_v4() || ip_address.is_v6(),
        "The address must be either IPv4 or IPv6.");

    try
    {
        return ip_address.is_v6() ? ip_address.to_v6() :
            to_ipv6(ip_address.to_v4());
    }
    catch (std::exception)
    {
        return {};
    }
}

// asio/string conversions.
// ----------------------------------------------------------------------------

std::string to_ipv4_host(const asio::address& ip_address) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    static const boost::regex regular("^::ffff:([0-9\\.]+)$");
    const auto address = ip_address.to_string();
    boost::sregex_iterator it(address.begin(), address.end(), regular), end;
    if (it == end)
        return {};

    const auto& match = *it;
    return match[1];
    BC_POP_WARNING()
}

std::string to_ipv6_host(const asio::address& ip_address) NOEXCEPT
{
    // IPv6 URLs use a bracketed IPv6 address, see rfc2732.
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return (boost::format("[%1%]") % to_ipv6(ip_address)).str();
    BC_POP_WARNING()
}

std::string to_ip_host(const asio::address& ip_address) NOEXCEPT
{
    auto ipv4_host = to_ipv4_host(ip_address);
    return ipv4_host.empty() ? to_ipv6_host(ip_address) : ipv4_host;
}

// asio/messages conversions.
// ----------------------------------------------------------------------------

messages::ip_address to_address(const asio::ipv6& in) NOEXCEPT
{
    messages::ip_address out;
    const auto bytes = in.to_bytes();
    BC_ASSERT(bytes.size() == out.size());
    std::copy_n(bytes.begin(), bytes.size(), out.begin());
    return out;
}

asio::ipv6 to_address(const messages::ip_address& in) NOEXCEPT
{
    asio::ipv6::bytes_type bytes{};
    BC_ASSERT(bytes.size() == in.size());
    std::copy_n(in.begin(), in.size(), bytes.begin());
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const asio::ipv6 out(bytes);
    BC_POP_WARNING()
    return out;
}

} // namespace config
} // namespace network
} // namespace libbitcoin
