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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_TRANSACTION_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_TRANSACTION_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

// TODO: build witness into feature w/magic and negotiated version.
struct BCT_API transaction
{
    typedef std::shared_ptr<const transaction> cptr;

    static const identifier id;
    static const std::string command;
    static const uint32_t version_minimum;
    static const uint32_t version_maximum;

    static system::hash_digest desegregated_hash(size_t witnessed,
        size_t unwitnessed, const uint8_t* data) NOEXCEPT;

    static cptr deserialize(uint32_t version, const system::data_chunk& data,
        bool witness=true) NOEXCEPT;
    static transaction deserialize(uint32_t version, system::reader& source,
        bool witness=true) NOEXCEPT;

    bool serialize(uint32_t version,
        const system::data_slab& data, bool witness=true) const NOEXCEPT;
    void serialize(uint32_t version, system::writer& sink,
        bool witness=true) const NOEXCEPT;

    size_t size(uint32_t version, bool witness) const NOEXCEPT;

    system::chain::transaction::cptr transaction_ptr;
};

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
