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

static_assert(array_count<messages::ip_address> == 16);
static_assert(array_count<ip::address_v4::bytes_type> == 4);
static_assert(array_count<ip::address_v6::bytes_type> == 16);
static constexpr data_array<12> mapping_prefix
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff
};

// asio/asio conversions.
// ----------------------------------------------------------------------------

static asio::ipv6 to_v6(const asio::ipv4& ip4) NOEXCEPT
{
    return asio::ipv6{ splice(mapping_prefix, ip4.to_bytes()) };
}

// Convert IPv6-mapped to IPV4 (ensures consistent matching).
asio::address normalize(const asio::address& ip) NOEXCEPT
{
    if (ip.is_v6())
    {
        try
        {
            const auto ip6 = ip.to_v6();
            if (ip6.is_v4_mapped())
                return { ip6.to_v4() };
        }
        catch (std::exception)
        {
        }
    }

    return ip;
}

// asio/string host conversions.
// ----------------------------------------------------------------------------
// IPv4-Compatible IPv6 address are deprecated, support only IPv4-Mapped.
// rfc-editor.org/rfc/rfc4291

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
        const auto norm = normalize(ip);
        return norm.is_v4() ? to_host(norm.to_v4()) : to_host(norm.to_v6());
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
        throw istream_exception{ host };
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
    if (it == end) throw istream_exception{ host };
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
        return ip.is_v6() ? ip.to_v6() : to_v6(ip.to_v4());
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
        return { asio::ipv6{ address } };
    }
    catch (std::exception)
    {
        return { asio::ipv6{} };
    }
}

inline bool is_member_v4(const asio::ipv4& ip4, const asio::ipv4& net4,
    uint8_t cidr)  NOEXCEPT(false)
{
    const auto hosts = ip::make_network_v4(net4, cidr).hosts();
    return hosts.find(ip4) != hosts.end();
}

inline bool is_member_v6(const asio::ipv6& ip6, const asio::ipv6& net6,
    uint8_t cidr) NOEXCEPT(false)
{
    const auto hosts = ip::make_network_v6(net6, cidr).hosts();
    return hosts.find(ip6) != hosts.end();
}

// This assumes host and/or subnet v4 address(s) unmapped.
bool is_member(const asio::address& ip, const asio::address& subnet,
    uint8_t cidr) NOEXCEPT
{
    try
    {
        if (ip.is_v4() && subnet.is_v4())
            return is_member_v4(ip.to_v4(), subnet.to_v4(), cidr);

        if (ip.is_v6() && subnet.is_v6())
            return is_member_v6(ip.to_v6(), subnet.to_v6(), cidr);
    }
    catch (std::exception)
    {
    }

    return false;
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
