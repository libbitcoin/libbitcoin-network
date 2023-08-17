/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/messages/version.hpp>

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;
    
// version.relay added by bip37.
const std::string version::command = "version";
const identifier version::id = identifier::version;
const uint32_t messages::version::version_minimum = level::minimum_protocol;
const uint32_t messages::version::version_maximum = level::maximum_protocol;

// Time stamps are always used in version messages.
constexpr auto with_timestamp = false;

// This is just a guess, required as memory guard.
constexpr size_t max_user_agent = max_uint8;

// static
typename version::cptr version::deserialize(uint32_t version,
    const system::data_chunk& data) NOEXCEPT
{
    system::istream source{ data };
    system::byte_reader reader{ source };
    const auto message = to_shared(deserialize(version, reader));
    return reader ? message : nullptr;
}

// static
version version::deserialize(uint32_t version, reader& source) NOEXCEPT
{
    const auto value = source.read_4_bytes_little_endian();

    // ************************************************************************
    // PROTOCOL:
    // The relay field is optional >= bip37.
    // But both peers cannot know each other's version when sending theirs.
    // This is a bug in the BIP37 design as it forces older peers to adapt to
    // the expansion of the version message, which is a clear compat break.
    // ************************************************************************
    const auto read_relay = [=](reader& source) NOEXCEPT
    {
        // ********************************************************************
        // PROTOCOL:
        // The exhaustion check allows peers that set 'value >= bip37' to
        // succeed without providing the relay byte. This is broadly observed
        // on the network, including by the satoshi client (see test cases).
        // BIP37 defines the relay as a bool byte. Presumably this must be
        // interpreted as any non-zero value and not simply bit zero.
        // ********************************************************************

        // Read relay if bip37 and source is not exhausted, otherwise set true.
        // This ignores the specified version, instead respecting the peer's
        // version, since the specified version is not yet negotiated. A true
        // relay value may then be ignored when negotiated version is < bip37.
        // If value >= bip37 with no relay byte, the source is invalidated.
        return (value >= level::bip37) && (source.is_exhausted() ||
            to_bool(source.read_byte()));
    };

    // ************************************************************************
    // PROTOCOL:
    // The protocol requires 'services' matches 'address_sender.services', but
    // this validation is disabled due to the broad inconsistency of nodes.
    // ************************************************************************
    return
    {
        value,
        source.read_8_bytes_little_endian(),
        source.read_8_bytes_little_endian(),
        address_item::deserialize(version, source, with_timestamp),
        address_item::deserialize(version, source, with_timestamp),
        source.read_8_bytes_little_endian(),
        source.read_string(max_user_agent),
        source.read_4_bytes_little_endian(),
        read_relay(source)
    };
}

bool version::serialize(uint32_t version,
    const system::data_slab& data) const NOEXCEPT
{
    system::ostream sink{ data };
    system::byte_writer writer{ sink };
    serialize(version, writer);
    return writer;
}

void version::serialize(uint32_t version, writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    // ************************************************************************
    // PROTOCOL:
    // The relay field is optional >= bip37.
    // But both peers cannot know each other's version when sending theirs.
    // This is a bug in the BIP37 design as it forces older peers to adapt to
    // the expansion of the version message, which is a clear compat break.
    // ************************************************************************
    const auto write_relay = [this](writer& sink) NOEXCEPT
    {
        // Write 'relay' if and only if the 'value' field supports bip37.
        // This ignores the specified version, as it is not yet negotiated.
        // The peer may ignore relay if negotiated version < bip37.
        if (value >= level::bip37)
            sink.write_byte(to_int<uint8_t>(relay));
    };

    // ************************************************************************
    // PROTOCOL:
    // The protocol requires 'services' matches 'address_sender.services', but
    // this is not enforced here due to the broad inconsistency of nodes.
    // ************************************************************************
    sink.write_4_bytes_little_endian(value);
    sink.write_8_bytes_little_endian(services);
    sink.write_8_bytes_little_endian(timestamp);
    address_receiver.serialize(version, sink, with_timestamp);
    address_sender.serialize(version, sink, with_timestamp);
    sink.write_8_bytes_little_endian(nonce);
    sink.write_string(user_agent);
    sink.write_4_bytes_little_endian(start_height);
    write_relay(sink);

    BC_ASSERT(sink && sink.get_write_position() - start == bytes);
}

// The 'version' parameter is presumed to be set to expected sender 'value'.
// This is required as the 'value' is not available on this static sizing.
size_t version::size(uint32_t version) const NOEXCEPT
{
    return sizeof(uint32_t)
        + sizeof(uint64_t)
        + sizeof(uint64_t)
        + address_item::size(version, with_timestamp)
        + address_item::size(version, with_timestamp)
        + sizeof(uint64_t)
        + variable_size(user_agent.length()) + user_agent.length()
        + sizeof(uint32_t)
        + ((version < level::bip37) ? zero : sizeof(uint8_t));
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
