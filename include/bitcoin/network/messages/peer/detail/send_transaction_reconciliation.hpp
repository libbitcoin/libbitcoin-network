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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_PEER_SEND_TRANSACTION_RECONCILIATION_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_PEER_SEND_TRANSACTION_RECONCILIATION_HPP

#include <span>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/peer/enums/identifiers.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

/// Reconciliation is not supported, so this is never sent or acted upon.
struct BCT_API send_transaction_reconciliation
{
    typedef std::shared_ptr<const send_transaction_reconciliation> cptr;

    static constexpr uint8_t identifier{ identifiers::unassigned };
    static const uint32_t version_minimum;
    static const uint32_t version_maximum;
    static const std::string command;

    static size_t size(uint32_t version) NOEXCEPT;

    static cptr deserialize(uint32_t version,
        const std::span<const uint8_t>& data) NOEXCEPT;
    static send_transaction_reconciliation deserialize(uint32_t version,
        system::reader& source) NOEXCEPT;

    bool serialize(uint32_t version,
        const system::data_slab& data) const NOEXCEPT;
    void serialize(uint32_t version,
        system::writer& sink) const NOEXCEPT;

    uint32_t value;
    uint64_t salt;
};

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
