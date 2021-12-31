/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_COMPACT_BLOCK_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_COMPACT_BLOCK_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/compact_block_item.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

struct BCT_API compact_block
{
    typedef std::shared_ptr<const compact_block> ptr;
    typedef system::mini_hash short_id;
    typedef system::mini_hash_list short_id_list;

    static const identifier id;
    static const std::string command;
    static const uint32_t version_minimum;
    static const uint32_t version_maximum;

    static compact_block deserialize(uint32_t version, system::reader& source,
        bool witness);
    void serialize(uint32_t version, system::writer& sink, bool witness) const;
    size_t size(uint32_t version, bool witness) const;

    system::chain::header header;
    uint64_t nonce;
    short_id_list short_ids;
    compact_block_item::list transactions;
};

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif