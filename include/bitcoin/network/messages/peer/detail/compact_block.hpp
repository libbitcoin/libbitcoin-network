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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_PEER_COMPACT_BLOCK_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_PEER_COMPACT_BLOCK_HPP

#include <memory>
#include <span>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/peer/detail/compact_block_item.hpp>
#include <bitcoin/network/messages/peer/enums/identifiers.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

// TODO: build witness into feature w/magic and negotiated version.
struct BCT_API compact_block
{
    typedef std::shared_ptr<const compact_block> cptr;
    typedef system::mini_hash short_id;
    typedef std_vector<short_id> short_id_list;

    static constexpr uint8_t identifier{ identifiers::compact_block };
    static const uint32_t version_minimum;
    static const uint32_t version_maximum;
    static const std::string command;

    static cptr deserialize(uint32_t version, const std::span<const uint8_t>& data,
        bool witness=true) NOEXCEPT;
    static compact_block deserialize(uint32_t version, system::reader& source,
        bool witness=true) NOEXCEPT;

    bool serialize(uint32_t version,
        const system::data_slab& data, bool witness=true) const NOEXCEPT;
    void serialize(uint32_t version, system::writer& sink,
        bool witness=true) const NOEXCEPT;

    size_t size(uint32_t version, bool witness=true) const NOEXCEPT;

    system::chain::header::cptr header_ptr;
    uint64_t nonce;
    short_id_list short_ids;
    compact_block_items transactions;
};

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
