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
#include <bitcoin/network/messages/compact_transactions.hpp>

#include <numeric>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;
    
const std::string compact_transactions::command = "blocktxn";
const identifier compact_transactions::id = identifier::compact_transactions;
const uint32_t compact_transactions::version_minimum = level::bip152;
const uint32_t compact_transactions::version_maximum = level::maximum_protocol;

// static
typename compact_transactions::cptr compact_transactions::deserialize(
    uint32_t version, const system::data_chunk& data, bool witness) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader, witness));
    return reader ? message : nullptr;
}

// static
compact_transactions compact_transactions::deserialize(uint32_t version,
    reader& source, bool witness) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    const auto read_transactions = [=](reader& source) NOEXCEPT
    {
        const auto size = source.read_size(chain::max_block_size);
        chain::transaction_cptrs transaction_ptrs;
        transaction_ptrs.reserve(size);

        for (size_t tx = 0; tx < size; ++tx)
            transaction_ptrs.emplace_back(
                new chain::transaction{ source, witness });

        return transaction_ptrs;
    };

    return
    {
        source.read_hash(),
        read_transactions(source)
    };
}

bool compact_transactions::serialize(uint32_t version,
    const system::data_slab& data, bool witness) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer, witness);
    return writer;
}

void compact_transactions::serialize(uint32_t BC_DEBUG_ONLY(version),
    writer& sink, bool witness) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version, witness);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_bytes(block_hash);
    sink.write_variable(transaction_ptrs.size());

    for (const auto& tx: transaction_ptrs)
        tx->to_data(sink, witness);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t compact_transactions::size(uint32_t, bool witness) const NOEXCEPT
{
    const auto sizes = [=](size_t total, const chain::transaction::cptr& tx) NOEXCEPT
    {
        return total + tx->serialized_size(witness);
    };

    return hash_size
        + variable_size(transaction_ptrs.size()) + std::accumulate(
            transaction_ptrs.begin(), transaction_ptrs.end(), zero, sizes);
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
