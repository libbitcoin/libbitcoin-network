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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_SV2_SETUP_CONNECTION_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_SV2_SETUP_CONNECTION_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/sv2/enums/identifiers.hpp>
#include <bitcoin/network/messages/sv2/types.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {

/// SetupConnection (client to server).
struct BCT_API setup_connection
{
    typedef std::shared_ptr<const setup_connection> cptr;

    static constexpr uint8_t identifier{ identifiers::setup_connection };
    static constexpr bool channel{ false };

    static cptr deserialize(const std::span<const uint8_t>& data) NOEXCEPT;
    static setup_connection deserialize(system::reader& source) NOEXCEPT;

    bool serialize(const system::data_slab& data) const NOEXCEPT;
    void serialize(system::writer& sink) const NOEXCEPT;

    size_t size() const NOEXCEPT;

    uint8_t protocol;
    uint16_t min_version;
    uint16_t max_version;
    uint32_t flags;
    std::string endpoint_host;
    uint16_t endpoint_port;
    std::string vendor;
    std::string hardware_version;
    std::string firmware;
    std::string device_id;
};

} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
