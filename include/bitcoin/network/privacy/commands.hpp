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
#ifndef LIBBITCOIN_NETWORK_PRIVACY_COMMANDS_HPP
#define LIBBITCOIN_NETWORK_PRIVACY_COMMANDS_HPP

#include <string>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace privacy {

/// v2 short message type identifiers (bip324).
/// A zero identifier indicates an unmapped command (13 byte encoding).

/// Get the command for a short identifier (empty if unassigned).
BCT_API const std::string& to_command(uint8_t identifier) NOEXCEPT;

/// Get the short identifier for a command (zero if unmapped).
BCT_API uint8_t to_identifier(const std::string& command) NOEXCEPT;

} // namespace privacy
} // namespace network
} // namespace libbitcoin

#endif
