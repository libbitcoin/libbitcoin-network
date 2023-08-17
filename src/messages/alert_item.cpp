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
#include <bitcoin/network/messages/alert_item.hpp>

#include <numeric>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/enums/magic_numbers.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;

// Unbounded except by network message size.
constexpr size_t max_message = max_inventory;
constexpr size_t max_messages = max_inventory;

// Libbitcoin doesn't use this.
const ec_uncompressed alert_item::satoshi_public_key
{
    {
        0x04, 0xfc, 0x97, 0x02, 0x84, 0x78, 0x40, 0xaa, 0xf1, 0x95, 0xde,
        0x84, 0x42, 0xeb, 0xec, 0xed, 0xf5, 0xb0, 0x95, 0xcd, 0xbb, 0x9b,
        0xc7, 0x16, 0xbd, 0xa9, 0x11, 0x09, 0x71, 0xb2, 0x8a, 0x49, 0xe0,
        0xea, 0xd8, 0x56, 0x4f, 0xf0, 0xdb, 0x22, 0x20, 0x9e, 0x03, 0x74,
        0x78, 0x2c, 0x09, 0x3b, 0xb8, 0x99, 0x69, 0x2d, 0x52, 0x4e, 0x9d,
        0x6a, 0x69, 0x56, 0xe7, 0xc5, 0xec, 0xbc, 0xd6, 0x82, 0x84
    }
};

alert_item alert_item::deserialize(uint32_t, reader& source) NOEXCEPT
{
    const auto read_cans = [](reader& source) NOEXCEPT
    {
        const auto size = source.read_size(max_messages);
        cancels_t cans;
        cans.reserve(size);

        for (size_t can = 0; can < size; can++)
            cans.push_back(source.read_4_bytes_little_endian());

        return cans;
    };

    const auto read_subs = [](reader& source) NOEXCEPT
    {
        const auto size = source.read_size(max_messages);
        sub_versions_t subs;
        subs.reserve(size);

        for (size_t sub = 0; sub < size; sub++)
            subs.push_back(source.read_string(max_message));

        return subs;
    };

    return
    {
        source.read_4_bytes_little_endian(),
        source.read_8_bytes_little_endian(),
        source.read_8_bytes_little_endian(),
        source.read_4_bytes_little_endian(),
        source.read_4_bytes_little_endian(),
        read_cans(source),
        source.read_4_bytes_little_endian(),
        source.read_4_bytes_little_endian(),
        read_subs(source),
        source.read_4_bytes_little_endian(),
        source.read_string(max_message),
        source.read_string(max_message),
        source.read_string(max_message)
    };
}

// Use "_" to avoid 'version' parameter hiding class member.
void alert_item::serialize(uint32_t BC_DEBUG_ONLY(version_),
    writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version_);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_4_bytes_little_endian(version);
    sink.write_8_bytes_little_endian(relay_until);
    sink.write_8_bytes_little_endian(expiration);
    sink.write_4_bytes_little_endian(id);
    sink.write_4_bytes_little_endian(cancel);
    sink.write_variable(cancels.size());

    for (const auto& entry: cancels)
        sink.write_4_bytes_little_endian(entry);

    sink.write_4_bytes_little_endian(min_version);
    sink.write_4_bytes_little_endian(max_version);
    sink.write_variable(sub_versions.size());

    for (const auto& entry: sub_versions)
        sink.write_string(entry);

    sink.write_4_bytes_little_endian(priority);
    sink.write_string(comment);
    sink.write_string(status_bar);
    sink.write_string(reserved);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t alert_item::size(uint32_t) const NOEXCEPT
{
    const auto subs = [](size_t total, const std::string& sub) NOEXCEPT
    {
        return total + variable_size(sub.length()) + sub.length();
    };

    return sizeof(uint32_t)
        + sizeof(uint64_t)
        + sizeof(uint64_t)
        + sizeof(uint32_t)
        + sizeof(uint32_t)
        + variable_size(cancels.size()) + 
            (cancels.size() * sizeof(uint32_t))
        + sizeof(uint32_t)
        + sizeof(uint32_t)
        + variable_size(sub_versions.size()) + std::accumulate(
            sub_versions.begin(), sub_versions.end(), zero, subs)
        + sizeof(uint32_t)
        + variable_size(comment.length()) + comment.length()
        + variable_size(status_bar.length()) + status_bar.length()
        + variable_size(reserved.length()) + reserved.length();
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
