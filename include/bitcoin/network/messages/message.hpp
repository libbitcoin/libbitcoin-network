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

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/heading.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
    
/// Serialize message object to the wire protocol encoding.
template <typename Message>
void serialize(Message& instance, system::writer& sink,
    uint32_t version) NOEXCEPT
{
    instance.serialize(version, sink);
}

/// Serialize message object to the wire protocol encoding.
template <typename Message>
system::chunk_ptr serialize(const Message& instance, uint32_t magic,
    uint32_t version) NOEXCEPT
{
    using namespace system;
    const auto buffer = std::make_shared<data_chunk>(heading::size() +
        instance.size(version));

    data_slab body(std::next(buffer->begin(), heading::size()), buffer->end());
    write::bytes::copy body_writer(body);
    serialize(instance, body_writer, version);

    write::bytes::copy head_writer(*buffer);
    heading::factory(magic, Message::command, body).serialize(head_writer);

    return buffer;
}

/// Deserialize message object from the wire protocol encoding.
template <typename Message>
typename Message::cptr deserialize(system::reader& source,
    uint32_t version) NOEXCEPT
{
    return system::to_shared(Message::deserialize(version, source));
}

/// Compute an internal representation of the message checksum.
BCT_API uint32_t network_checksum(const system::data_slice& data) NOEXCEPT;

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
