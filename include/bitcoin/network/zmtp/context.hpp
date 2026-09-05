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
#ifndef LIBBITCOIN_NETWORK_ZMTP_CONTEXT_HPP
#define LIBBITCOIN_NETWORK_ZMTP_CONTEXT_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace zmtp {

/// Shared configuration for native ZMTP (ZeroMQ 3.x) transport sockets.
/// The owner must outlive all sockets created with a reference to it. Only
/// the NULL mechanism is implemented, so there is nothing to configure yet;
/// the context selects the zmtp upgrade, as privacy::context selects p2ps.
struct BCT_API context
{
    // TODO: CURVE mechanism (server keypair), phase 2.
};

} // namespace zmtp
} // namespace network
} // namespace libbitcoin

#endif
