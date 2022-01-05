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

#include <cstdint>
#include <cstddef>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/block.hpp>
#include <bitcoin/network/messages/compact_block.hpp>
#include <bitcoin/network/messages/heading.hpp>
#include <bitcoin/network/messages/transaction.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

/// Network protocol constants.
///----------------------------------------------------------------------------

/// Explicit limits.
constexpr size_t max_address = 1000;
constexpr size_t max_bloom_filter_add = 520;
constexpr size_t max_bloom_filter_functions = 50;
////constexpr size_t max_bloom_filter_hashes = 2000;
constexpr size_t max_bloom_filter_load = 36000;
constexpr size_t max_get_blocks = 500;
constexpr size_t max_get_headers = 2000;
////constexpr size_t max_get_data = 50000;
constexpr size_t max_inventory = 50000;
////constexpr size_t max_get_client_filter_headers = 1999;
////constexpr size_t max_get_client_filters = 99;

/////// compact filter checkpoint interval
////constexpr size_t client_filter_checkpoint_interval = 1000;

/////// Effective limit given a 32 bit chain height boundary: 10 + log2(2^32) + 1.
////constexpr size_t max_locator = 43;

// Serialization templates.
///----------------------------------------------------------------------------
/// TODO: define template concept for witnessable.

template <typename Message, system::if_base_of<Message, block> = true>
void serialize(Message& instance, system::writer& sink, uint32_t version,
    bool witness)
{
    instance.serialize(version, sink, witness);
}

template <typename Message, system::if_base_of<Message, transaction> = true>
void serialize(Message& instance, system::writer& sink, uint32_t version,
    bool witness)
{
    instance.serialize(version, sink, witness);
}

template <typename Message, system::if_base_of<Message, compact_block> = true>
void serialize(Message& instance, system::writer& sink, uint32_t version,
    bool witness)
{
    instance.serialize(version, sink, witness);
}

template <typename Message>
void serialize(Message& instance, system::writer& sink, uint32_t version, bool)
{
    instance.serialize(version, sink);
}

/// Serialize a message object to the Bitcoin wire protocol encoding.
template <typename Message>
system::chunk_ptr serialize(const Message& instance, uint32_t magic,
    uint32_t version, bool witness)
{
    const auto buffer = std::make_shared<system::data_chunk>(
        system::no_fill_byte_allocator);
    buffer->resize(heading::size() + instance.size(version));

    system::data_slab body(std::next(buffer->begin(), heading::size()),
        buffer->end());
    system::write::bytes::copy body_writer(body);
    serialize(instance, body_writer, version, witness);

    system::write::bytes::copy head_writer(*buffer);
    heading::factory(magic, Message::command, body).serialize(head_writer);

    return buffer;
}

template <typename Message, system::if_base_of<Message, block> = true>
typename Message::ptr deserialize(system::reader& source, uint32_t version,
    bool witness)
{
    return system::to_shared(Message::deserialize(version, source, witness));
}

template <typename Message, system::if_base_of<Message, transaction> = true>
typename Message::ptr deserialize(system::reader& source, uint32_t version,
    bool witness)
{
    return system::to_shared(Message::deserialize(version, source, witness));
}

template <typename Message, system::if_base_of<Message, compact_block> = true>
typename Message::ptr deserialize(system::reader& source, uint32_t version,
    bool witness)
{
    return system::to_shared(Message::deserialize(version, source, witness));
}

template <typename Message>
typename Message::ptr deserialize(system::reader& source, uint32_t version,
    bool)
{
    return system::to_shared(Message::deserialize(version, source));
}

template <typename Message>
typename Message::ptr deserialize(const system::data_chunk&, uint32_t, bool)
{
    ////system::read::bytes::copy source(data);
    ////auto message = deserialize<Message>(source, version, witness);

    ////if (source && source.is_exhausted())
    ////    return message;

    return nullptr;
}

/// Compute an internal representation of the message checksum.
BCT_API uint32_t network_checksum(const system::data_slice& data);

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
