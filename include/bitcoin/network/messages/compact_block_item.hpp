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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_COMPACT_BLOCK_ITEM_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_COMPACT_BLOCK_ITEM_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

/// This is also known as a "prefilled transaction".
struct BCT_API compact_block_item
{
public:
    typedef std::shared_ptr<const compact_block_item> cptr;

    static compact_block_item deserialize(uint32_t version,
        system::reader& source, bool witness) NOEXCEPT;
    void serialize(uint32_t version, system::writer& sink,
        bool witness) const NOEXCEPT;
    size_t size(uint32_t version, bool witness) const NOEXCEPT;

    uint64_t index;
    system::chain::transaction::cptr transaction_ptr;
};

typedef std_vector<compact_block_item> compact_block_items;

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
