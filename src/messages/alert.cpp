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
#include <bitcoin/network/messages/alert.hpp>

#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/alert_item.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;
    
const std::string alert::command = "alert";
const identifier alert::id = identifier::alert;
const uint32_t alert::version_minimum = level::minimum_protocol;
const uint32_t alert::version_maximum = level::maximum_protocol;

// static
typename alert::cptr alert::deserialize(uint32_t version,
    const system::data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader));
    return reader ? message : nullptr;
}

// static
alert alert::deserialize(uint32_t version, reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    // Stream limit is relative to current position.
    source.set_limit(source.read_size(chain::max_block_size));
    auto item = alert_item::deserialize(level::minimum_protocol, source);
    source.set_limit();

    return
    {
        std::move(item),
        source.read_bytes(source.read_size(chain::max_block_size))
    };
}

bool alert::serialize(uint32_t version,
    const system::data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer);
    return writer;
}

void alert::serialize(uint32_t version, writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_variable(payload.size(version));
    payload.serialize(version, sink);
    sink.write_variable(signature.size());
    sink.write_bytes(signature);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t alert::size(uint32_t version) const NOEXCEPT
{
    const auto signature_size = signature.size();
    const auto payload_size = payload.size(version);

    return
        variable_size(payload_size) + payload_size +
        variable_size(signature_size) + signature_size;
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
