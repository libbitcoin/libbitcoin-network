/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/messages/address.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/address_item.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace bc::system;

const std::string address::command = "addr";
const identifier address::id = identifier::address;
const uint32_t address::version_minimum = level::minimum_protocol;
const uint32_t address::version_maximum = level::maximum_protocol;

// Time stamps are always used in address messages.
constexpr auto with_timestamp = true;

address address::deserialize(uint32_t version, system::reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    const auto size = source.read_size(max_address);
    address_items addresses;
    addresses.reserve(size);

    for (size_t address = 0; address < size; ++address)
        addresses.push_back(address_item::deserialize(
            version, source, with_timestamp));

    return { addresses };
}

void address::serialize(uint32_t version, writer& sink) const NOEXCEPT
{
    // sink.get_position() removed due to flipper conflict, commenting out debug
    // BC_DEBUG_ONLY(const auto bytes = size(version);)
    // BC_DEBUG_ONLY(const auto start = sink.get_position();)

    sink.write_variable(addresses.size());

    for (const auto& net: addresses)
        net.serialize(version, sink, with_timestamp);

    // sink.get_position() removed due to flipper conflict, commenting out debug
    // BC_ASSERT(sink && sink.get_position() - start == bytes);
}

size_t address::size(uint32_t version) const NOEXCEPT
{
    return variable_size(addresses.size()) +
        (addresses.size() * address_item::size(version, with_timestamp));
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
