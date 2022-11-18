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
#include <bitcoin/network/messages/bloom_filter_add.hpp>

#include <cstddef>
#include <cstdint>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace bc::system;
    
const std::string bloom_filter_add::command = "filteradd";
const identifier bloom_filter_add::id = identifier::bloom_filter_add;
const uint32_t bloom_filter_add::version_minimum = level::bip37;
const uint32_t bloom_filter_add::version_maximum = level::maximum_protocol;

bloom_filter_add bloom_filter_add::deserialize(uint32_t version,
    reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    return { source.read_bytes(source.read_size(max_bloom_filter_add)) };
}

void bloom_filter_add::serialize(uint32_t BC_DEBUG_ONLY(version),
    writer& sink) const NOEXCEPT
{
    // sink.get_position() removed due to flipper conflict, commenting out debug
    // BC_DEBUG_ONLY(const auto bytes = size(version);)
    // BC_DEBUG_ONLY(const auto start = sink.get_position();)

    sink.write_variable(data.size());
    sink.write_bytes(data);

    // sink.get_position() removed due to flipper conflict, commenting out debug
    // BC_ASSERT(sink && sink.get_position() - start == bytes);
}

size_t bloom_filter_add::size(uint32_t) const NOEXCEPT
{
    return variable_size(data.size()) + data.size();
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
