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
#include <bitcoin/network/zmtp/cipher.hpp>

namespace libbitcoin {
namespace network {
namespace zmtp {

/// Shared configuration for native ZMTP (ZeroMQ 3.x) transport sockets.
/// The owner must outlive all sockets created with a reference to it. The
/// context selects the zmtp upgrade, as p2ps::context selects p2ps, and
/// holds the server long-term keypair of the CURVE mechanism when one is
/// configured (otherwise the NULL mechanism is used), and the public keys
/// of the authorized clients (any client if none).
class BCT_API context
{
public:
    /// The NULL mechanism.
    context() NOEXCEPT;

    /// The CURVE mechanism with the given server long-term secret key and
    /// authorized client public keys. The mechanism is NULL if the secret or
    /// any client key is not of key_size or the secret is invalid.
    context(const system::data_chunk& secret,
        const system::data_stack& clients={}) NOEXCEPT;

    /// The CURVE mechanism is configured.
    bool curve() const NOEXCEPT;

    /// The server long-term keypair (valid if curve).
    const cipher::key& secret() const NOEXCEPT;
    const cipher::key& public_key() const NOEXCEPT;

    /// The client is authorized (any client if none are configured).
    bool authorized(const cipher::key& client) const NOEXCEPT;

private:
    bool curve_{};
    cipher::key secret_{};
    cipher::key public_{};
    std::vector<cipher::key> clients_{};
};

} // namespace zmtp
} // namespace network
} // namespace libbitcoin

#endif
