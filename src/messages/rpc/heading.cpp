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
#include <bitcoin/network/messages/rpc/heading.hpp>

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

using namespace system;

const std::string heading::space{ " " };
const std::string heading::separator{ ":" };
const std::string heading::line{ "\r\n" };
const std::string heading::terminal{ "\r\n\r\n" };

size_t heading::headers_size(const headers_t& headers) NOEXCEPT
{
    return std::accumulate(headers.begin(), headers.end(), zero,
        [](size_t sum, const auto& pair) NOEXCEPT
        {
            return sum +
                pair.first.size() + heading::separator.size() +
                pair.second.size() + heading::line.size();
        });
};

heading::headers_t heading::to_headers(reader& source) NOEXCEPT
{
    headers_t out{};

    // Read until empty/fail or line starts with first line character.
    while (!source.is_exhausted() && (source.peek_byte() != line.front()))
    {
        // Must control read order here.
        auto header = source.read_line(separator);
        out.emplace(std::move(header), source.read_line());
    }

    // Headers end with empty line.
    if (!source.read_line().empty())
        source.invalidate();

    if (!source)
        return {};

    // name:value pairs are not validated at this point.
    return out;
}

void heading::from_headers(const headers_t& headers, writer& sink) NOEXCEPT
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

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin
