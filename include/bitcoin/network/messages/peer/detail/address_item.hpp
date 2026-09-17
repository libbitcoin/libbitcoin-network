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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_PEER_ADDRESS_ITEM_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_PEER_ADDRESS_ITEM_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/peer/enums/service.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

typedef system::data_array<16> ip_address;

/// True if ip_address starts with the ip map prefix (maps to a v4 address).
constexpr bool is_v4(const ip_address& ip) NOEXCEPT
{
    using namespace system::config;
    return std::equal(ip_map_prefix.begin(), ip_map_prefix.end(), ip.begin());
}

constexpr bool is_v6(const ip_address& ip) NOEXCEPT
{
    return !is_v4(ip);
}

/// The onioncat prefix, which maps a tor v2 address (fd87:d87e:eb43::/48).
constexpr system::data_array<6> torv2_map_prefix
{
    0xfd, 0x87, 0xd8, 0x7e, 0xeb, 0x43
};

/// The cjdns network range (fc00::/8).
constexpr uint8_t cjdns_prefix = 0xfc;

/// True if ip_address starts with the onioncat prefix (maps a tor v2 address).
constexpr bool is_torv2(const ip_address& ip) NOEXCEPT
{
    return std::equal(torv2_map_prefix.begin(), torv2_map_prefix.end(),
        ip.begin());
}

/// True if ip_address is within the cjdns network range.
constexpr bool is_cjdns(const ip_address& ip) NOEXCEPT
{
    return ip.front() == cjdns_prefix;
}

/// Distinct type per network, as variant alternatives must not repeat.
template <uint8_t Id, size_t Size, size_t Wire = Size>
struct address_of
{
    static constexpr uint8_t id = Id;
    static constexpr size_t size = Size;
    static constexpr size_t wire = Wire;

    system::data_array<Size> value;

    bool operator==(const address_of& other) const NOEXCEPT = default;
};

/// BIP155 addresses, ipv4 is stored v6-mapped (as encoded by the v1 protocol)
/// and is the only network whose wire size differs from its storage size.
using ipv4_t = address_of<1, 16, 4>;
using ipv6_t = address_of<2, 16>;

/// Tor v2 is not operational, this reserves its network identifier.
using torv2_t = address_of<3, 10>;
using torv3_t = address_of<4, 32>;
using i2p_t = address_of<5, 32>;
using cjdns_t = address_of<6, 16>;

/// The alternative index is the BIP155 network identifier.
using address_t = std::variant<std::monostate, ipv4_t, ipv6_t, torv2_t,
    torv3_t, i2p_t, cjdns_t>;

template <uint8_t Id>
using address_at = std::variant_alternative_t<Id, address_t>;

static_assert(is_same_type<address_at<ipv4_t::id>, ipv4_t>);
static_assert(is_same_type<address_at<ipv6_t::id>, ipv6_t>);
static_assert(is_same_type<address_at<torv2_t::id>, torv2_t>);
static_assert(is_same_type<address_at<torv3_t::id>, torv3_t>);
static_assert(is_same_type<address_at<i2p_t::id>, i2p_t>);
static_assert(is_same_type<address_at<cjdns_t::id>, cjdns_t>);

constexpr bool is_v4(const address_t& address) NOEXCEPT
{
    return std::holds_alternative<ipv4_t>(address);
}

constexpr bool is_v6(const address_t& address) NOEXCEPT
{
    return std::holds_alternative<ipv6_t>(address);
}

/// True if the address is representable by the v1 protocol.
constexpr bool is_v1(const address_t& address) NOEXCEPT
{
    return is_v4(address) || is_v6(address);
}

/// True if the address is a cjdns address (an ip address, routed as v6).
constexpr bool is_cjdns(const address_t& address) NOEXCEPT
{
    return std::holds_alternative<cjdns_t>(address);
}

/// True if the address is expressed as a host name (bip155 tor v3, i2p).
constexpr bool is_named(const address_t& address) NOEXCEPT
{
    return std::holds_alternative<torv3_t>(address)
        || std::holds_alternative<i2p_t>(address);
}

/// True if the address is unset or all zeros.
constexpr bool is_unspecified(const address_t& address) NOEXCEPT
{
    return std::visit([](const auto& value) NOEXCEPT
    {
        using type = std::decay_t<decltype(value)>;
        if constexpr (is_same_type<type, std::monostate>)
            return true;
        else
            return value == type{};
    }, address);
}

/// The v1 classification of an ip address (v4 is v6-mapped).
BCT_API address_t to_address(const ip_address& ip) NOEXCEPT;

/// The ip address, unspecified if the address has no ip form.
BCT_API const ip_address& to_ip_address(const address_t& address) NOEXCEPT;

/// Hash of the network identifier and the address bytes.
BCT_API size_t hash_address(const address_t& address) NOEXCEPT;

struct BCT_API address_item
{
    typedef std::shared_ptr<const address_item> cptr;

    static size_t size(uint32_t version, bool with_timestamp) NOEXCEPT;
    static address_item deserialize(uint32_t version, system::reader& source,
        bool with_timestamp) NOEXCEPT;
    void serialize(uint32_t version, system::writer& sink,
        bool with_timestamp) const NOEXCEPT;

    size_t size_v2(uint32_t version) const NOEXCEPT;
    static address_item deserialize_v2(uint32_t version,
        system::reader& source) NOEXCEPT;
    void serialize_v2(uint32_t version, system::writer& sink) const NOEXCEPT;

    uint32_t timestamp;
    uint64_t services;
    address_t address;
    uint16_t port;
};

// Equality ignores timestamp and services (used in hosts).
bool operator==(const address_item& left, const address_item& right) NOEXCEPT;
bool operator!=(const address_item& left, const address_item& right) NOEXCEPT;

typedef std_vector<address_item> address_items;
typedef std::shared_ptr<address_items> address_items_ptr;

// tools.ietf.org/html/rfc4291#section-2.5.3
constexpr ip_address loopback_ip_address =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

// tools.ietf.org/html/rfc4291#section-2.5.2
constexpr ip_address unspecified_ip_address
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

constexpr uint32_t unspecified_timestamp = 0;
constexpr uint16_t unspecified_ip_port = 0;
constexpr address_item unspecified_address_item
{
    unspecified_timestamp,
    service::node_none,
    {},
    unspecified_ip_port
};

constexpr bool is_specified(const address_item& item) NOEXCEPT
{
    // I2P addresses have no port, others require a nonzero port.
    return !is_unspecified(item.address) &&
        (!is_zero(item.port) || std::holds_alternative<i2p_t>(item.address));
}

} // namespace peer
} // namespace messages

using address_item_cptr = messages::peer::address_item::cptr;

} // namespace network
} // namespace libbitcoin

/// std lib hash table support (hosts).
namespace std
{
template<>
struct hash<bc::network::messages::peer::address_item>
{
    size_t operator()(
        const bc::network::messages::peer::address_item& value) const NOEXCEPT
    {
        return bc::system::hash_combine(
            bc::network::messages::peer::hash_address(value.address),
            std::hash<uint16_t>{}(value.port));
    }
};
} // namespace std

#endif
