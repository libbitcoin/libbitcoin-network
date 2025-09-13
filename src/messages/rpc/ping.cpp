/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/messages/rpc/ping.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/rpc/enums/identifier.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

using namespace system;

const identifier ping::id = identifier::ping;
const std::string ping::command = "ping";

// static
size_t ping::size() NOEXCEPT
{
    return sizeof(uint64_t);
}

// static
typename ping::cptr ping::deserialize(const data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(reader));
    return reader ? message : nullptr;
}

// static
ping ping::deserialize(reader& source) NOEXCEPT
{
    const auto nonce = source.read_8_bytes_little_endian();
    return { nonce };
}

bool ping::serialize(const data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(writer);
    return writer;
}

void ping::serialize(writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size();)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_8_bytes_little_endian(nonce);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin
