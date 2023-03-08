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
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {
namespace config {

using namespace system;
using namespace boost::asio;

constexpr uint8_t maximum_cidr_ip4 = 32;
constexpr uint8_t maximum_cidr_ip6 = 128;
static_assert(array_count<messages::ip_address> == ipv6_size);
static_assert(array_count<ip::address_v4::bytes_type> == ipv4_size);
static_assert(array_count<ip::address_v6::bytes_type> == ipv6_size);
static_assert(is_same_type<ip::address_v6::bytes_type, messages::ip_address>);

// Because system::deserialize doesn't convert empty to zero.
template <typename Integer>
inline bool to_integer(Integer& out, const std::string& in) NOEXCEPT
{
    if (in.empty())
    {
        out = Integer{};
        return true;
    }

    return system::deserialize(out, in);
}

// For calling consistency.
inline bool to_string(std::string& to, std::string&& from) NOEXCEPT
{
    to = std::move(from);
    return true;
}

// ASIO make_address allows a port on win32 (which is then lost), so guard in
// regex. ASIO addresses do not have ports, that's what endpoints are for.
inline bool make_address(asio::address& ip, const std::string& host) NOEXCEPT
{
    try
    {
        // Regex extracts literal host, non-win32 boost make_address rejects.
        ip = ip::make_address(system::trim_copy(host, { "[", "]" }));
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

// regex parsers.
// ----------------------------------------------------------------------------
// C++11: use std::regex.
using namespace boost;

// en.wikipedia.org/wiki/List_of_URI_schemes
// Schemes of p2p network and our zeromq endpoints.
#define SCHEME "(tcp|udp|http|https|inproc):\\/\\/"
#define IPV4   "([0-9.]+)"
#define IPV6   "\\[([0-9a-f:]+)]"
#define HOST   "([^:?/\\\\]+)"
#define PORT   ":([1-9][0-9]{0,4})"
#define CIDR   "\\/([1-9][0-9]{0,2})"
////#define IPV6E4 "\\[([0-9a-f:.]+)]"

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Excludes ipv4 mapped/compat, unbracketed, and ports allowed by make_address.
bool parse_host(asio::address& ip, const std::string& value) NOEXCEPT
{
    static const regex regular
    {
        "^(" IPV4 "|" IPV6 ")$"
    };

    sregex_iterator token{ value.begin(), value.end(), regular }, end{};
    return token != end
        && make_address(ip, (*token)[1]);
}

// Excludes ipv4 mapped/compat to ipv6.
bool parse_authority(asio::address& ip, uint16_t& port, uint8_t& cidr,
    const std::string& value) NOEXCEPT
{
    static const regex regular
    {
        "^(" IPV4 "|" IPV6 ")(" PORT ")?(" CIDR ")?$"
    };

    sregex_iterator token{ value.begin(), value.end(), regular }, end{};
    return token != end
        && make_address(ip, (*token)[1])
        && to_integer(port, (*token)[5])
        && to_integer(cidr, (*token)[7])
        && ((ip.is_v4() && cidr <= maximum_cidr_ip4) ||
            (ip.is_v6() && cidr <= maximum_cidr_ip6));
}

// Excludes ipv4 mapped/compat to ipv6.
bool parse_endpoint(std::string& scheme, std::string& host, uint16_t& port,
    const std::string& value) NOEXCEPT
{
    static const regex regular
    {
        "^(" SCHEME ")?(" IPV4 "|" IPV6 "|" HOST ")(" PORT ")?$"
    };

    sregex_iterator token{ value.begin(), value.end(), regular }, end{};
    return token != end
        && to_string(scheme, (*token)[2])
        && to_string(host,   (*token)[3])
        && to_integer(port,  (*token)[8]);
}

BC_POP_WARNING()

// asio/asio conversions.
// ----------------------------------------------------------------------------

inline bool is_embedded_v4(const asio::ipv6& ip6) NOEXCEPT
{
    return ip6.is_v4_mapped() || ip6.is_v4_compatible();
}

static asio::ipv6 to_v6(const asio::ipv4& ip4) NOEXCEPT
{
    try
    {
        return asio::ipv6{ splice(ip_map_prefix, ip4.to_bytes()) };
    }
    catch (const std::exception&)
    {
        return {};
    }
}

// Convert IPv6-mapped to IPV4 (ensures consistent internal matching).
// Reduce 4 encodings (IPv6, IPv6-mapped, IPv6-compat, IPv4) to 2 (IPv6, IPv4).
asio::address denormalize(const asio::address& ip) NOEXCEPT
{
    if (ip.is_v6())
    {
        try
        {
            const auto ip6 = ip.to_v6();
            if (is_embedded_v4(ip6)) return { ip6.to_v4() };
        }
        catch (const std::exception&)
        {
        }
    }

    return ip;
}

// asio/string host conversions.
// ----------------------------------------------------------------------------

inline std::string to_host(const asio::ipv6& ip6) NOEXCEPT
{
    try
    {
        return is_embedded_v4(ip6) ? to_host(ip6.to_v4()) : ip6.to_string();
    }
    catch (const std::exception&)
    {
        return { "::" };
    }
}

inline std::string to_host(const asio::ipv4& ip4) NOEXCEPT
{
    try
    {
        return ip4.to_string();
    }
    catch (const std::exception&)
    {
        return { "0.0.0.0" };
    }
}

// Serialize to host denormal form (unmapped) without ipv6 backets.
std::string to_host(const asio::address& ip) NOEXCEPT
{
    try
    {
        const auto host = denormalize(ip);
        return host.is_v4() ? to_host(host.to_v4()) : to_host(host.to_v6());
    }
    catch (const std::exception&)
    {
        return { "0.0.0.0" };
    }
}

// Serialize to host denormal form (unmapped) and with ipv6 backets.
std::string to_literal(const asio::address& ip) NOEXCEPT
{
    const auto host = to_host(ip);
    return (host.find(":") == std::string::npos) ? host : ("[" + host + "]");
}

// Rejects ipv6 mapped/compat to ipv4 and unbracketed ipv6.
asio::address from_host(const std::string& host) THROWS
{
    asio::address out{};
    if (!parse_host(out, host))
        throw istream_exception{ host };

    return out;
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

inline bool is_member_v4(const asio::ipv4& ip4, const asio::ipv4& net4,
    uint8_t cidr)  THROWS
{
    const auto hosts = ip::make_network_v4(net4, cidr).hosts();
    return hosts.find(ip4) != hosts.end();
}

inline bool is_member_v6(const asio::ipv6& ip6, const asio::ipv6& net6,
    uint8_t cidr) THROWS
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
    catch (const std::exception&)
    {
    }

    return false;
}

} // namespace config
} // namespace network
} // namespace libbitcoin
