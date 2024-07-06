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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_ADDRESS_ITEM_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_ADDRESS_ITEM_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/enums/service.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

typedef system::data_array<16> ip_address;

struct BCT_API address_item
{
    typedef std::shared_ptr<const address_item> cptr;

    static size_t size(uint32_t version, bool with_timestamp) NOEXCEPT;
    static address_item deserialize(uint32_t version, system::reader& source,
        bool with_timestamp) NOEXCEPT;
    void serialize(uint32_t version, system::writer& sink,
        bool with_timestamp) const NOEXCEPT;

    uint32_t timestamp;
    uint64_t services;
    ip_address ip;
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
    unspecified_ip_address,
    unspecified_ip_port
};

constexpr bool is_specified(const address_item& item) NOEXCEPT
{
    // Specified if the host is not unspecified and port is non-zero.
    return !is_zero(item.port) && item.ip != unspecified_ip_address;
}

} // namespace messages

using address_item_cptr = messages::address_item::cptr;

} // namespace network
} // namespace libbitcoin

/// std lib hash table support (hosts).
namespace std
{
template<>
struct hash<bc::network::messages::address_item>
{
    size_t operator()(
        const bc::network::messages::address_item& value) const NOEXCEPT
    {
        return bc::system::hash_combine(
            std::hash<bc::network::messages::ip_address>{}(value.ip),
            std::hash<uint16_t>{}(value.port));
    }
};
} // namespace std

#endif
