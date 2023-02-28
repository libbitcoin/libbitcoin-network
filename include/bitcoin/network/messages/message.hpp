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

/// Deserialize message object from the wire protocol encoding.
template <typename Message>
typename Message::cptr deserialize(const system::data_chunk& data,
    uint32_t version /*, const system::hash_cptr& hash*/) NOEXCEPT
{
    using namespace system;
    read::bytes::copy reader(data);

    // TODO: each Message type determines support for hash/checksum caching.
    // TODO: hash may have been computed for checksum validation, but if not
    // TODO: data is provided for computation on the message. Checksum is cheap
    // TODO: drivation from the hash, if there is no need to retain the hash.
    const auto message = to_shared(Message::deserialize(version, reader /*, data, hash*/));

    return reader ? message : nullptr;
}

/// Serialize message object to the wire protocol encoding.
template <typename Message>
system::chunk_ptr serialize(const Message& instance, uint32_t magic,
    uint32_t version) NOEXCEPT
{
    using namespace system;
    const auto buffer_size = heading::size() + instance.size(version);
    const auto buffer = std::make_shared<data_chunk>(buffer_size);
    const auto body_start = std::next(buffer->begin(), heading::size());
    data_slab body(body_start, buffer->end());

    write::bytes::copy body_writer(body);
    instance.serialize(version, body_writer);

    // TODO: each Message type determines support for hash/checksum caching.
    // TODO: the value must be optional such as via std::optional<uint32_t>.
    // TODO: if hash is presered, checksum may be a computed property.
    ////const std::optional<uint32_t> checksum = instance.checksum();

    write::bytes::copy head_writer(*buffer);
    heading::factory(magic, Message::command, body /*, checksum*/)
        .serialize(head_writer);

    BC_ASSERT(body_writer && head_writer);
    return buffer;
}

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

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
