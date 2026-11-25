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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_SV2_HEADING_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_SV2_HEADING_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {

struct BCT_API heading
{
    typedef std::shared_ptr<const heading> cptr;

    /// The extension type bit of a message addressed to a channel.
    static constexpr uint16_t channel_bit{ 0x8000 };

    /// Largest payload (U24).
    static constexpr uint32_t maximum_payload{ 0x00ffffff };

    static constexpr size_t size() NOEXCEPT
    {
        return sizeof(uint16_t)
            + sizeof(uint8_t)
            + sizeof(uint8_t) * 3u;
    }

    /// Heading of a core protocol message, default if payload is oversized.
    static heading factory(uint8_t identifier, bool channel,
        size_t payload_size) NOEXCEPT;

    static cptr deserialize(const std::span<const uint8_t>& data) NOEXCEPT;
    static heading deserialize(system::reader& source) NOEXCEPT;
    bool serialize(const system::data_slab& data) const NOEXCEPT;
    void serialize(system::writer& sink) const NOEXCEPT;

    /// The message is addressed to a channel.
    bool channel() const NOEXCEPT;

    /// The extension, less the channel bit (zero for the core protocols).
    uint16_t extension() const NOEXCEPT;

    /// Registry index of the message, unknown if not a registered message.
    size_t index() const NOEXCEPT;

    uint16_t extension_type;
    uint8_t message_type;
    uint32_t message_length;
};

} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
