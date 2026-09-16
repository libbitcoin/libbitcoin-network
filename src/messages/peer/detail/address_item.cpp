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
#include <bitcoin/network/messages/peer/detail/address_item.hpp>

#include <bitcoin/network/messages/peer/enums/level.hpp>
#include <bitcoin/network/messages/peer/enums/magic_numbers.hpp>
#include <bitcoin/network/messages/peer/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

using namespace system;
using system::config::ip_map_prefix;

constexpr auto ip_address_size = array_count<ip_address>;

// bytereader interface cannot expose templated method, so do here.
template <size_t Size>
data_array<Size> read_forward(reader& source) NOEXCEPT
{
    data_array<Size> out{};
    source.read_bytes(out.data(), Size);
    return out;
}

// The v1 protocol encodes only ip addresses, as v6 (v4 is v6-mapped).
address_t to_address(const ip_address& ip) NOEXCEPT
{
    if (is_v4(ip))
        return ipv4_t{ ip };

    return ipv6_t{ ip };
}

const ip_address& to_ip_address(const address_t& address) NOEXCEPT
{
    if (const auto value = std::get_if<ipv4_t>(&address))
        return value->value;

    if (const auto value = std::get_if<ipv6_t>(&address))
        return value->value;

    return unspecified_ip_address;
}

size_t hash_address(const address_t& address) NOEXCEPT
{
    return hash_combine(std::hash<size_t>{}(address.index()),
        std::visit([](const auto& value) NOEXCEPT
        {
            using type = std::decay_t<decltype(value)>;
            if constexpr (is_same_type<type, std::monostate>)
                return size_t{};
            else
                return std::hash<data_array<type::size>>{}(value.value);
        }, address));
}

// static
size_t address_item::size(uint32_t, bool with_timestamp) NOEXCEPT
{
    return (with_timestamp ? sizeof(uint32_t) : zero)
        + sizeof(uint64_t)
        + ip_address_size
        + sizeof(uint16_t);
}

// static
address_item address_item::deserialize(uint32_t, reader& source,
    bool with_timestamp) NOEXCEPT
{
    return
    {
        with_timestamp ? source.read_4_bytes_little_endian() : 0u,
        source.read_8_bytes_little_endian(),
        to_address(read_forward<ip_address_size>(source)),
        source.read_2_bytes_big_endian()
    };
}

void address_item::serialize(uint32_t BC_DEBUG_ONLY(version), writer& sink,
    bool with_timestamp) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version, with_timestamp);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    if (with_timestamp)
        sink.write_4_bytes_little_endian(timestamp);

    const auto& ip = to_ip_address(address);
    sink.write_8_bytes_little_endian(services);
    sink.write_bytes(ip.data(), ip.size());
    sink.write_2_bytes_big_endian(port);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

// BIP155 (address v2) entry codec.
// ----------------------------------------------------------------------------

template <uint8_t Id>
static address_t read_address(size_t size, reader& source) NOEXCEPT
{
    using type = address_at<Id>;
    constexpr auto offset = type::size - type::wire;

    if (size != type::wire)
    {
        source.invalidate();
        return {};
    }

    type out{};
    std::copy_n(ip_map_prefix.begin(), offset, out.value.begin());
    source.read_bytes(std::next(out.value.data(), offset), type::wire);
    return out;
}

static address_t read_address(reader& source) NOEXCEPT
{
    const auto id = source.read_byte();
    const auto size = source.read_size(max_address_bytes);

    switch (id)
    {
        case ipv4_t::id: return read_address<ipv4_t::id>(size, source);
        case ipv6_t::id: return read_address<ipv6_t::id>(size, source);
        case torv2_t::id: return read_address<torv2_t::id>(size, source);
        case torv3_t::id: return read_address<torv3_t::id>(size, source);
        case i2p_t::id: return read_address<i2p_t::id>(size, source);
        case cjdns_t::id: return read_address<cjdns_t::id>(size, source);

        // Unknown networks are discarded, not rejected (forward compatible).
        default:
            source.skip_bytes(size);
            return {};
    }
}

static void write_address(const address_t& address, writer& sink) NOEXCEPT
{
    sink.write_byte(narrow_cast<uint8_t>(address.index()));

    std::visit([&sink](const auto& value) NOEXCEPT
    {
        using type = std::decay_t<decltype(value)>;
        if constexpr (is_same_type<type, std::monostate>)
        {
            sink.write_variable(zero);
        }
        else
        {
            constexpr auto offset = type::size - type::wire;
            sink.write_variable(type::wire);
            sink.write_bytes(std::next(value.value.data(), offset), type::wire);
        }
    }, address);
}

static size_t address_size(const address_t& address) NOEXCEPT
{
    return std::visit([](const auto& value) NOEXCEPT
    {
        using type = std::decay_t<decltype(value)>;
        if constexpr (is_same_type<type, std::monostate>)
            return variable_size(zero);
        else
            return variable_size(type::wire) + type::wire;
    }, address);
}

size_t address_item::size_v2(uint32_t) const NOEXCEPT
{
    return sizeof(uint32_t)
        + variable_size(services)
        + sizeof(uint8_t)
        + address_size(address)
        + sizeof(uint16_t);
}

// static
address_item address_item::deserialize_v2(uint32_t, reader& source) NOEXCEPT
{
    return
    {
        source.read_4_bytes_little_endian(),
        source.read_variable(),
        read_address(source),
        source.read_2_bytes_big_endian()
    };
}

void address_item::serialize_v2(uint32_t BC_DEBUG_ONLY(version),
    writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size_v2(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_4_bytes_little_endian(timestamp);
    sink.write_variable(services);
    write_address(address, sink);
    sink.write_2_bytes_big_endian(port);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

// Equality ignores timestamp and services (used in hosts).
bool operator==(const address_item& left, const address_item& right) NOEXCEPT
{
    return left.address == right.address
        && left.port == right.port;
}

bool operator!=(const address_item& left, const address_item& right) NOEXCEPT
{
    return !(left == right);
}

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin
