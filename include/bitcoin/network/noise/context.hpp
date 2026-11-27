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
#ifndef LIBBITCOIN_NETWORK_NOISE_CONTEXT_HPP
#define LIBBITCOIN_NETWORK_NOISE_CONTEXT_HPP

#include <optional>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/noise/certificate.hpp>
#include <bitcoin/network/noise/cipher.hpp>

namespace libbitcoin {
namespace network {
namespace noise {

/// Shared responder configuration of Noise sockets: the static keypair and
/// either a pre-issued certificate or the authority secret, with which a
/// certificate is issued for each handshake. Owner must outlive all sockets
/// created with reference to it. Thread safe.
class BCT_API context final
{
public:
    DELETE_COPY_MOVE(context);

    /// Present the pre-issued certificate.
    context(const system::ec_secret& static_secret,
        const certificate::data& issued) NOEXCEPT;

    /// Issue certificates valid for validity seconds either side of issue.
    context(const system::ec_secret& static_secret,
        const system::ec_secret& authority, uint32_t validity) NOEXCEPT;

    /// False if a secret is invalid.
    bool valid() const NOEXCEPT;

    /// The static keypair (the public key ElligatorSwift encoded).
    const system::ec_secret& static_secret() const NOEXCEPT;
    const cipher::key& static_key() const NOEXCEPT;

    /// The authority key (issuing only, otherwise empty).
    const std::optional<system::ec_xonly>& authority() const NOEXCEPT;

    /// The certificate to present at now (false if issuing fails).
    bool issue(certificate::data& out, uint32_t now) const NOEXCEPT;

private:
    context(const system::ec_secret& static_secret,
        const certificate::data& issued,
        const system::ec_secret& authority, uint32_t validity,
        bool issuing) NOEXCEPT;

    const system::ec_secret static_secret_;
    const certificate::data certificate_;
    const system::ec_secret authority_secret_;
    const uint32_t validity_;
    cipher::key static_key_{};
    system::ec_xonly server_{};
    std::optional<system::ec_xonly> authority_{};
    bool valid_{};
};

} // namespace noise
} // namespace network
} // namespace libbitcoin

#endif
