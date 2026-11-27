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
#ifndef LIBBITCOIN_NETWORK_NOISE_CERTIFICATE_HPP
#define LIBBITCOIN_NETWORK_NOISE_CERTIFICATE_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/noise/cipher.hpp>

namespace libbitcoin {
namespace network {
namespace noise {

/// Stratum v2 authority certificate (SIGNATURE_NOISE_MESSAGE): version,
/// validity window (unix seconds) and the authority's bip340 signature of the
/// server static x-only key. The authority key is published to miners as
/// base58check of its version bytes and x-only key.
class BCT_API certificate final
{
public:
    /// Constants.
    static constexpr uint16_t version = 0;
    static constexpr size_t header_size = sizeof(uint16_t) +
        sizeof(uint32_t) + sizeof(uint32_t);
    static constexpr size_t authority_size = sizeof(uint16_t) +
        system::ec_xonly_size;

    typedef cipher::certificate data;

    /// The x-only key of a secret (false if the secret is invalid).
    static bool to_xonly(system::ec_xonly& out,
        const system::ec_secret& secret) NOEXCEPT;

    /// The x-only key of an ElligatorSwift encoded key.
    static bool to_xonly(system::ec_xonly& out,
        const cipher::key& key) NOEXCEPT;

    /// The x-only key of a compressed key.
    static system::ec_xonly to_xonly(const system::ec_compressed& key) NOEXCEPT;

    /// Sign the server key over the window, with random auxiliary data.
    static bool sign(data& out, const system::ec_secret& authority,
        const system::ec_xonly& server, uint32_t valid_from,
        uint32_t not_valid_after) NOEXCEPT;

    /// Sign the server key over the window (deterministic, for test vectors).
    static bool sign(data& out, const system::ec_secret& authority,
        const system::ec_xonly& server, uint32_t valid_from,
        uint32_t not_valid_after, const system::hash_digest& auxiliary) NOEXCEPT;

    /// Verify version, window (inclusive of now) and signature.
    static bool verify(const data& in, const system::ec_xonly& server,
        const system::ec_xonly& authority, uint32_t now) NOEXCEPT;

    /// The window of a certificate.
    static uint32_t valid_from(const data& in) NOEXCEPT;
    static uint32_t not_valid_after(const data& in) NOEXCEPT;

    /// Encode an authority key for publication.
    static std::string encode_authority(
        const system::ec_xonly& authority) NOEXCEPT;

    /// Decode a published authority key (false if malformed).
    static bool decode_authority(system::ec_xonly& out,
        const std::string& text) NOEXCEPT;
};

} // namespace noise
} // namespace network
} // namespace libbitcoin

#endif
