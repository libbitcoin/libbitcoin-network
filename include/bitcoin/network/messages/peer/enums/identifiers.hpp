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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_PEER_ENUMS_IDENTIFIERS_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_PEER_ENUMS_IDENTIFIERS_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {
namespace identifiers {

enum identifier_t : uint8_t
{
    unassigned = 0,
    address = 1,
    block = 2,
    compact_transactions = 3,
    compact_block = 4,
    fee_filter = 5,
    bloom_filter_add = 6,
    bloom_filter_clear = 7,
    bloom_filter_load = 8,
    get_blocks = 9,
    get_compact_transactions = 10,
    get_data = 11,
    get_headers = 12,
    headers = 13,
    inventory = 14,
    memory_pool = 15,
    merkle_block = 16,
    not_found = 17,
    ping = 18,
    pong = 19,
    send_compact = 20,
    transaction = 21,
    get_client_filters = 22,
    client_filter = 23,
    get_client_filter_headers = 24,
    client_filter_headers = 25,
    get_client_filter_checkpoint = 26,
    client_filter_checkpoint = 27,
    address_v2 = 28
};

} // namespace identifiers
} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
