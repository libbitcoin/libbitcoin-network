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
#include <bitcoin/network/messages/send_headers.hpp>

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

const std::string send_headers::command = "sendheaders";
const identifier send_headers::id = identifier::send_headers;
const uint32_t send_headers::version_minimum = level::bip130;
const uint32_t send_headers::version_maximum = level::maximum_protocol;

// static
size_t send_headers::size(uint32_t)
{
    return zero;
}

send_headers send_headers::deserialize(uint32_t version, reader& source)
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    return {};
}

void send_headers::serialize(uint32_t BC_DEBUG_ONLY(version),
    writer& BC_DEBUG_ONLY(sink)) const
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_position();)
    BC_ASSERT(sink && sink.get_position() - start == bytes);
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
