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
#include <bitcoin/network/messages/peer/detail/address_v2.hpp>

#include <bitcoin/network/messages/peer/detail/address_item.hpp>
#include <bitcoin/network/messages/peer/enums/level.hpp>
#include <bitcoin/network/messages/peer/enums/magic_numbers.hpp>
#include <bitcoin/network/messages/peer/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

using namespace system;

const std::string address_v2::command = "addrv2";
const uint32_t address_v2::version_minimum = level::bip155;
const uint32_t address_v2::version_maximum = level::maximum_protocol;

// static
typename address_v2::cptr address_v2::deserialize(uint32_t version,
    const std::span<const uint8_t>& data) NOEXCEPT
{
    system::istream source{ { data.begin(), data.end() } };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader));
    return reader ? message : nullptr;
}

// static
address_v2 address_v2::deserialize(uint32_t version,
    system::reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    const auto size = source.read_size(max_address);
    address_items addresses;
    addresses.reserve(size);

    for (size_t address = 0; address < size; ++address)
        addresses.push_back(address_item::deserialize_v2(version, source));

    return { addresses };
}

bool address_v2::serialize(uint32_t version,
    const system::data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer);
    return writer;
}

void address_v2::serialize(uint32_t version, writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_variable(addresses.size());

    for (const auto& net: addresses)
        net.serialize_v2(version, sink);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t address_v2::size(uint32_t version) const NOEXCEPT
{
    auto size = variable_size(addresses.size());

    for (const auto& net: addresses)
        size += net.size_v2(version);

    return size;
}

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin
