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
#ifndef LIBBITCOIN_NETWORK_TOR_CERTIFICATE_HPP
#define LIBBITCOIN_NETWORK_TOR_CERTIFICATE_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace tor {

/// Tor certificate (cert-spec 2.1), certified key signed by Ed25519 key.
struct certificate
{
    uint8_t type{};

    /// Expiration, hours since unix epoch.
    uint32_t expiration{};

    uint8_t key_type{};
    system::ed25519::key key{};

    /// Signing key, if included (extension 4).
    bool has_signer{};
    system::ed25519::key signer{};

    system::data_chunk signed_data{};
    system::ed25519::signature signature{};
};

/// RSA identity certificate of Ed25519 identity (cert-spec 2.3).
struct crosscert
{
    system::ed25519::key key{};

    /// Expiration, hours since unix epoch.
    uint32_t expiration{};

    system::data_chunk signed_data{};
    system::data_chunk signature{};
};

/// Parse v1 certificate, false if malformed or unknown validating extension.
BCT_API bool parse(certificate& out, const_byte_span data) NOEXCEPT;

/// Parse cross-certificate, false if malformed.
BCT_API bool parse(crosscert& out, const_byte_span data) NOEXCEPT;

/// True if certificate signed by key.
BCT_API bool is_signed_by(const certificate& value,
    const system::ed25519::key& key) NOEXCEPT;

/// True if cross-certificate signed by RSA key.
BCT_API bool is_signed_by(const crosscert& value,
    const system::rsa::public_key& key) NOEXCEPT;

/// True if expiration (hours) not after time (unix seconds).
BCT_API bool is_expired(uint32_t expiration, uint64_t time) NOEXCEPT;

} // namespace tor
} // namespace network
} // namespace libbitcoin

#endif
