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
#include <bitcoin/network/messages/compact_transactions.hpp>

#include <cstddef>
#include <cstdint>
#include <numeric>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace bc::system;
    
const std::string compact_transactions::command = "blocktxn";
const identifier compact_transactions::id = identifier::compact_transactions;
const uint32_t compact_transactions::version_minimum = level::bip152;
const uint32_t compact_transactions::version_maximum = level::maximum_protocol;

// static
compact_transactions compact_transactions::deserialize(uint32_t version,
    reader& source, bool witness) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    const auto read_transactions = [=](reader& source) NOEXCEPT
    {
        const auto size = source.read_size(chain::max_block_size);
        chain::transactions transactions;
        transactions.reserve(size);

        for (size_t tx = 0; tx < size; ++tx)
            transactions.emplace_back(source, witness);

        return transactions;
    };

    return
    {
        source.read_hash(),
        read_transactions(source)
    };
}

void compact_transactions::serialize(uint32_t BC_DEBUG_ONLY(version),
    writer& sink, bool witness) const NOEXCEPT
{
    // sink.get_position() removed due to flipper conflict, commenting out debug
    // BC_DEBUG_ONLY(const auto bytes = size(version, witness);)
    // BC_DEBUG_ONLY(const auto start = sink.get_position();)

    sink.write_bytes(block_hash);
    sink.write_variable(transactions.size());

    for (const auto& tx: transactions)
        tx.to_data(sink, witness);

    // sink.get_position() removed due to flipper conflict, commenting out debug
    // BC_ASSERT(sink && sink.get_position() - start == bytes);
}

size_t compact_transactions::size(uint32_t, bool witness) const NOEXCEPT
{
    const auto sizes = [=](size_t total, const chain::transaction& tx) NOEXCEPT
    {
        return total + tx.serialized_size(witness);
    };

    return hash_size
        + variable_size(transactions.size()) + std::accumulate(
            transactions.begin(), transactions.end(), zero, sizes);
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
