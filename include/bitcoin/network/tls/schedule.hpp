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
#ifndef LIBBITCOIN_NETWORK_TLS_SCHEDULE_HPP
#define LIBBITCOIN_NETWORK_TLS_SCHEDULE_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

/// The TLS 1.3 key schedule (rfc8446 7) over the hash of a cipher suite,
/// SHA-384 for TLS_AES_256_GCM_SHA384 and otherwise SHA-256.
class BCT_API schedule final
{
public:
    typedef system::data_chunk secret;
    typedef system::data_array<12> iv;

    schedule(uint16_t suite) NOEXCEPT;

    /// Size of the hash and of each secret.
    size_t size() const NOEXCEPT;

    /// Hash of the data.
    secret hash(const const_byte_span& data) const NOEXCEPT;

    /// HKDF-Extract of the material with the salt.
    secret extract(const secret& salt,
        const const_byte_span& material) const NOEXCEPT;

    /// HKDF-Expand-Label of secret size.
    secret expand_label(const secret& key, std::string_view label,
        const const_byte_span& context) const NOEXCEPT;

    /// Derive-Secret of the transcript hash.
    secret derive_secret(const secret& key, std::string_view label,
        const secret& transcript) const NOEXCEPT;

    /// Early, handshake and master secrets (without pre-shared keys).
    secret early_secret() const NOEXCEPT;
    secret handshake_secret(const secret& early,
        const const_byte_span& shared) const NOEXCEPT;
    secret master_secret(const secret& handshake) const NOEXCEPT;

    /// Traffic key of the cipher suite and iv of a traffic secret (7.3).
    system::data_chunk traffic_key(const secret& traffic) const NOEXCEPT;
    iv traffic_iv(const secret& traffic) const NOEXCEPT;

    /// Finished verify data of the transcript hash (4.4.4).
    secret finished(const secret& traffic,
        const secret& transcript) const NOEXCEPT;

    /// Next application traffic secret (7.2).
    secret update(const secret& traffic) const NOEXCEPT;

private:
    bool is_sha384() const NOEXCEPT;

    uint16_t suite_;
};

/// Running hash of the handshake messages (4.4.1), over the hash of a
/// cipher suite. Not thread safe.
class BCT_API transcript final
{
public:
    transcript() NOEXCEPT;

    /// Restart empty, hashing with the hash of the cipher suite.
    void reset(uint16_t suite) NOEXCEPT;

    /// Append a handshake message.
    void write(const const_byte_span& message) NOEXCEPT;

    /// Hash of the messages written.
    schedule::secret hash() const NOEXCEPT;

private:
    std::variant<system::accumulator<system::sha256>,
        system::accumulator<system::sha512_384>> state_{};
};

} // namespace tls
} // namespace network
} // namespace libbitcoin

#endif
