/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/messages/message.hpp>

#include <cstdint>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace bc::system;

uint32_t network_checksum(const data_slice& data) NOEXCEPT
{
    return from_little_endian<uint32_t>(bitcoin_hash(data.size(), data.data()));
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
