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
#include <bitcoin/network/messages/rpc/response.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/rpc/enums/identifier.hpp>
#include <bitcoin/network/messages/rpc/enums/status.hpp>
#include <bitcoin/network/messages/rpc/enums/version.hpp>
#include <bitcoin/network/messages/rpc/heading.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

using namespace system;

const identifier response::id = identifier::response;
const std::string response::command = "response";

size_t response::size() const NOEXCEPT
{
    return
        from_version(version).size() + heading::space.size() +
        from_status(status).size() + heading::line.size() +
        heading::headers_size(headers) +
        heading::line.size();
}

// static
typename response::cptr response::deserialize(const data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(reader));
    return reader ? message : nullptr;
}

// static
response response::deserialize(reader& source) NOEXCEPT
{
    return
    {
        to_version(source.read_line(heading::space)),
        to_status(source.read_line()),
        heading::to_headers(source)
    };
}

bool response::serialize(const data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(writer);
    return writer;
}

void response::serialize(writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size();)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_line(from_version(version), heading::space);
    sink.write_line(from_status(status));
    heading::from_headers(headers, sink);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin
