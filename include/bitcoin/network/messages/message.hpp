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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_MESSAGE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_MESSAGE_HPP

#include <iterator>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/heading.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

constexpr auto empty_hash = system::sha256::double_hash(
    system::sha256::ablocks_t<zero>{});
constexpr auto empty_checksum = system::from_little_endian<uint32_t>(
    empty_hash);

inline uint32_t network_checksum(
    const system::hash_digest& hash) NOEXCEPT
{
    // TODO: verify this is just a cast.
    return system::from_little_endian<uint32_t>(hash);
}

inline system::hash_digest network_hash(
    const system::data_slice& data) NOEXCEPT
{
    // TODO: const should be baked into all hash(empty).
    return data.empty() ? empty_hash :
        system::bitcoin_hash(data.size(), data.begin());
}

/// Deserialize message object from the wire protocol encoding.
template <typename Message>
typename Message::cptr deserialize(const system::data_chunk& data,
    uint32_t version, const system::hash_cptr&) NOEXCEPT
{
    using namespace system;
    read::bytes::copy reader(data);

    auto message = Message::deserialize(version, reader);

    // TODO: 
    // Transaction is the only object that can use the wire hash.
    // Block can use the wire data to compute transaction and header hashes.
    // Header is fixed size and txs can be iterated using tx[*].size().
    // Set .hash property on header/tx and .header_hash/.tx_hashes on block.
    // These can be set here using a constexpr on Message type, passing
    // Message::deserialize(version, reader, data) to isolate parse. 
    // Transaction is the only message that could benefit from checksum relay,
    // however as txs relay hash there is no need for a checksum property.
    // The hash parameter is only available when hash has been generated just
    // to validate the incoming checksum, which is optional. This facilitates
    // block merkle tree validation, header identity, and each tx identity.

    return reader ? to_shared(std::move(message)) : nullptr;
}

/// Serialize message object to the wire protocol encoding.
template <typename Message>
system::chunk_ptr serialize(const Message& message, uint32_t magic,
    uint32_t version) NOEXCEPT
{
    using namespace system;
    const auto buffer_size = heading::size() + message.size(version);
    const auto buffer = std::make_shared<data_chunk>(buffer_size);
    const auto body_start = std::next(buffer->begin(), heading::size());
    const data_slab body(body_start, buffer->end());

    write::bytes::copy body_writer(body);
    message.serialize(version, body_writer);

    // TODO:
    // Obtain hash ptr from tx message (only), via constexpr isolation.
    // heading::factory otherwise generates the hash from body. The tx hash
    // cache can be populated from store or by transaction relay.

    const auto head = heading::factory(magic, Message::command, body, {});

    write::bytes::copy head_writer(*buffer);
    head.serialize(head_writer);

    BC_ASSERT(body_writer && head_writer);
    return buffer;
}

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
