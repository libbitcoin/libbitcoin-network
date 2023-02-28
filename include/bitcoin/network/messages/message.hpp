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

/// Deserialize message object from the wire protocol encoding.
template <typename Message>
typename Message::cptr deserialize(const system::data_chunk& data,
    uint32_t version) NOEXCEPT
{
    using namespace system;
    read::bytes::copy reader(data);
    const auto message = to_shared(Message::deserialize(version, reader));
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

    write::bytes::copy head_writer(*buffer);
    heading::factory(magic, Message::command, body).serialize(head_writer);

    BC_ASSERT(body_writer && head_writer);
    return buffer;
}

/// Compute an internal representation of the message checksum.
BCT_API uint32_t network_checksum(const system::data_slice& data) NOEXCEPT;

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
