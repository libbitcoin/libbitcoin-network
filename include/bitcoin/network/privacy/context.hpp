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
#ifndef LIBBITCOIN_NETWORK_PRIVACY_CONTEXT_HPP
#define LIBBITCOIN_NETWORK_PRIVACY_CONTEXT_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace privacy {

/// Shared configuration for bip324 (v2) transport sockets.
/// The owner must outlive all sockets created with a reference to it.
struct BCT_API context
{
    /// The network magic (heading identifier), keys the v2 key derivation
    /// salt, the v1 detection prefix, and synthesized v1 headings.
    uint32_t identifier{};
};

} // namespace privacy
} // namespace network
} // namespace libbitcoin

#endif
