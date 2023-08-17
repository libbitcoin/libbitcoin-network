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

// Sponsored in part by Digital Contract Design, LLC

#include <bitcoin/network/messages/client_filter.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;
    
const std::string client_filter::command = "cfilter";
const identifier client_filter::id = identifier::client_filter;
const uint32_t client_filter::version_minimum = level::bip157;
const uint32_t client_filter::version_maximum = level::maximum_protocol;

// static
typename client_filter::cptr client_filter::deserialize(uint32_t version,
    const system::data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader));
    return reader ? message : nullptr;
}

// static
client_filter client_filter::deserialize(uint32_t version,
    reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    return
    {
        source.read_byte(),
        source.read_hash(),
        source.read_bytes(source.read_size(chain::max_block_size))
    };
}

bool client_filter::serialize(uint32_t version,
    const system::data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer);
    return writer;
}

void client_filter::serialize(uint32_t BC_DEBUG_ONLY(version),
    writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_byte(filter_type);
    sink.write_bytes(block_hash);
    sink.write_variable(filter.size());
    sink.write_bytes(filter);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t client_filter::size(uint32_t) const NOEXCEPT
{
    return sizeof(uint8_t)
        + hash_size
        + variable_size(filter.size()) + filter.size();
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
