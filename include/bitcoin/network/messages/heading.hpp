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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HEADING_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HEADING_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/magic_numbers.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

struct BCT_API heading
{
    typedef std::shared_ptr<const heading> cptr;

    static constexpr size_t command_size = heading_command_size;
    static constexpr size_t maximum_payload(uint32_t, bool witness) NOEXCEPT
    {
        constexpr size_t inventory = sizeof(uint32_t) + system::hash_size;
        constexpr size_t data_size = inventory * max_inventory;
        constexpr size_t non_witness = variable_size(max_inventory) + data_size;
        return witness ? system::chain::max_block_weight : non_witness;
    }

    static std::string get_command(const system::data_chunk& payload) NOEXCEPT;
    static heading factory(uint32_t magic, const std::string& command,
        const system::data_slice& payload) NOEXCEPT;
    static heading factory(uint32_t magic, const std::string& command,
        const system::data_slice& payload,
        const system::hash_cptr& payload_hash) NOEXCEPT;

    // Heading does not use version.
    static constexpr size_t size() NOEXCEPT
    {
        return sizeof(uint32_t)
            + command_size
            + sizeof(uint32_t)
            + sizeof(uint32_t);
    }

    static cptr deserialize(const system::data_chunk& data) NOEXCEPT;
    static heading deserialize(system::reader& source) NOEXCEPT;
    bool serialize(const system::data_slab& data) const NOEXCEPT;
    void serialize(system::writer& sink) const NOEXCEPT;

    identifier id() const NOEXCEPT;

    uint32_t magic;
    std::string command;
    uint32_t payload_size;
    uint32_t checksum;
};

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
