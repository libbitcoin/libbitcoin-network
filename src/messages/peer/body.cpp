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
        PEER_MESSAGE_LIST(PEER_DESERIALIZE_ANY)
        default: return {};
    }
}

#undef PEER_DESERIALIZE_ANY

// peer::body::reader
// ----------------------------------------------------------------------------
// The reader parses at most one message per read, deferring (consuming
// nothing) until the complete frame is buffered by the caller. The fault
// member carries parse failure detail (the boost code is generic).

static size_t set_fault(frame& value, const code& fault,
    boost_code& ec) NOEXCEPT
{
    value.fault = fault;
    ec = error::to_http_code(error::http_error_t::bad_value);
    return zero;
}

void body::reader::init(const http::length_type&, boost_code& ec) NOEXCEPT
{
    need_ = heading::size();
    headed_ = false;
    done_ = false;
    value_.fault = error::success;
    ec = {};
}

size_t body::reader::need() const NOEXCEPT
{
    return need_;
}

// private
bool body::reader::accept(const data_slice& payload, boost_code& ec) NOEXCEPT
{
    if (value_.checksum && value_.head.checksum !=
        network_checksum(bitcoin_hash(payload.size(), payload.data())))
    {
        set_fault(value_, error::invalid_checksum, ec);
        return false;
    }

    system::stream::in::fast stream{ payload };
    system::read::bytes::fast source{ stream };
    value_.payload = to_any(value_.head.id(), source, value_.version,
        value_.witness);

    if (!value_.payload)
    {
        set_fault(value_, error::invalid_message, ec);
        return false;
    }

    need_ = zero;
    done_ = true;
    return true;
}

size_t body::reader::put(const buffer_type& buffer, boost_code& ec) NOEXCEPT
{
    const auto data = static_cast<const uint8_t*>(buffer.data());
    const auto size = buffer.size();
    ec = {};

    // Payload.
    if (headed_)
    {
        if (size < need_)
            return zero;

        const data_slice payload{ data, std::next(data, need_) };
        return accept(payload, ec) ? payload.size() : zero;
    }

    // Heading.
    if (size < heading::size())
        return zero;

    const data_slice head{ data, std::next(data, heading::size()) };
    system::stream::in::fast stream{ head };
    system::read::bytes::fast source{ stream };
    value_.head = heading::deserialize(source);

    if (!source)
        return set_fault(value_, error::invalid_heading, ec);

    if (value_.head.magic != value_.magic)
        return set_fault(value_, error::invalid_magic, ec);

    if (value_.head.payload_size > value_.maximum)
        return set_fault(value_, error::oversized_payload, ec);

    headed_ = true;
    need_ = value_.head.payload_size;

    // An empty payload completes the message.
    if (is_zero(need_) && !accept({}, ec))
        return zero;

    return heading::size();
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
