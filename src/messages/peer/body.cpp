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
#include <bitcoin/network/messages/peer/body.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/peer/message.hpp>
#include <bitcoin/network/messages/peer/peer.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(NO_UNGUARDED_POINTERS)
BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)

#define PEER_DESERIALIZE_ANY(name, ...) \
case identifier::name: \
{ \
    const name::cptr ptr = to_shared(name::deserialize(version_, source \
        __VA_OPT__(,) __VA_ARGS__)); \
    return source ? rpc::any_t{ ptr } : rpc::any_t{}; \
}

// Type-erased deserialization of the identified message payload.
static rpc::any_t to_any(identifier id, system::reader& source,
    uint32_t version_, bool witness) NOEXCEPT
{
    switch (id)
    {
        PEER_DESERIALIZE_ANY(address)
        PEER_DESERIALIZE_ANY(alert)
        PEER_DESERIALIZE_ANY(block, witness)
        PEER_DESERIALIZE_ANY(bloom_filter_add)
        PEER_DESERIALIZE_ANY(bloom_filter_clear)
        PEER_DESERIALIZE_ANY(bloom_filter_load)
        PEER_DESERIALIZE_ANY(client_filter)
        PEER_DESERIALIZE_ANY(client_filter_checkpoint)
        PEER_DESERIALIZE_ANY(client_filter_headers)
        PEER_DESERIALIZE_ANY(compact_block, witness)
        PEER_DESERIALIZE_ANY(compact_transactions, witness)
        PEER_DESERIALIZE_ANY(fee_filter)
        PEER_DESERIALIZE_ANY(get_address)
        PEER_DESERIALIZE_ANY(get_blocks)
        PEER_DESERIALIZE_ANY(get_client_filter_checkpoint)
        PEER_DESERIALIZE_ANY(get_client_filter_headers)
        PEER_DESERIALIZE_ANY(get_client_filters)
        PEER_DESERIALIZE_ANY(get_compact_transactions)
        PEER_DESERIALIZE_ANY(get_data)
        PEER_DESERIALIZE_ANY(get_headers)
        PEER_DESERIALIZE_ANY(headers)
        PEER_DESERIALIZE_ANY(inventory)
        PEER_DESERIALIZE_ANY(memory_pool)
        PEER_DESERIALIZE_ANY(merkle_block)
        PEER_DESERIALIZE_ANY(not_found)
        PEER_DESERIALIZE_ANY(ping)
        PEER_DESERIALIZE_ANY(pong)
        PEER_DESERIALIZE_ANY(reject)
        PEER_DESERIALIZE_ANY(send_address_v2)
        PEER_DESERIALIZE_ANY(send_compact)
        PEER_DESERIALIZE_ANY(send_headers)
        PEER_DESERIALIZE_ANY(transaction, witness)
        PEER_DESERIALIZE_ANY(version)
        PEER_DESERIALIZE_ANY(version_acknowledge)
        PEER_DESERIALIZE_ANY(witness_tx_id_relay)
        default: return {};
    }
}

#undef PEER_DESERIALIZE_ANY

// peer::body::reader
// ----------------------------------------------------------------------------
// The reader parses at most one message per read, deferring (consuming
// nothing) until the complete frame is buffered by the caller. The fault
// member carries parse failure detail (the boost code is generic).

void body::reader::init(const http::length_type&, boost_code& ec) NOEXCEPT
{
    done_ = false;
    value_.fault = error::success;
    ec = {};
}

size_t body::reader::put(const buffer_type& buffer, boost_code& ec) NOEXCEPT
{
    const auto data = static_cast<const uint8_t*>(buffer.data());
    const auto size = buffer.size();
    ec = {};

    // Defer until the heading is complete.
    if (size < heading::size())
        return zero;

    const data_slice head_slice{ data, std::next(data, heading::size()) };
    system::stream::in::fast head_stream{ head_slice };
    system::read::bytes::fast head_reader{ head_stream };
    value_.head = heading::deserialize(head_reader);

    if (!head_reader)
    {
        value_.fault = error::invalid_heading;
        ec = error::to_http_code(error::http_error_t::bad_value);
        return zero;
    }

    if (value_.head.magic != value_.magic)
    {
        value_.fault = error::invalid_magic;
        ec = error::to_http_code(error::http_error_t::bad_value);
        return zero;
    }

    const auto payload_size = value_.head.payload_size;
    if (payload_size > value_.maximum)
    {
        value_.fault = error::oversized_payload;
        ec = error::to_http_code(error::http_error_t::bad_value);
        return zero;
    }

    // Defer until the payload is complete.
    const auto frame_size = ceilinged_add(heading::size(), payload_size);
    if (size < frame_size)
        return zero;

    const auto payload = std::next(data, heading::size());
    if (value_.checksum && value_.head.checksum !=
        network_checksum(bitcoin_hash(payload_size, payload)))
    {
        value_.fault = error::invalid_checksum;
        ec = error::to_http_code(error::http_error_t::bad_value);
        return zero;
    }

    const data_slice payload_slice{ payload, std::next(payload, payload_size) };
    system::stream::in::fast payload_stream{ payload_slice };
    system::read::bytes::fast payload_reader{ payload_stream };
    value_.payload = to_any(value_.head.id(), payload_reader, value_.version,
        value_.witness);

    if (!value_.payload)
    {
        value_.fault = error::invalid_message;
        ec = error::to_http_code(error::http_error_t::bad_value);
        return zero;
    }

    done_ = true;
    return frame_size;
}

void body::reader::finish(boost_code& ec) NOEXCEPT
{
    ec = done_ ? boost_code{} :
        error::to_http_code(error::http_error_t::need_more);
}

bool body::reader::done() const NOEXCEPT
{
    return done_;
}

// peer::body::writer
// ----------------------------------------------------------------------------
// The writer emits the frame serialized by peer::serialize (in the value),
// as typed serialization is performed where the message type is static.

void body::writer::init(boost_code& ec) NOEXCEPT
{
    done_ = false;
    ec = value_.data ? boost_code{} :
        error::to_http_code(error::http_error_t::bad_value);
}

body::writer::out_buffer body::writer::get(boost_code& ec) NOEXCEPT
{
    ec = {};

    if (done_)
        return {};

    done_ = true;
    return out_buffer{ std::make_pair(const_buffers_type
        { value_.data->data(), value_.data->size() }, false) };
}

bool body::writer::done() const NOEXCEPT
{
    return done_;
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin
