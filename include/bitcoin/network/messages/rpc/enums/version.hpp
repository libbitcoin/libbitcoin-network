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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_ENUMS_VERSION_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_ENUMS_VERSION_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

/// Enumeration of JSON-RPC protocol versions.
enum class version
{
    /// Undefined version (not set).
    undefined,

    /// JSON-RPC 1.0 (bitcoin core).
    v1,

    /// JSON-RPC 2.0 (electrum daemon, electrum protocol, stratum v1).
    v2,

    /// Determine the version from the jsonrpc element.
    any,

    /// The jsonrpc value is invalid.
    invalid
};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
