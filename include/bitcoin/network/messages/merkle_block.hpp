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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_MERKLE_BLOCK_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_MERKLE_BLOCK_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

struct BCT_API merkle_block
{
public:
    typedef std::vector<merkle_block> list;
    typedef std::shared_ptr<const merkle_block> ptr;

    static const identifier id;
    static const std::string command;
    static const uint32_t version_minimum;
    static const uint32_t version_maximum;

    static merkle_block deserialize(uint32_t version, system::reader& source);
    void serialize(uint32_t version, system::writer& sink) const;
    size_t size(uint32_t version) const;

    // TODO: could use size_t for transactions.
    system::chain::header::ptr header;
    uint32_t transactions;
    system::hash_list hashes;
    system::data_chunk flags;
};

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
