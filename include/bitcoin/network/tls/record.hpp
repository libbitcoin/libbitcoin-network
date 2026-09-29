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
#ifndef LIBBITCOIN_NETWORK_TLS_RECORD_HPP
#define LIBBITCOIN_NETWORK_TLS_RECORD_HPP

#include <optional>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tls/schedule.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

/// Record protection of one direction of a connection (rfc8446 5), either
/// cleartext (before keys) or AEAD with a traffic secret. Not thread safe.
class BCT_API record final
{
public:
    DELETE_COPY_MOVE(record);

    /// Authentication tag size of both supported suites.
    static constexpr size_t tag_size = 16;

    record() NOEXCEPT;

    /// Protect subsequent records with the traffic secret of the suite. The
    /// sequence number is reset.
    void set_secret(uint16_t suite, const schedule::secret& traffic) NOEXCEPT;

    /// True if records are protected.
    bool is_protected() const NOEXCEPT;

    /// Append one record of the content type to out. Content must not exceed
    /// the maximum plaintext size.
    void seal(system::data_chunk& out, uint8_t type,
        const const_byte_span& content) NOEXCEPT;

    /// Open the fragment of one protected record given its header. False if
    /// not authenticated or no inner type is present.
    bool open(uint8_t& type, system::data_chunk& content,
        const const_byte_span& header,
        const const_byte_span& fragment) NOEXCEPT;

private:
    typedef system::aes128_gcm::nonce nonce;

    nonce next_nonce() NOEXCEPT;

    uint16_t suite_{};
    schedule::iv iv_{};
    uint64_t sequence_{};
    std::optional<system::aes128_gcm> aes_{};
    std::optional<system::chacha20_poly1305> chacha_{};
};

} // namespace tls
} // namespace network
} // namespace libbitcoin

#endif
