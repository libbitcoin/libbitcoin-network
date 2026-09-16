/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#include <bitcoin/network/messages/peer/detail/send_transaction_reconciliation.hpp>

#include <bitcoin/network/messages/peer/enums/level.hpp>
#include <bitcoin/network/messages/peer/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

using namespace system;

const std::string send_transaction_reconciliation::command = "sendtxrcncl";
const uint32_t send_transaction_reconciliation::version_minimum = level::bip330;
const uint32_t send_transaction_reconciliation::version_maximum =
    level::maximum_protocol;

// static
size_t send_transaction_reconciliation::size(uint32_t) NOEXCEPT
{
    return sizeof(uint32_t)
        + sizeof(uint64_t);
}

// static
typename send_transaction_reconciliation::cptr
send_transaction_reconciliation::deserialize(uint32_t version,
    const std::span<const uint8_t>& data) NOEXCEPT
{
    system::istream source{ { data.begin(), data.end() } };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader));
    return reader ? message : nullptr;
}

// static
send_transaction_reconciliation send_transaction_reconciliation::deserialize(
    uint32_t version, reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    return
    {
        source.read_4_bytes_little_endian(),
        source.read_8_bytes_little_endian()
    };
}

bool send_transaction_reconciliation::serialize(uint32_t version,
    const system::data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer);
    return writer;
}

void send_transaction_reconciliation::serialize(uint32_t,
    writer& sink) const NOEXCEPT
{
    sink.write_4_bytes_little_endian(value);
    sink.write_8_bytes_little_endian(salt);
}

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin
