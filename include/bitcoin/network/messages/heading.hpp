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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HEADING_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HEADING_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

struct BCT_API heading
{
    typedef std::shared_ptr<const heading> ptr;

    static constexpr size_t command_size = 12;

    static size_t maximum_payload_size(uint32_t version, bool witness);
    static heading factory(uint32_t magic, const std::string& command,
        const system::data_slice& payload);

    // Heading does not use version.
    static constexpr size_t size()
    {
        return sizeof(uint32_t)
            + command_size
            + sizeof(uint32_t)
            + sizeof(uint32_t);
    }

    static heading deserialize(system::reader& source);
    void serialize(system::writer& sink) const;

    identifier id() const;
    bool verify_checksum(const system::data_slice& body) const;

    uint32_t magic;
    std::string command;
    uint32_t payload_size;
    uint32_t checksum;
};

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
