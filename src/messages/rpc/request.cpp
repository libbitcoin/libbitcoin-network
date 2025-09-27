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
#include <bitcoin/network/messages/rpc/request.hpp>

#include <algorithm>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/rpc/enums/identifier.hpp>
#include <bitcoin/network/messages/rpc/enums/method.hpp>
#include <bitcoin/network/messages/rpc/enums/target.hpp>
#include <bitcoin/network/messages/rpc/enums/version.hpp>
#include <bitcoin/network/messages/rpc/heading.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

using namespace system;

const identifier request::id = identifier::request;
const std::string request::command = "request";

size_t request::size() const NOEXCEPT
{
    return
        from_method(method).size() + heading::space.size() +
        path.size() + heading::space.size() +
        from_version(version).size() + heading::crlf.size() +
        heading::fields_size(fields) +
        heading::crlf.size();
}

// static
typename request::cptr request::deserialize(const data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(reader));
    return reader ? message : nullptr;
}

// static
request request::deserialize(reader& source) NOEXCEPT
{
    const request out
    {
        to_method(source.read_line(heading::space)),
        source.read_line(heading::space),
        to_version(source.read_line()),
        heading::to_fields(source)
    };

    if (!out.valid())
        source.invalidate();

    return out;
}

bool request::serialize(const data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(writer);
    return writer;
}

void request::serialize(writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size();)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_line(from_method(method), heading::space);
    sink.write_line(path, heading::space);
    sink.write_line(from_version(version));
    heading::from_fields(fields, sink);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

rpc::target request::target() const NOEXCEPT
{
    return to_target(path, method);
}

bool request::valid() const NOEXCEPT
{
    // TODO: These imply properly parsed, not semantically correct.
    return (method != rpc::method::undefined) &&
        (version != rpc::version::undefined) &&
        (target() != rpc::target::undefined);
}

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin
