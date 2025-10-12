/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_PEER_INVENTORY_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_PEER_INVENTORY_HPP

#include <memory>
#include <ranges>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/peer/enums/identifier.hpp>
#include <bitcoin/network/messages/peer/inventory_item.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

// see also get_data.
struct BCT_API inventory
{
    typedef std::shared_ptr<const inventory> cptr;
    typedef inventory_item::type_id type_id;

    static const identifier id;
    static const std::string command;
    static const uint32_t version_minimum;
    static const uint32_t version_maximum;

    // TODO: parameterize with witness parameter (once node is ready).
    static inventory factory(system::hashes&& hashes,
        type_id type) NOEXCEPT;
    static inventory factory(const system::hashes& hashes,
        type_id type) NOEXCEPT;

    static cptr deserialize(uint32_t version,
        const system::data_chunk& data) NOEXCEPT;
    static inventory deserialize(uint32_t version,
        system::reader& source) NOEXCEPT;

    bool serialize(uint32_t version,
        const system::data_slab& data) const NOEXCEPT;
    void serialize(uint32_t version,
        system::writer& sink) const NOEXCEPT;

    size_t size(uint32_t version) const NOEXCEPT;

    // get_data implements the same methods.
    auto view(type_id type) const NOEXCEPT;
    inventory_items filter(type_id type) const NOEXCEPT;
    system::hashes to_hashes(type_id type) const NOEXCEPT;
    size_t count(type_id type) const NOEXCEPT;
    bool any(type_id type) const NOEXCEPT;
    bool any_transaction() const NOEXCEPT;
    bool any_block() const NOEXCEPT;
    bool any_witness() const NOEXCEPT;

    inventory_items items;
};

inline auto inventory::view(type_id type) const NOEXCEPT
{
    const auto is_type = [type](const auto& item) NOEXCEPT
    {
        return item.type == type;
    };

    return std::ranges::filter_view(items, is_type);
}

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
