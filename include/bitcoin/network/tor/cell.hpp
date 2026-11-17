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
#ifndef LIBBITCOIN_NETWORK_TOR_CELL_HPP
#define LIBBITCOIN_NETWORK_TOR_CELL_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tor/constants.hpp>

namespace libbitcoin {
namespace network {
namespace tor {

/// A channel cell (tor-spec 3): circuit id, command and payload.
struct cell
{
    uint32_t circuit{};
    uint8_t command{};
    system::data_chunk payload{};
};

/// True if command has variable length payload (tor-spec 3).
constexpr bool is_variable(uint8_t value) NOEXCEPT
{
    return (value == command::versions) || (value >= command::vpadding);
}

/// Encode cell, fixed payload zero padded, circuit id 4 or narrow 2 bytes.
BCT_API system::data_chunk encode(const cell& value,
    bool narrow=false) NOEXCEPT;

/// Decode cell at front of data, returning size, zero if data incomplete.
BCT_API size_t decode(cell& out, const_byte_span data,
    bool narrow=false) NOEXCEPT;

} // namespace tor
} // namespace network
} // namespace libbitcoin

#endif
