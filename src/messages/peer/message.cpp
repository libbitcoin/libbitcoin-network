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
#include <bitcoin/network/messages/peer/message.hpp>

#include <iterator>
#include <memory>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/peer/heading.hpp>
#include <bitcoin/network/messages/peer/detail/transaction.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

template <>
inline system::chunk_ptr serialize<transaction>(const transaction& message,
    uint32_t magic, uint32_t version) NOEXCEPT
{
    using namespace system;
    const auto body_size = message.size(version);
    const auto size = heading::size() + body_size;
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const auto data = std::make_shared<data_chunk>(size);
    BC_POP_WARNING()
    const auto start = std::next(data->begin(), heading::size());
    const data_slab body(start, data->end());
    if (!message.serialize(version, body))
        return {};

    if (message.transaction_ptr->is_segregated())
    {
        if (!message.serialize(version, body) || !heading::factory(magic,
            transaction::command, body).serialize(*data))
            return {};
    }
    else
    {
        // The message heading hash must be full message hash (wtxid if witness
        // even if not using wtxid in relay). Since wtxid is not cached, this
        // is counterproductive (buffer hash is faster) except for non-witness
        // writes.
        const auto& hash = message.transaction_ptr->get_hash(false);

        if (!message.serialize(version, body) || !heading::factory(magic,
            transaction::command, body_size, hash).serialize(*data))
            return {};
    }

    return data;
}

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin
