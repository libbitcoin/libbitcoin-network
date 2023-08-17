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
#include <bitcoin/network/messages/ping.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;

// ping.nonce added by bip31.
const identifier ping::id = identifier::ping;
const std::string ping::command = "ping";
const uint32_t ping::version_minimum = level::minimum_protocol;
const uint32_t ping::version_maximum = level::maximum_protocol;

// static
size_t ping::size(uint32_t version) NOEXCEPT
{
    // bip31 added nonce field
    return version < level::bip31 ? zero : sizeof(uint64_t);
}

// static
typename ping::cptr ping::deserialize(uint32_t version,
    const system::data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader));
    return reader ? message : nullptr;
}

// static
ping ping::deserialize(uint32_t version, reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    const auto nonce = version < level::bip31 ? 0u :
        source.read_8_bytes_little_endian();

    return { nonce };
}

bool ping::serialize(uint32_t version,
    const system::data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer);
    return writer;
}

void ping::serialize(uint32_t version, writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    if (version >= level::bip31)
        sink.write_8_bytes_little_endian(nonce);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
