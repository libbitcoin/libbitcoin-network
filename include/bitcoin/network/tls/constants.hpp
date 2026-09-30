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
#ifndef LIBBITCOIN_NETWORK_TLS_CONSTANTS_HPP
#define LIBBITCOIN_NETWORK_TLS_CONSTANTS_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

/// TLS 1.3 wire values (rfc8446).

/// Protocol versions.
constexpr uint16_t legacy_version = 0x0303;
constexpr uint16_t version_13 = 0x0304;

/// Record sizes.
constexpr size_t record_header_size = 5;
constexpr size_t maximum_plaintext = 16384;
constexpr size_t maximum_ciphertext = maximum_plaintext + 256;

/// Content types (5.1).
namespace content
{
    constexpr uint8_t change_cipher_spec = 20;
    constexpr uint8_t alert = 21;
    constexpr uint8_t handshake = 22;
    constexpr uint8_t application_data = 23;
}

/// Handshake types (4).
namespace handshake
{
    constexpr uint8_t client_hello = 1;
    constexpr uint8_t server_hello = 2;
    constexpr uint8_t new_session_ticket = 4;
    constexpr uint8_t end_of_early_data = 5;
    constexpr uint8_t encrypted_extensions = 8;
    constexpr uint8_t certificate = 11;
    constexpr uint8_t certificate_request = 13;
    constexpr uint8_t certificate_verify = 15;
    constexpr uint8_t finished = 20;
    constexpr uint8_t key_update = 24;
    constexpr uint8_t message_hash = 254;
}

/// Extension types (4.2).
namespace extension
{
    constexpr uint16_t server_name = 0;
    constexpr uint16_t supported_groups = 10;
    constexpr uint16_t signature_algorithms = 13;
    constexpr uint16_t pre_shared_key = 41;
    constexpr uint16_t early_data = 42;
    constexpr uint16_t supported_versions = 43;
    constexpr uint16_t cookie = 44;
    constexpr uint16_t key_share = 51;
}

/// Alert descriptions (6).
namespace alert
{
    constexpr uint8_t close_notify = 0;
    constexpr uint8_t unexpected_message = 10;
    constexpr uint8_t bad_record_mac = 20;
    constexpr uint8_t record_overflow = 22;
    constexpr uint8_t handshake_failure = 40;
    constexpr uint8_t bad_certificate = 42;
    constexpr uint8_t unsupported_certificate = 43;
    constexpr uint8_t certificate_expired = 45;
    constexpr uint8_t certificate_unknown = 46;
    constexpr uint8_t illegal_parameter = 47;
    constexpr uint8_t unknown_ca = 48;
    constexpr uint8_t decode_error = 50;
    constexpr uint8_t decrypt_error = 51;
    constexpr uint8_t protocol_version = 70;
    constexpr uint8_t internal_error = 80;
    constexpr uint8_t user_canceled = 90;
    constexpr uint8_t missing_extension = 109;
    constexpr uint8_t certificate_required = 116;
}

/// Alert levels (6).
constexpr uint8_t alert_warning = 1;
constexpr uint8_t alert_fatal = 2;

/// Cipher suites (B.4).
constexpr uint16_t aes_128_gcm_sha256 = 0x1301;
constexpr uint16_t aes_256_gcm_sha384 = 0x1302;
constexpr uint16_t chacha20_poly1305_sha256 = 0x1303;

/// Named groups (4.2.7).
constexpr uint16_t secp256r1_group = 0x0017;
constexpr uint16_t secp384r1_group = 0x0018;
constexpr uint16_t x25519_group = 0x001d;

/// Signature schemes (4.2.3).
constexpr uint16_t ecdsa_secp256r1_sha256 = 0x0403;
constexpr uint16_t ecdsa_secp384r1_sha384 = 0x0503;

/// HelloRetryRequest random, SHA-256 of "HelloRetryRequest" (4.1.3).
constexpr system::data_array<32> retry_random
{
    0xcf, 0x21, 0xad, 0x74, 0xe5, 0x9a, 0x61, 0x11,
    0xbe, 0x1d, 0x8c, 0x02, 0x1e, 0x65, 0xb8, 0x91,
    0xc2, 0xa2, 0x11, 0x16, 0x7a, 0xbb, 0x8c, 0x5e,
    0x07, 0x9e, 0x09, 0xe2, 0xc8, 0xa8, 0x33, 0x9c
};

} // namespace tls
} // namespace network
} // namespace libbitcoin

#endif
