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

/// The TLS 1.3 key schedule (rfc8446 7) over SHA-256, the hash of both
/// supported cipher suites.
class BCT_API schedule final
{
public:
    typedef system::hash_digest secret;
    typedef system::data_array<12> iv;

    /// HKDF-Extract of the material with the salt.
    static secret extract(const secret& salt,
        const const_byte_span& material) NOEXCEPT;

    /// HKDF-Expand-Label of secret size.
    static secret expand_label(const secret& key, std::string_view label,
        const const_byte_span& context) NOEXCEPT;

    /// Derive-Secret of the transcript hash.
    static secret derive_secret(const secret& key, std::string_view label,
        const secret& transcript) NOEXCEPT;

    /// Early, handshake and master secrets (without pre-shared keys).
    static secret early_secret() NOEXCEPT;
    static secret handshake_secret(const secret& early,
        const const_byte_span& shared) NOEXCEPT;
    static secret master_secret(const secret& handshake) NOEXCEPT;

    /// Traffic key of the cipher suite and iv of a traffic secret (7.3).
    static system::data_chunk traffic_key(const secret& traffic,
        uint16_t suite) NOEXCEPT;
    static iv traffic_iv(const secret& traffic) NOEXCEPT;

    /// Finished verify data of the transcript hash (4.4.4).
    static secret finished(const secret& traffic,
        const secret& transcript) NOEXCEPT;

    /// Next application traffic secret (7.2).
    static secret update(const secret& traffic) NOEXCEPT;
};

} // namespace tls
} // namespace network
} // namespace libbitcoin

#endif
