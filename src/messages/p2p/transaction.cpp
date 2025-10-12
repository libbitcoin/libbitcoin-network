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
#include <bitcoin/network/messages/peer/transaction.hpp>

#include <iterator>
#include <bitcoin/network/messages/peer/enums/identifier.hpp>
#include <bitcoin/network/messages/peer/enums/level.hpp>
#include <bitcoin/network/messages/peer/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

using namespace system;
    
const std::string transaction::command = "tx";
const identifier transaction::id = identifier::transaction;
const uint32_t transaction::version_minimum = level::minimum_protocol;
const uint32_t transaction::version_maximum = level::maximum_protocol;

// static
typename transaction::cptr transaction::deserialize(uint32_t version,
    const data_chunk& data, bool witness) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader, witness));
    if (!reader)
        return nullptr;

    const auto& tx = message->transaction_ptr;

    // Cache transaction hashes.
    // If !witness then wire txs cannot have been segregated.
    if (tx->is_segregated())
    {
        const auto true_size = tx->serialized_size(true);
        const auto false_size = tx->serialized_size(false);
        tx->set_witness_hash(bitcoin_hash(true_size, data.data()));
        tx->set_nominal_hash(chain::transaction::desegregated_hash(
            true_size, false_size, data.data()));
    }
    else
    {
        const auto false_size = tx->serialized_size(false);
        tx->set_nominal_hash(bitcoin_hash(false_size, data.data()));
    }

    return message;
}

// static
transaction transaction::deserialize(uint32_t version, reader& source,
    bool witness) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    return { to_shared<chain::transaction>(source, witness) };
}

bool transaction::serialize(uint32_t version,
    const data_slab& data, bool witness) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer, witness);
    return writer;
}

void transaction::serialize(uint32_t BC_DEBUG_ONLY(version), writer& sink,
    bool witness) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version, witness);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    if (transaction_ptr)
        transaction_ptr->to_data(sink, witness);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

size_t transaction::size(uint32_t, bool witness) const NOEXCEPT
{
    return transaction_ptr ? transaction_ptr->serialized_size(witness) : zero;
}

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin
