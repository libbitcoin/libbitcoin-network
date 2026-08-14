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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_PEER_ENUMS_IDENTIFIER_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_PEER_ENUMS_IDENTIFIER_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

// Single source for the message set. The second argument (witness) is used
// only by witness-carrying messages, and is ignored by the identifier.
#define PEER_MESSAGE_LIST(MESSAGE) \
    MESSAGE(address) \
    MESSAGE(alert) \
    MESSAGE(block, witness) \
    MESSAGE(bloom_filter_add) \
    MESSAGE(bloom_filter_clear) \
    MESSAGE(bloom_filter_load) \
    MESSAGE(client_filter) \
    MESSAGE(client_filter_checkpoint) \
    MESSAGE(client_filter_headers) \
    MESSAGE(compact_block, witness) \
    MESSAGE(compact_transactions, witness) \
    MESSAGE(fee_filter) \
    MESSAGE(get_address) \
    MESSAGE(get_blocks) \
    MESSAGE(get_client_filter_checkpoint) \
    MESSAGE(get_client_filter_headers) \
    MESSAGE(get_client_filters) \
    MESSAGE(get_compact_transactions) \
    MESSAGE(get_data) \
    MESSAGE(get_headers) \
    MESSAGE(headers) \
    MESSAGE(inventory) \
    MESSAGE(memory_pool) \
    MESSAGE(merkle_block) \
    MESSAGE(not_found) \
    MESSAGE(ping) \
    MESSAGE(pong) \
    MESSAGE(reject) \
    MESSAGE(send_address_v2) \
    MESSAGE(send_compact) \
    MESSAGE(send_headers) \
    MESSAGE(transaction, witness) \
    MESSAGE(version) \
    MESSAGE(version_acknowledge) \
    MESSAGE(witness_tx_id_relay)

#define PEER_IDENTIFIER(name, ...) name,
enum class identifier
{
    unknown,
    PEER_MESSAGE_LIST(PEER_IDENTIFIER)
};
#undef PEER_IDENTIFIER

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
