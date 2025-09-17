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
#include <bitcoin/network/messages/rpc/enums/version.hpp>
#include <bitcoin/network/messages/rpc/enums/verb.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

using namespace system;

const identifier request::id = identifier::request;
const std::string request::command = "request";

const std::string space{ " " };
const std::string separator{ ":" };
const std::string terminal{ "\r\n" };

size_t request::size() const NOEXCEPT
{
    const auto headers_size = [this]() NOEXCEPT
    {
        return std::accumulate(headers.begin(), headers.end(), zero,
            [](size_t sum, const auto& pair) NOEXCEPT
            {
                return sum +
                    pair.first.size() + separator.size() +
                    pair.second.size() + terminal.size();
            });
    };

    return
        from_verb(verb).size() + space.size() +
        path.size() + space.size() +
        from_version(version).size() + terminal.size() +
        headers_size() +
        terminal.size();
}

// static
typename request::cptr request::deserialize(const data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(reader));
    return reader ? message : nullptr;
}

// local
request::headers_t to_headers(reader& source) NOEXCEPT
{
    request::headers_t headers{};

    // Read until empty/fail or line starts with first terminal character.
    while (!source.is_exhausted() && (source.peek_byte() != terminal.front()))
    {
        // Must control read order here.
        auto header = source.read_line(separator);
        headers.emplace(std::move(header), source.read_line());
    }

    // Headers end with empty line.
    if (!source.read_line().empty())
        source.invalidate();

    if (!source)
        return {};

    // name:value pairs are not validated at this point.
    return headers;
}

// local
void from_headers(const request::headers_t& headers, writer& sink) NOEXCEPT
{
    // Write all headers.
    std::for_each(headers.begin(), headers.end(),
        [&sink](const auto& header) NOEXCEPT
        {
            sink.write_line(header.first, separator);
            sink.write_line(header.second);
        });

    // Headers end with empty line.
    sink.write_line();
}

// static
request request::deserialize(reader& source) NOEXCEPT
{
    return
    {
        to_verb(source.read_line(space)),
        source.read_line(space),
        to_version(source.read_line()),
        to_headers(source)
    };
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

    sink.write_line(from_verb(verb), space);
    sink.write_line(path, space);
    sink.write_line(from_version(version));
    from_headers(headers, sink);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin
