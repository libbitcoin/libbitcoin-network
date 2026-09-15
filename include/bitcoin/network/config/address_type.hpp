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
#ifndef LIBBITCOIN_NETWORK_CONFIG_ADDRESS_TYPE_HPP
#define LIBBITCOIN_NETWORK_CONFIG_ADDRESS_TYPE_HPP

#include <bitcoin/network/config/utilities.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace config {

/// Count of address networks, including the unspecified network.
constexpr size_t address_types =
    std::variant_size_v<messages::peer::address_t>;

/// Count of pooled addresses, indexed by BIP155 network identifier.
typedef std::array<size_t, address_types> address_counts;

} // namespace config
} // namespace network
} // namespace libbitcoin

#endif
